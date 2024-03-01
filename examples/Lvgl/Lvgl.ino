/*
 * @Description: LVGL
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2023-09-22 11:59:37
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2024-03-01 11:16:00
 * @License: GPL 3.0
 */
// #define TOUCH_MODULES_GT911
// #define TOUCH_MODULES_CST_SELF
#define TOUCH_MODULES_CST_MUTUAL
// #define TOUCH_MODULES_ZTW622
// #define TOUCH_MODULES_L58
// #define TOUCH_MODULES_FT3267
// #define TOUCH_MODULES_FT5x06

#include "lvgl.h"
#include "Arduino_GFX_Library.h"
#include "pin_config.h"
#include "TouchLib.h"
#include "gui_guider.h"
#include "events_init.h"
#include "custom.h"

static bool Touch_Int_Flag = false;

lv_ui guider_ui;

TouchLib touch(Wire, TOUCH_SDA, TOUCH_SCL, CST3240_ADDRESS);

Arduino_DataBus *bus = new Arduino_XL9535SWSPI(IIC_SDA /* SDA */, IIC_SCL /* SCL */, -1 /* XL PWD */,
                                               XL95X5_CS /* XL CS */, XL95X5_SCLK /* XL SCK */, XL95X5_MOSI /* XL MOSI */);
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    -1 /* DE */, LCD_VSYNC /* VSYNC */, LCD_HSYNC /* HSYNC */, LCD_PCLK /* PCLK */,
    LCD_B0 /* B0 */, LCD_B1 /* B1 */, LCD_B2 /* B2 */, LCD_B3 /* B3 */, LCD_B4 /* B4 */,
    LCD_G0 /* G0 */, LCD_G1 /* G1 */, LCD_G2 /* G2 */, LCD_G3 /* G3 */, LCD_G4 /* G4 */, LCD_G5 /* G5 */,
    LCD_R0 /* R0 */, LCD_R1 /* R1 */, LCD_R2 /* R2 */, LCD_R3 /* R3 */, LCD_R4 /* R4 */,
    1 /* hsync_polarity */, 20 /* hsync_front_porch */, 2 /* hsync_pulse_width */, 0 /* hsync_back_porch */,
    1 /* vsync_polarity */, 30 /* vsync_front_porch */, 8 /* vsync_pulse_width */, 1 /* vsync_back_porch */,
    10 /* pclk_active_neg */, 6000000L /* prefer_speed */, false /* useBigEndian */,
    0 /* de_idle_high*/, 0 /* pclk_idle_high */);
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    LCD_WIDTH /* width */, LCD_HEIGHT /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
    bus, -1 /* RST */, st7701_type9_init_operations, sizeof(st7701_type9_init_operations));

static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
    gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

    lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    if (Touch_Int_Flag == true)
    {
        Touch_Int_Flag = false;

        touch.read();
        if (touch.getPointNum() > 0)
        {
            TP_Point t = touch.getPoint(0);

            data->state = LV_INDEV_STATE_PR;

            /*Set the coordinates*/
            data->point.x = t.x;
            data->point.y = t.y;

            // Serial.print("Data x ");
            // Serial.printf("%d\n", x[0]);

            // Serial.print("Data y ");
            // Serial.printf("%d\n", y[0]);
        }
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}

void lvgl_initialization(void)
{
    lv_init();

    uint32_t screen_width = gfx->width();
    uint32_t screen_height = gfx->height();

    // 使用PSRAM分配内存
    lv_color_t *buf_1 = (lv_color_t *)heap_caps_malloc(48 * 1024, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    lv_color_t *buf_2 = (lv_color_t *)heap_caps_malloc(48 * 1024, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    while ((!buf_1) || (!buf_2))
    {
        Serial.println("LVGL disp_draw_buf allocate failed!");
        delay(1000);
    }

    lv_disp_draw_buf_init(&draw_buf, buf_1, buf_2, 48 * 1024 * 2);

    /* Initialize the display */
    lv_disp_drv_init(&disp_drv);
    /* Change the following line to your display resolution */
    disp_drv.hor_res = screen_width;
    disp_drv.ver_res = screen_height;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.full_refresh = 1; // 双缓冲全像素刷新
    lv_disp_drv_register(&disp_drv);

    /*Initialize the (dummy) input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Ciallo");

    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, HIGH);

    attachInterrupt(
        TOUCH_INT,
        []
        {
            Touch_Int_Flag = true;
            Serial.println("get_int");
        },
        FALLING); // Triggered every 1ms

    Wire.begin(IIC_SDA, IIC_SCL);

    gfx->begin();
    gfx->fillScreen(BLACK);

    gfx->XL_digitalWrite(TOUCH_RST, LOW);
    delay(200);
    gfx->XL_digitalWrite(TOUCH_RST, HIGH);
    delay(200);

    touch.init();
    
    lvgl_initialization();

    setup_ui(&guider_ui);
    events_init(&guider_ui);
}

void loop()
{
    lv_timer_handler(); /* let the GUI do its work */
    delay(5);
}