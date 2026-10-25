/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * @file settings_notices.c
 * @brief Implementation of the sleep image & settings layer notifications for the multi-widget.
 * @details Draws the sleeping image while the board is idle, and short text notifications confirming display setting
 * changes made from the settings layer.
 */

/* ========================================================================= */
/*                        INCLUDES AND DEPENDENCIES                          */
/* ========================================================================= */
#include "../../status_screen.h"
#include "../../styles.h"
#include "../../util.h"
#include "views.h"

/* ========================================================================= */
/*                            STATE AND GLOBALS                              */
/* ========================================================================= */
extern const lv_img_dsc_t *sleep_frames[];

/* ========================================================================= */
/*                         UI RENDERING AND DRAWING                          */
/* ========================================================================= */
void draw_sleep_image(lv_obj_t *canvas) {
    const uint8_t yOffset = 0;
    const int WIDGET_HEIGHT = 82;
    lv_draw_img_dsc_t img_dsc;
    lv_draw_img_dsc_init(&img_dsc);

    const lv_img_dsc_t *frame = sleep_frames[0];
    int x_offset = (DISPLAY_WIDTH - frame->header.w) / 2;
    int y_offset = yOffset + (WIDGET_HEIGHT - frame->header.h) / 2 + 1;

    lv_canvas_draw_img(canvas, x_offset, y_offset, frame, &img_dsc);
}


void draw_wpm_text_notification(lv_obj_t *canvas, int display_mode) {
    const uint8_t yOffset = 0;
    if (display_mode == 1) { // MULTI_WIDGET_MODE_WPM_GRAPH
        lv_canvas_draw_text(canvas, 0, yOffset + 20, DISPLAY_WIDTH, &style_label_18_center, "Show\nWPM Graph");
    } else if (display_mode == 2) { // MULTI_WIDGET_MODE_MODIFIERS
        lv_canvas_draw_text(canvas, 0, yOffset + 20, DISPLAY_WIDTH, &style_label_18_center, "Show\nModifiers");
    } else {
        lv_canvas_draw_text(canvas, 0, yOffset + 20, DISPLAY_WIDTH, &style_label_18_center, "Show\nWPM Steve");
    }
}


void draw_mod_auto_text_notification(lv_obj_t *canvas, bool enabled) {
    const uint8_t yOffset = 0;
    if (enabled) {
        lv_canvas_draw_text(canvas, 0, yOffset + 20, DISPLAY_WIDTH, &style_label_18_center, "Auto Mod\nON");
    } else {
        lv_canvas_draw_text(canvas, 0, yOffset + 20, DISPLAY_WIDTH, &style_label_18_center, "Auto Mod\nOFF");
    }
}
