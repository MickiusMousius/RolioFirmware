/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */

#include <lvgl.h>
#include <zmk/endpoints.h>

#define CANVAS_SIZE 144

#define WPM_SAMPLES 20

#define LVGL_BACKGROUND                                                                            \
    IS_ENABLED(CONFIG_VISTA508_WIDGET_INVERTED) ? lv_color_black() : lv_color_white()
#define LVGL_FOREGROUND                                                                            \
    IS_ENABLED(CONFIG_VISTA508_WIDGET_INVERTED) ? lv_color_white() : lv_color_black()

struct battery_info {
    uint8_t source;
    uint8_t level;
    bool usb_present;
};

struct status_state {
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
    uint8_t layer_index;
    const char *layer_label;
    uint8_t wpm[WPM_SAMPLES];
    struct battery_info batteries[2];
#else
    bool connected;
#endif
};

void rotate_canvas(lv_obj_t *canvas, lv_color_t cbuf[]);
// void draw_battery(lv_obj_t *canvas, const struct status_state *state);
void init_label_dsc(lv_draw_label_dsc_t *label_dsc, lv_color_t color, const lv_font_t *font,
                    lv_text_align_t align);
void init_rect_dsc(lv_draw_rect_dsc_t *rect_dsc, lv_color_t bg_color);
void init_line_dsc(lv_draw_line_dsc_t *line_dsc, lv_color_t color, uint8_t width);
void init_arc_dsc(lv_draw_arc_dsc_t *arc_dsc, lv_color_t color, uint8_t width);
