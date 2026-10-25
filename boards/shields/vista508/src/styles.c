/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * @file styles.c
 * @brief Definition and initialization of global LVGL display styles.
 * @details Declares and initializes global text and shape descriptors used across the Vista508 UI to minimize stack
 * overhead. These styles are registered during Zephyr system initialization.
 */

#include "styles.h"
#include "util.h"
#include <zephyr/init.h>

// Definitions
lv_draw_label_dsc_t style_label_16_left;
lv_draw_label_dsc_t style_label_16_right;
lv_draw_label_dsc_t style_label_16_center;
lv_draw_label_dsc_t style_label_18_center;
lv_draw_label_dsc_t style_label_18_center_bg;
lv_draw_label_dsc_t style_label_16_center_bg;
lv_draw_label_dsc_t style_label_18_left;

lv_draw_rect_dsc_t style_rect_fg;
lv_draw_rect_dsc_t style_rect_fg_square;
lv_draw_rect_dsc_t style_rect_bg;
lv_draw_rect_dsc_t style_rect_border;
lv_draw_line_dsc_t style_line_1;
lv_draw_line_dsc_t style_line_2;
lv_draw_arc_dsc_t style_arc_2;
lv_draw_arc_dsc_t style_arc_9;

void vista508_styles_init(void) {
    // Labels
    lv_draw_label_dsc_init(&style_label_16_left);
    style_label_16_left.color = LVGL_FOREGROUND;
    style_label_16_left.font = &lv_font_montserrat_16;
    style_label_16_left.align = LV_TEXT_ALIGN_LEFT;

    lv_draw_label_dsc_init(&style_label_16_right);
    style_label_16_right.color = LVGL_FOREGROUND;
    style_label_16_right.font = &lv_font_montserrat_16;
    style_label_16_right.align = LV_TEXT_ALIGN_RIGHT;

    lv_draw_label_dsc_init(&style_label_18_center);
    style_label_18_center.color = LVGL_FOREGROUND;
    style_label_18_center.font = &lv_font_montserrat_18;
    style_label_18_center.align = LV_TEXT_ALIGN_CENTER;

    lv_draw_label_dsc_init(&style_label_18_center_bg);
    style_label_18_center_bg.color = LVGL_BACKGROUND;
    style_label_18_center_bg.font = &lv_font_montserrat_18;
    style_label_18_center_bg.align = LV_TEXT_ALIGN_CENTER;

    lv_draw_label_dsc_init(&style_label_16_center);
    style_label_16_center.color = LVGL_FOREGROUND;
    style_label_16_center.font = &lv_font_montserrat_16;
    style_label_16_center.align = LV_TEXT_ALIGN_CENTER;

    lv_draw_label_dsc_init(&style_label_16_center_bg);
    style_label_16_center_bg.color = LVGL_BACKGROUND;
    style_label_16_center_bg.font = &lv_font_montserrat_16;
    style_label_16_center_bg.align = LV_TEXT_ALIGN_CENTER;

    lv_draw_label_dsc_init(&style_label_18_left);
    style_label_18_left.color = LVGL_FOREGROUND;
    style_label_18_left.font = &lv_font_montserrat_18;
    style_label_18_left.align = LV_TEXT_ALIGN_LEFT;

    // Shapes
    lv_draw_rect_dsc_init(&style_rect_fg);
    style_rect_fg.bg_color = LVGL_FOREGROUND;
    style_rect_fg.radius = 4;

    lv_draw_rect_dsc_init(&style_rect_fg_square);
    style_rect_fg_square.bg_color = LVGL_FOREGROUND;
    style_rect_fg_square.radius = 0;

    lv_draw_rect_dsc_init(&style_rect_bg);
    style_rect_bg.bg_color = LVGL_BACKGROUND;

    lv_draw_rect_dsc_init(&style_rect_border);
    style_rect_border.bg_opa = LV_OPA_TRANSP;
    style_rect_border.border_color = LVGL_FOREGROUND;
    style_rect_border.border_width = 2;
    style_rect_border.radius = 4;

    lv_draw_line_dsc_init(&style_line_1);
    style_line_1.color = LVGL_FOREGROUND;
    style_line_1.width = 1;

    lv_draw_line_dsc_init(&style_line_2);
    style_line_2.color = LVGL_FOREGROUND;
    style_line_2.width = 2;

    lv_draw_arc_dsc_init(&style_arc_2);
    style_arc_2.color = LVGL_FOREGROUND;
    style_arc_2.width = 2;

    lv_draw_arc_dsc_init(&style_arc_9);
    style_arc_9.color = LVGL_FOREGROUND;
    style_arc_9.width = 9;
}


static int styles_init_cb(void) {
    vista508_styles_init();
    return 0;
}


SYS_INIT(styles_init_cb, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
