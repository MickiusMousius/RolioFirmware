/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * @file wpm_chart.c
 * @brief Implementation of the Words Per Minute (WPM) chart view for the multi-widget.
 * @details Uses the WPM history array stored in the multi-widget state to draw a historical line graph or bar chart of
 * typing speed on the LVGL canvas.
 */

/* ========================================================================= */
/*                        INCLUDES AND DEPENDENCIES                          */
/* ========================================================================= */
#include "../../status_screen.h"
#include "../../styles.h"
#include "../../util.h"
#include "../multi_widget.h"
#include "views.h"
#include <stdio.h>

#include <zmk/event_manager.h>
#include <zmk/events/wpm_state_changed.h>

/* ========================================================================= */
/*                            STATE AND GLOBALS                              */
/* ========================================================================= */
static uint8_t wpm_history[WPM_SAMPLES] = {0};

/* ========================================================================= */
/*                         UI RENDERING AND DRAWING                          */
/* ========================================================================= */
void draw_wpm_chart(lv_obj_t *canvas) {
    const uint8_t yOffset = 0;
    const int WPM_HEIGHT = 82;

    char wpm_text[6] = {};
    snprintf(wpm_text, sizeof(wpm_text), "%d", wpm_history[WPM_SAMPLES - 1]);
    lv_canvas_draw_text(canvas, DISPLAY_WIDTH - 44, 3 + WPM_HEIGHT - 21, 40, &style_label_16_right, wpm_text);

    int max = 0;
    int min = 256;

    for (int i = 0; i < WPM_SAMPLES; i++) {
        if (wpm_history[i] > max) {
            max = wpm_history[i];
        }
        if (wpm_history[i] < min) {
            min = wpm_history[i];
        }
    }

    int range = max - min;
    if (range == 0) {
        range = 1;
    }

    const int WPM_COLUMN_WIDTH = (DISPLAY_WIDTH - 4) / WPM_SAMPLES;
    lv_point_t points[WPM_SAMPLES];
    for (int i = 0; i < WPM_SAMPLES; i++) {
        points[i].x = 3 + i * WPM_COLUMN_WIDTH;
        points[i].y = yOffset - 1 + WPM_HEIGHT - (wpm_history[i] - min) * (WPM_HEIGHT - 7) / range;
    }
    lv_canvas_draw_line(canvas, points, WPM_SAMPLES, &style_line_1);
    for (int i = 0; i < WPM_SAMPLES; i++) {
        lv_canvas_set_px_color(canvas, points[i].x, points[i].y, LVGL_FOREGROUND);
    }
}


/* ========================================================================= */
/*                    EVENT LISTENERS AND STATE UPDATES                      */
/* ========================================================================= */
int wpm_chart_wpm_listener(const zmk_event_t *eh) {
    const struct zmk_wpm_state_changed *ev = as_zmk_wpm_state_changed(eh);
    if (ev) {
        for (int i = 0; i < WPM_SAMPLES - 1; i++) {
            wpm_history[i] = wpm_history[i + 1];
        }
        wpm_history[WPM_SAMPLES - 1] = ev->state;
        if (zmk_widget_multi_is_wpm_chart_active()) {
            zmk_widget_multi_status_update_async();
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}


ZMK_LISTENER(widget_wpm_chart_wpm, wpm_chart_wpm_listener);
ZMK_SUBSCRIPTION(widget_wpm_chart_wpm, zmk_wpm_state_changed);
