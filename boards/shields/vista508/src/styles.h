/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * @file styles.h
 * @brief Global style declarations for the Vista508 LVGL UI.
 * @details Provides extern declarations for all LVGL label, rect, line, and arc styles used by the UI widgets to
 * optimize rendering.
 */

#pragma once

#include <lvgl.h>

// Labels
extern lv_draw_label_dsc_t style_label_16_left;
extern lv_draw_label_dsc_t style_label_16_right;
extern lv_draw_label_dsc_t style_label_16_center;
extern lv_draw_label_dsc_t style_label_18_center;
extern lv_draw_label_dsc_t style_label_18_center_bg;
extern lv_draw_label_dsc_t style_label_16_center_bg;
extern lv_draw_label_dsc_t style_label_18_left;

// Shapes
extern lv_draw_rect_dsc_t style_rect_fg;
extern lv_draw_rect_dsc_t style_rect_fg_square;
extern lv_draw_rect_dsc_t style_rect_bg;
extern lv_draw_rect_dsc_t style_rect_border;
extern lv_draw_line_dsc_t style_line_1;
extern lv_draw_line_dsc_t style_line_2;
extern lv_draw_arc_dsc_t style_arc_2;
extern lv_draw_arc_dsc_t style_arc_9;

// Init function
void vista508_styles_init(void);
