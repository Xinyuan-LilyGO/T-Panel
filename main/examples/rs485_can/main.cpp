/*
 * @Description: RS485/CAN throughput test for T-Panel
 * @Author: LILYGO_L
 * @Date: 2026-05-28
 * @License: GPL 3.0
 */
#include <cstdint>
#include <cstdio>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "cpp_bus_driver_library.h"
#include "driver/i2c_master.h"
#include "driver/twai.h"
#include "driver/uart.h"
#include "driver/usb_serial_jtag.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "t_panel_config.h"

namespace {

constexpr i2c_port_t kI2cPort = I2C_NUM_0;
constexpr int kI2cFreqHz = 400000;

constexpr uart_port_t kRs485UartPort = UART_NUM_2;
constexpr int kRs485BaudRate = 115200;
constexpr int kRs485RxBufferSize = 4096;
constexpr int kRs485TxBufferSize = 4096;
constexpr int kRs485MaxTransmitSize = 1024;
constexpr int kRs485DirectionSettleMs = 2;
constexpr int kRs485TxDoneWaitMs = 20;
constexpr char kRs485TestChar = 'D';

constexpr int kCanDataLength = 8;
constexpr int kCanTxWaitMs = 0;
constexpr int kCanPollMs = 100;
constexpr int kCanRecoveryExitWaitMs = 1000;
constexpr uint32_t kCanTestId = 0x0F1;
constexpr char kCanTestChar = 'C';
constexpr uint32_t kCanAlertFlags =
    TWAI_ALERT_TX_FAILED | TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_ERROR |
    TWAI_ALERT_RX_DATA | TWAI_ALERT_RX_QUEUE_FULL | TWAI_ALERT_BUS_OFF |
    TWAI_ALERT_BUS_RECOVERED;

constexpr int kPrintIntervalUs = 3 * 1000 * 1000;

std::shared_ptr<cpp_bus_driver::HardwareI2c1> g_i2c_bus;
std::unique_ptr<cpp_bus_driver::Xl95x5> g_xl9535;
TaskHandle_t g_rs485_task_handle = nullptr;
TaskHandle_t g_can_task_handle = nullptr;
volatile bool g_exit_test = false;
std::string g_console_buffer;
uint32_t g_can_pending_alerts = 0;
int64_t g_can_last_alert_print_us = 0;
bool g_can_recovering = false;

bool ValidateData(const char* tag, const uint8_t* data, size_t len,
    uint8_t expected_char) {
  for (size_t i = 0; i < len; ++i) {
    if (data[i] != expected_char) {
      printf("\n[%s] !!! data corruption error !!!\n", tag);
      printf("[%s] offset: %zu, expected: 0x%02X ('%c'), got: 0x%02X\n",
          tag, i, expected_char, expected_char, data[i]);
      printf("[%s] data snippet:", tag);
      const size_t start = (i > 5) ? i - 5 : 0;
      const size_t end = (i + 5 > len) ? len : i + 5;
      for (size_t j = start; j < end; ++j) {
        printf(" 0x%02X", data[j]);
      }
      printf("\n");
      g_exit_test = true;
      return false;
    }
  }

  return true;
}

void PrintProgress(const char* tag, size_t bytes_this_time, size_t total_size,
    int64_t now, int64_t last_print_time) {
  const float elapsed_s =
      static_cast<float>(now - last_print_time) / 1000000.0f;
  const float speed_kbps =
      (elapsed_s > 0.0f) ? static_cast<float>(bytes_this_time) / 1024.0f /
                               elapsed_s
                         : 0.0f;
  printf("[%s] size: %.2f kb | speed: %.2f kb/s | total size: %.2f kb\n",
      tag, static_cast<float>(bytes_this_time) / 1024.0f, speed_kbps,
      static_cast<float>(total_size) / 1024.0f);
}

void PrintResult(const char* tag, size_t total_size, int64_t start_time) {
  const float total_time_s =
      static_cast<float>(esp_timer_get_time() - start_time) / 1000000.0f;
  const float avg_speed =
      (total_time_s > 0.0f)
          ? static_cast<float>(total_size) / 1024.0f / total_time_s
          : 0.0f;
  printf("[%s] === result ===\n", tag);
  printf("[%s] total size: %.2f kb\n", tag,
      static_cast<float>(total_size) / 1024.0f);
  printf("[%s] total time %.3f s\n", tag, total_time_s);
  printf("[%s] avg speed: %.2f kb/s\n", tag, avg_speed);
}

bool InitXl9535() {
  if (g_xl9535 != nullptr) {
    return true;
  }

  if (g_i2c_bus == nullptr) {
    g_i2c_bus = std::make_shared<cpp_bus_driver::HardwareI2c1>(
        t_panel::gpio::i2c::kSda, t_panel::gpio::i2c::kScl, kI2cPort);
  }

  auto xl9535 = std::make_unique<cpp_bus_driver::Xl95x5>(
      g_i2c_bus, t_panel::device::xl95x5::kI2cAddress);

  if (!xl9535->Init(kI2cFreqHz)) {
    printf("Init XL9535 failed\n");
    return false;
  }

  g_xl9535 = std::move(xl9535);
  return true;
}

bool SetRs485Transmit(bool enable) {
  if (g_xl9535 == nullptr) {
    return false;
  }

  if (!g_xl9535->GpioWrite(
          t_panel::gpio::xl95x5::kRs485Con, enable ? 1 : 0)) {
    printf("Set RS485 direction failed\n");
    return false;
  }

  return true;
}

bool InitRs485() {
  if (!InitXl9535()) {
    return false;
  }
  if (!g_xl9535->SetGpioMode(t_panel::gpio::xl95x5::kRs485Con,
          cpp_bus_driver::Xl95x5::Mode::kOutput)) {
    printf("Set RS485 direction pin mode failed\n");
    return false;
  }
  SetRs485Transmit(false);

  uart_config_t uart_config = {};
  uart_config.baud_rate = kRs485BaudRate;
  uart_config.data_bits = UART_DATA_8_BITS;
  uart_config.parity = UART_PARITY_DISABLE;
  uart_config.stop_bits = UART_STOP_BITS_1;
  uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  uart_config.source_clk = UART_SCLK_DEFAULT;

  esp_err_t err = uart_driver_install(kRs485UartPort, kRs485RxBufferSize,
      kRs485TxBufferSize, 0, nullptr, 0);
  if (err != ESP_OK) {
    printf("Install RS485 UART driver failed: %s\n", esp_err_to_name(err));
    return false;
  }
  err = uart_param_config(kRs485UartPort, &uart_config);
  if (err != ESP_OK) {
    printf("Config RS485 UART failed: %s\n", esp_err_to_name(err));
    uart_driver_delete(kRs485UartPort);
    return false;
  }
  err = uart_set_pin(kRs485UartPort, t_panel::gpio::rs485::kTx,
      t_panel::gpio::rs485::kRx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  if (err != ESP_OK) {
    printf("Set RS485 UART pins failed: %s\n", esp_err_to_name(err));
    uart_driver_delete(kRs485UartPort);
    return false;
  }

  return true;
}

void DeinitRs485() {
  SetRs485Transmit(false);
  uart_driver_delete(kRs485UartPort);
}

void Rs485Task(void* param) {
  const bool is_send = (param != nullptr);
  printf("rs485_test_task (%s) start\n", is_send ? "send" : "receive");

  if (!InitRs485()) {
    g_rs485_task_handle = nullptr;
    vTaskDelete(nullptr);
    return;
  }

  size_t total_size = 0;
  size_t bytes_this_time = 0;
  int64_t start_time = esp_timer_get_time();
  int64_t last_print_time = start_time;

  if (is_send) {
    std::string payload(kRs485MaxTransmitSize, kRs485TestChar);
    SetRs485Transmit(true);
    vTaskDelay(pdMS_TO_TICKS(kRs485DirectionSettleMs));

    while (!g_exit_test) {
      const int len = uart_write_bytes(kRs485UartPort, payload.data(),
          payload.size());
      if (len > 0) {
        uart_wait_tx_done(kRs485UartPort, pdMS_TO_TICKS(kRs485TxDoneWaitMs));
        bytes_this_time += len;
        total_size += len;
      }

      const int64_t now = esp_timer_get_time();
      if (now - last_print_time >= kPrintIntervalUs) {
        PrintProgress("rs485 send", bytes_this_time, total_size, now,
            last_print_time);
        bytes_this_time = 0;
        last_print_time = now;
      }

      vTaskDelay(pdMS_TO_TICKS(10));
    }

    PrintResult("rs485 send", total_size, start_time);
  } else {
    SetRs485Transmit(false);
    vTaskDelay(pdMS_TO_TICKS(kRs485DirectionSettleMs));

    while (!g_exit_test) {
      size_t rx_len = 0;
      uart_get_buffered_data_len(kRs485UartPort, &rx_len);
      if (rx_len > 0) {
        std::unique_ptr<uint8_t[]> buffer(new uint8_t[rx_len]);
        const int read_len = uart_read_bytes(kRs485UartPort, buffer.get(),
            rx_len, pdMS_TO_TICKS(20));
        if (read_len > 0 &&
            ValidateData("rs485 receive", buffer.get(), read_len,
                kRs485TestChar)) {
          bytes_this_time += read_len;
          total_size += read_len;
        }
      }

      const int64_t now = esp_timer_get_time();
      if (now - last_print_time >= kPrintIntervalUs) {
        PrintProgress("rs485 receive", bytes_this_time, total_size, now,
            last_print_time);
        bytes_this_time = 0;
        last_print_time = now;
      }

      vTaskDelay(pdMS_TO_TICKS(10));
    }

    PrintResult("rs485 receive", total_size, start_time);
  }

  DeinitRs485();
  printf("[rs485] task completed\n");
  g_rs485_task_handle = nullptr;
  vTaskDelete(nullptr);
}

bool InitCan() {
  g_can_pending_alerts = 0;
  g_can_last_alert_print_us = 0;
  g_can_recovering = false;

  twai_general_config_t general_config = TWAI_GENERAL_CONFIG_DEFAULT(
      static_cast<gpio_num_t>(t_panel::gpio::can::kTx),
      static_cast<gpio_num_t>(t_panel::gpio::can::kRx), TWAI_MODE_NORMAL);
  twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  esp_err_t err =
      twai_driver_install(&general_config, &timing_config, &filter_config);
  if (err != ESP_OK) {
    printf("Install TWAI driver failed: %s\n", esp_err_to_name(err));
    return false;
  }

  err = twai_start();
  if (err != ESP_OK) {
    printf("Start TWAI driver failed: %s\n", esp_err_to_name(err));
    twai_driver_uninstall();
    return false;
  }

  err = twai_reconfigure_alerts(kCanAlertFlags, nullptr);
  if (err != ESP_OK) {
    printf("Configure TWAI alerts failed: %s\n", esp_err_to_name(err));
    twai_stop();
    twai_driver_uninstall();
    return false;
  }

  return true;
}

bool DeinitCan() {
  twai_status_info_t status = {};
  if (twai_get_status_info(&status) != ESP_OK) {
    return true;
  }

  if (status.state == TWAI_STATE_RUNNING) {
    twai_stop();
  } else if (status.state == TWAI_STATE_RECOVERING) {
    const int64_t start_us = esp_timer_get_time();
    while (esp_timer_get_time() - start_us <
           kCanRecoveryExitWaitMs * 1000) {
      uint32_t alerts = 0;
      twai_read_alerts(&alerts, pdMS_TO_TICKS(50));
      if (alerts & TWAI_ALERT_BUS_RECOVERED) {
        break;
      }
      twai_get_status_info(&status);
      if (status.state == TWAI_STATE_STOPPED ||
          status.state == TWAI_STATE_BUS_OFF) {
        break;
      }
    }
    twai_get_status_info(&status);
    if (status.state == TWAI_STATE_STOPPED) {
      g_can_recovering = false;
    } else if (status.state == TWAI_STATE_RECOVERING) {
      printf("Stop CAN failed: recovery is still running\n");
      return false;
    }
  }

  esp_err_t err = twai_driver_uninstall();
  if (err != ESP_OK) {
    printf("Uninstall TWAI driver failed: %s\n", esp_err_to_name(err));
    return false;
  }
  return true;
}

void HandleCanAlerts(uint32_t alerts, bool force_print = false) {
  if (alerts == 0) {
    return;
  }

  g_can_pending_alerts |= alerts;
  const int64_t now = esp_timer_get_time();
  if (!force_print && now - g_can_last_alert_print_us < 1000000) {
    return;
  }
  alerts = g_can_pending_alerts;
  g_can_pending_alerts = 0;
  g_can_last_alert_print_us = now;

  twai_status_info_t status = {};
  twai_get_status_info(&status);

  if (alerts & TWAI_ALERT_ERR_PASS) {
    printf("CAN alert: controller error passive\n");
  }
  if (alerts & TWAI_ALERT_BUS_ERROR) {
    printf("CAN alert: bus error count=%lu\n",
        static_cast<unsigned long>(status.bus_error_count));
  }
  if (alerts & TWAI_ALERT_TX_FAILED) {
    printf("CAN alert: TX failed tx_error=%lu failed=%lu\n",
        static_cast<unsigned long>(status.tx_error_counter),
        static_cast<unsigned long>(status.tx_failed_count));
  }
  if (alerts & TWAI_ALERT_RX_QUEUE_FULL) {
    printf("CAN alert: RX queue full missed=%lu overrun=%lu\n",
        static_cast<unsigned long>(status.rx_missed_count),
        static_cast<unsigned long>(status.rx_overrun_count));
  }
  if (alerts & TWAI_ALERT_BUS_OFF) {
    printf("CAN alert: bus off, start recovery\n");
    if (status.state == TWAI_STATE_BUS_OFF && !g_can_recovering) {
      if (twai_initiate_recovery() == ESP_OK) {
        g_can_recovering = true;
      }
    }
  }
  if (alerts & TWAI_ALERT_BUS_RECOVERED) {
    printf("CAN alert: bus recovered\n");
    g_can_recovering = false;
    if (twai_start() != ESP_OK) {
      printf("CAN restart after recovery failed\n");
    }
  }
}

void CanTask(void* param) {
  const bool is_send = (param != nullptr);
  printf("can_test_task (%s) start\n", is_send ? "send" : "receive");

  if (!InitCan()) {
    g_can_task_handle = nullptr;
    vTaskDelete(nullptr);
    return;
  }

  size_t total_size = 0;
  size_t bytes_this_time = 0;
  int64_t start_time = esp_timer_get_time();
  int64_t last_print_time = start_time;

  if (is_send) {
    twai_message_t message = {};
    message.identifier = kCanTestId;
    message.data_length_code = kCanDataLength;
    for (int i = 0; i < kCanDataLength; ++i) {
      message.data[i] = kCanTestChar;
    }

    while (!g_exit_test) {
      uint32_t alerts = 0;
      twai_read_alerts(&alerts, 0);
      HandleCanAlerts(alerts);

      twai_status_info_t status = {};
      twai_get_status_info(&status);
      if (status.state == TWAI_STATE_RUNNING) {
        const esp_err_t err =
            twai_transmit(&message, pdMS_TO_TICKS(kCanTxWaitMs));
        if (err == ESP_OK) {
          bytes_this_time += kCanDataLength;
          total_size += kCanDataLength;
        }
      }

      const int64_t now = esp_timer_get_time();
      if (now - last_print_time >= kPrintIntervalUs) {
        PrintProgress("can send", bytes_this_time, total_size, now,
            last_print_time);
        bytes_this_time = 0;
        last_print_time = now;
      }

      vTaskDelay(pdMS_TO_TICKS(1));
    }

    PrintResult("can send", total_size, start_time);
  } else {
    while (!g_exit_test) {
      uint32_t alerts = 0;
      twai_read_alerts(&alerts, pdMS_TO_TICKS(kCanPollMs));
      HandleCanAlerts(alerts);

      twai_message_t rx_message = {};
      while (twai_receive(&rx_message, 0) == ESP_OK) {
        if (!rx_message.rtr && !rx_message.extd &&
            rx_message.identifier == kCanTestId &&
            rx_message.data_length_code == kCanDataLength &&
            ValidateData("can receive", rx_message.data,
                rx_message.data_length_code, kCanTestChar)) {
          bytes_this_time += rx_message.data_length_code;
          total_size += rx_message.data_length_code;
        }
      }

      const int64_t now = esp_timer_get_time();
      if (now - last_print_time >= kPrintIntervalUs) {
        PrintProgress("can receive", bytes_this_time, total_size, now,
            last_print_time);
        bytes_this_time = 0;
        last_print_time = now;
      }
    }

    PrintResult("can receive", total_size, start_time);
  }

  while (!DeinitCan()) {
    vTaskDelay(pdMS_TO_TICKS(500));
  }
  printf("[can] task completed\n");
  g_can_task_handle = nullptr;
  vTaskDelete(nullptr);
}

std::string TrimCommandToken(std::string token) {
  while (!token.empty() &&
         (token.back() == '\r' || token.back() == '\n' || token.back() == ' ')) {
    token.pop_back();
  }
  while (!token.empty() && token.front() == ' ') {
    token.erase(token.begin());
  }
  return token;
}

void StopCurrentTask() {
  if (g_rs485_task_handle == nullptr && g_can_task_handle == nullptr) {
    g_exit_test = false;
    return;
  }

  g_exit_test = true;
  for (int i = 0; i < 10; ++i) {
    if (g_rs485_task_handle == nullptr && g_can_task_handle == nullptr) {
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  if (g_rs485_task_handle != nullptr || g_can_task_handle != nullptr) {
    printf("Stop task timeout\n");
  }
  g_exit_test = false;
}

bool StartRs485Task(bool is_send) {
  printf("Start RS485 %s command\n", is_send ? "send" : "receive");
  StopCurrentTask();
  if (g_rs485_task_handle != nullptr || g_can_task_handle != nullptr) {
    printf("Start RS485 failed: previous task is still running\n");
    return false;
  }

  g_exit_test = false;
  if (xTaskCreate(Rs485Task, "rs485_task", 1024 * 6,
          is_send ? reinterpret_cast<void*>(1) : nullptr, 5,
          &g_rs485_task_handle) != pdPASS) {
    g_rs485_task_handle = nullptr;
    printf("Create RS485 task failed\n");
    return false;
  }
  return true;
}

bool StartCanTask(bool is_send) {
  printf("Start CAN %s command\n", is_send ? "send" : "receive");
  StopCurrentTask();
  if (g_rs485_task_handle != nullptr || g_can_task_handle != nullptr) {
    printf("Start CAN failed: previous task is still running\n");
    return false;
  }

  g_exit_test = false;
  if (xTaskCreate(CanTask, "can_task", 1024 * 6,
          is_send ? reinterpret_cast<void*>(1) : nullptr, 5,
          &g_can_task_handle) != pdPASS) {
    g_can_task_handle = nullptr;
    printf("Create CAN task failed\n");
    return false;
  }
  return true;
}

void PrintCommandHelp() {
  printf("operation command:\n");
  printf("[rs485:send:]\n");
  printf("[rs485:receive:]\n");
  printf("[can:send:]\n");
  printf("[can:receive:]\n");
  printf("[all_exit:]\n");
}

bool ParseCommand(const std::vector<std::string>& cmd) {
  if (cmd.empty()) {
    printf("parse_cmd fail (cmd is empty)\n");
    return false;
  }
  if (cmd[0] == "all_exit") {
    StopCurrentTask();
    printf("all exit test\n");
    return true;
  }
  if (cmd.size() <= 1) {
    printf("parse_cmd fail (missing mode)\n");
    PrintCommandHelp();
    return false;
  }

  if (cmd[0] == "rs485") {
    if (cmd[1] == "send") {
      return StartRs485Task(true);
    }
    if (cmd[1] == "receive") {
      return StartRs485Task(false);
    }
  } else if (cmd[0] == "can") {
    if (cmd[1] == "send") {
      return StartCanTask(true);
    }
    if (cmd[1] == "receive") {
      return StartCanTask(false);
    }
  }

  printf("parse_cmd fail (unknown command)\n");
  PrintCommandHelp();
  return false;
}

void ParseCommandLine(const std::string& content) {
  std::stringstream stream(content);
  std::string token;
  std::vector<std::string> parts;
  while (std::getline(stream, token, ':')) {
    token = TrimCommandToken(token);
    if (!token.empty()) {
      parts.push_back(token);
    }
  }
  ParseCommand(parts);
}

bool IsCompleteCommand(const std::string& content) {
  const std::string command = TrimCommandToken(content);
  return command == "all_exit:" || command == "rs485:send:" ||
         command == "rs485:receive:" || command == "can:send:" ||
         command == "can:receive:";
}

void AppendConsoleInput(const std::string& content) {
  g_console_buffer += content;

  size_t line_end = std::string::npos;
  while ((line_end = g_console_buffer.find_first_of("\r\n")) !=
         std::string::npos) {
    const std::string line = g_console_buffer.substr(0, line_end);
    g_console_buffer.erase(0, line_end + 1);
    if (!TrimCommandToken(line).empty()) {
      ParseCommandLine(line);
    }
  }

  if (IsCompleteCommand(g_console_buffer)) {
    ParseCommandLine(g_console_buffer);
    g_console_buffer.clear();
  }
}

}  // namespace

extern "C" void app_main(void) {
  printf("Ciallo\n");
  usb_serial_jtag_driver_config_t usb_serial_jtag_config =
      USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
  usb_serial_jtag_config.rx_buffer_size = 1024 * 2;
  usb_serial_jtag_config.tx_buffer_size = 1024 * 2;
  esp_err_t err = usb_serial_jtag_driver_install(&usb_serial_jtag_config);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    printf("Install USB Serial/JTAG driver failed: %s\n",
        esp_err_to_name(err));
    return;
  }

  PrintCommandHelp();

  while (true) {
    uint8_t buffer[128] = {};
    const int read_len = usb_serial_jtag_read_bytes(buffer, sizeof(buffer),
        pdMS_TO_TICKS(20));
    if (read_len > 0) {
      AppendConsoleInput(
          std::string(reinterpret_cast<char*>(buffer), read_len));
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
