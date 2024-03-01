/*
* Copyright 2024 NXP
* NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#ifndef GUI_GUIDER_H
#define GUI_GUIDER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

typedef struct
{
  
	lv_obj_t *screen;
	bool screen_del;
	lv_obj_t *screen_label_1;
	lv_obj_t *screen_carousel_1;
	lv_obj_t *screen_carousel_1_element_1;
	lv_obj_t *screen_carousel_1_element_2;
	lv_obj_t *screen_carousel_1_element_3;
	lv_obj_t *screen_carousel_1_element_4;
	lv_obj_t *screen_img_1;
	lv_obj_t *screen_img_2;
	lv_obj_t *screen_img_3;
	lv_obj_t *screen_img_4;
}lv_ui;

void ui_init_style(lv_style_t * style);
void init_scr_del_flag(lv_ui *ui);
void setup_ui(lv_ui *ui);
extern lv_ui guider_ui;

void setup_scr_screen(lv_ui *ui);
LV_IMG_DECLARE(_1_alpha_240x400);
LV_IMG_DECLARE(_2_alpha_240x400);
LV_IMG_DECLARE(_3_alpha_240x400);
LV_IMG_DECLARE(_4_alpha_240x400);

LV_FONT_DECLARE(lv_font_arial_30)
LV_FONT_DECLARE(lv_font_montserratMedium_16)


#ifdef __cplusplus
}
#endif
#endif
