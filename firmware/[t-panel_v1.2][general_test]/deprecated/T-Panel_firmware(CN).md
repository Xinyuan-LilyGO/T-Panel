<!--
 * @Description: None
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2023-09-12 18:12:36
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2024-04-12 15:50:10
 * @License: GPL 3.0
-->
<h1 align = "center">T-Panel 初始测试 firmware 说明</h1>

## 适用版本：[T-Panel_V1.0]、[T-Panel_V1.1]
### <br />更改日志[ESP32S3]:
#### T-Panel_Test_ESP32S3_firmware_V1.0.0: 初始版本
#### T-Panel_Test_ESP32S3_firmware_V1.0.1:
    1.修复了整板重新上电后ESP32H2需要手动复位才有返回信息的问题
    2.修复了点击“Please touch me!”后进入主界面，屏幕出现大量ESP32H2初始化信息的问题
#### T-Panel_Test_ESP32S3_firmware_V1.0.2:
    1.新增屏幕边缘测试
#### T-Panel_Test_ESP32S3_firmware_V1.0.3:
    1.修复屏幕边缘测试部分时序不对检测失败的问题
#### T-Panel_Test_ESP32S3_firmware_V1.0.4:
    1.调整屏幕背光PWM为1Khz

### <br />更改日志[ESP32H2]:
#### T-Panel_Test_ESP32H2_firmware_V1.0.0: 初始版本
#### T-Panel_Test_ESP32H2_firmware_V1.0.1:
    1.修复了ESP32H2返回计数超过100后返回乱码的问题

## <br />适用版本：[T-Panel_V1.2]

### <br />更改日志[ESP32S3]:

#### [ESP32S3][T-Panel_V1.2][T-Panel_Test_V1_2]_firmware_V1.0.0(RS485): 初始版本
#### [ESP32S3][T-Panel_V1.2][T-Panel_Test_V1_2]_firmware_V1.0.1(RS485): 新增屏幕旋转功能 进入主循环后按ESP32S3的BOOT按键即可实现屏幕旋转

#### <br />[ESP32S3][T-Panel_V1.2][T-Panel_Test_V1_2]_firmware_V1.0.0(CAN): 初始版本
#### [ESP32S3][T-Panel_V1.2][T-Panel_Test_V1_2]_firmware_V1.0.1(CAN): 新增屏幕旋转功能 进入主循环后按ESP32S3的BOOT按键即可实现屏幕旋转


### <br />更改日志[ESP32H2]:
#### [ESP32H2][T-Panel_V1.2][ESP32H2_Test]_firmware_V1.0.2: 初始版本

