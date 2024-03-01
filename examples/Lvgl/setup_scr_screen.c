/*
* Copyright 2024 NXP
* NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"


void setup_scr_screen(lv_ui *ui)
{
	//Write codes screen
	ui->screen = lv_obj_create(NULL);
	lv_obj_set_size(ui->screen, 480, 480);
	lv_obj_set_scrollbar_mode(ui->screen, LV_SCROLLBAR_MODE_OFF);

	//Write style for screen, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->screen, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_1
	ui->screen_label_1 = lv_label_create(ui->screen);
	lv_label_set_text(ui->screen_label_1, "LVGL");
	lv_label_set_long_mode(ui->screen_label_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->screen_label_1, 167, 13);
	lv_obj_set_size(ui->screen_label_1, 146, 33);
	lv_obj_set_scrollbar_mode(ui->screen_label_1, LV_SCROLLBAR_MODE_OFF);

	//Write style for screen_label_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->screen_label_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->screen_label_1, &lv_font_arial_30, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->screen_label_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->screen_label_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_carousel_1
	ui->screen_carousel_1 = lv_carousel_create(ui->screen);
	lv_carousel_set_element_width(ui->screen_carousel_1, 300);
	lv_obj_set_size(ui->screen_carousel_1, 480, 430);
	ui->screen_carousel_1_element_1 = lv_carousel_add_element(ui->screen_carousel_1, 0);
	ui->screen_carousel_1_element_2 = lv_carousel_add_element(ui->screen_carousel_1, 1);
	ui->screen_carousel_1_element_3 = lv_carousel_add_element(ui->screen_carousel_1, 2);
	ui->screen_carousel_1_element_4 = lv_carousel_add_element(ui->screen_carousel_1, 3);
	lv_obj_set_pos(ui->screen_carousel_1, 0, 50);
	lv_obj_set_size(ui->screen_carousel_1, 480, 430);
	lv_obj_set_scrollbar_mode(ui->screen_carousel_1, LV_SCROLLBAR_MODE_OFF);

	//Write style for screen_carousel_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_carousel_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->screen_carousel_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_carousel_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->screen_carousel_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for screen_carousel_1, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_carousel_1, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_carousel_1, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

	//Write style state: LV_STATE_DEFAULT for &style_screen_carousel_1_extra_element_items_default
	static lv_style_t style_screen_carousel_1_extra_element_items_default;
	ui_init_style(&style_screen_carousel_1_extra_element_items_default);
	
	lv_style_set_bg_opa(&style_screen_carousel_1_extra_element_items_default, 0);
	lv_style_set_outline_width(&style_screen_carousel_1_extra_element_items_default, 0);
	lv_style_set_border_width(&style_screen_carousel_1_extra_element_items_default, 0);
	lv_style_set_radius(&style_screen_carousel_1_extra_element_items_default, 5);
	lv_style_set_shadow_width(&style_screen_carousel_1_extra_element_items_default, 0);
	lv_obj_add_style(ui->screen_carousel_1_element_4, &style_screen_carousel_1_extra_element_items_default, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_style(ui->screen_carousel_1_element_3, &style_screen_carousel_1_extra_element_items_default, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_style(ui->screen_carousel_1_element_2, &style_screen_carousel_1_extra_element_items_default, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_add_style(ui->screen_carousel_1_element_1, &style_screen_carousel_1_extra_element_items_default, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style state: LV_STATE_FOCUSED for &style_screen_carousel_1_extra_element_items_focused
	static lv_style_t style_screen_carousel_1_extra_element_items_focused;
	ui_init_style(&style_screen_carousel_1_extra_element_items_focused);
	
	lv_style_set_bg_opa(&style_screen_carousel_1_extra_element_items_focused, 0);
	lv_style_set_outline_width(&style_screen_carousel_1_extra_element_items_focused, 0);
	lv_style_set_border_width(&style_screen_carousel_1_extra_element_items_focused, 0);
	lv_style_set_radius(&style_screen_carousel_1_extra_element_items_focused, 5);
	lv_style_set_shadow_width(&style_screen_carousel_1_extra_element_items_focused, 0);
	lv_obj_add_style(ui->screen_carousel_1_element_4, &style_screen_carousel_1_extra_element_items_focused, LV_PART_MAIN|LV_STATE_FOCUSED);
	lv_obj_add_style(ui->screen_carousel_1_element_3, &style_screen_carousel_1_extra_element_items_focused, LV_PART_MAIN|LV_STATE_FOCUSED);
	lv_obj_add_style(ui->screen_carousel_1_element_2, &style_screen_carousel_1_extra_element_items_focused, LV_PART_MAIN|LV_STATE_FOCUSED);
	lv_obj_add_style(ui->screen_carousel_1_element_1, &style_screen_carousel_1_extra_element_items_focused, LV_PART_MAIN|LV_STATE_FOCUSED);



	//Write codes screen_img_1
	ui->screen_img_1 = lv_img_create(ui->screen_carousel_1_element_1);
	lv_obj_add_flag(ui->screen_img_1, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->screen_img_1, &_1_alpha_240x400);
	lv_img_set_pivot(ui->screen_img_1, 50,50);
	lv_img_set_angle(ui->screen_img_1, 0);
	lv_obj_set_pos(ui->screen_img_1, 30, 1);
	lv_obj_set_size(ui->screen_img_1, 240, 400);
	lv_obj_set_scrollbar_mode(ui->screen_img_1, LV_SCROLLBAR_MODE_OFF);

	//Write style for screen_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->screen_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);



	//Write codes screen_img_2
	ui->screen_img_2 = lv_img_create(ui->screen_carousel_1_element_2);
	lv_obj_add_flag(ui->screen_img_2, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->screen_img_2, &_2_alpha_240x400);
	lv_img_set_pivot(ui->screen_img_2, 50,50);
	lv_img_set_angle(ui->screen_img_2, 0);
	lv_obj_set_pos(ui->screen_img_2, 30, 1);
	lv_obj_set_size(ui->screen_img_2, 240, 400);
	lv_obj_set_scrollbar_mode(ui->screen_img_2, LV_SCROLLBAR_MODE_OFF);

	//Write style for screen_img_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->screen_img_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);



	//Write codes screen_img_3
	ui->screen_img_3 = lv_img_create(ui->screen_carousel_1_element_3);
	lv_obj_add_flag(ui->screen_img_3, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->screen_img_3, &_3_alpha_240x400);
	lv_img_set_pivot(ui->screen_img_3, 50,50);
	lv_img_set_angle(ui->screen_img_3, 0);
	lv_obj_set_pos(ui->screen_img_3, 30, 1);
	lv_obj_set_size(ui->screen_img_3, 240, 400);
	lv_obj_set_scrollbar_mode(ui->screen_img_3, LV_SCROLLBAR_MODE_OFF);

	//Write style for screen_img_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->screen_img_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);



	//Write codes screen_img_4
	ui->screen_img_4 = lv_img_create(ui->screen_carousel_1_element_4);
	lv_obj_add_flag(ui->screen_img_4, LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_src(ui->screen_img_4, &_4_alpha_240x400);
	lv_img_set_pivot(ui->screen_img_4, 50,50);
	lv_img_set_angle(ui->screen_img_4, 0);
	lv_obj_set_pos(ui->screen_img_4, 30, 1);
	lv_obj_set_size(ui->screen_img_4, 240, 400);
	lv_obj_set_scrollbar_mode(ui->screen_img_4, LV_SCROLLBAR_MODE_OFF);

	//Write style for screen_img_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_img_opa(ui->screen_img_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Update current screen layout.
	lv_obj_update_layout(ui->screen);

	
}
