/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * @file output.c
 * @brief Implementation of the output status widget, showing the current Bluetooth profile and connection state.
 * @details Listens to ZMK endpoint and BLE active profile events. It renders the active connection (USB or Bluetooth)
 * and indicates whether the active Bluetooth profile is connected, advertising, or disconnected.
 */

/* ========================================================================= */
/*                        INCLUDES AND DEPENDENCIES                          */
/* ========================================================================= */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include "../status_screen.h"
#include "../styles.h"
#include "output.h"
#include <zmk/ble.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/usb.h>

/* ========================================================================= */
/*                            STATE AND GLOBALS                              */
/* ========================================================================= */
LV_IMG_DECLARE(gear_icon);
LV_IMG_DECLARE(disconnected_icon);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct output_status_state {
    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
};


/* ========================================================================= */
/*                         UI RENDERING AND DRAWING                          */
/* ========================================================================= */
static void draw_output_info(struct zmk_widget_output_status *widget) {
    lv_obj_t *canvas = widget->canvas;

    // Fill background
    lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);

    bool usingUsb = false;
    bool profileConnected = false;
    bool profileAdvertising = false;
    if (widget->selected_endpoint.transport == ZMK_TRANSPORT_USB) {
        usingUsb = true;
    }
    if (widget->active_profile_bonded) {
        if (widget->active_profile_connected) {
            profileConnected = true;
        }
    } else {
        profileAdvertising = true;
    }

    if (usingUsb) {
        // Draw the pill outline
        lv_canvas_draw_arc(canvas, 15, 14, 13, 90, 270, &style_arc_2);
        lv_canvas_draw_arc(canvas, 95, 14, 13, 270, 90, &style_arc_2);
        lv_point_t points[2];
        points[0].x = 15;
        points[0].y = 2;
        points[1].x = 95;
        points[1].y = 2;
        lv_canvas_draw_line(canvas, points, 2, &style_line_2);
        points[0].y = 26;
        points[1].y = 26;
        lv_canvas_draw_line(canvas, points, 2, &style_line_2);
        // Draw the pill fill
        lv_canvas_draw_arc(canvas, 15, 14, 9, 0, 359, &style_arc_9);
        lv_canvas_draw_arc(canvas, 95, 14, 9, 0, 359, &style_arc_9);
        lv_canvas_draw_rect(canvas, 15, 5, 80, 18, &style_rect_fg);
        // Draw the label
        lv_canvas_draw_text(canvas, 5, 4, 100, &style_label_18_center_bg, "USB Out");
        // Draw the current profile indicator
        uint8_t xOffset = 129;
        uint8_t yOffset = 14;
        char profileNumber[4] = {};
        sprintf(profileNumber, "%d", (uint8_t)widget->active_profile_index + 1);
        if (profileConnected) {
            lv_canvas_draw_arc(canvas, xOffset, yOffset, 13, 0, 359, &style_arc_2);
            lv_canvas_draw_arc(canvas, xOffset, yOffset, 9, 0, 359, &style_arc_9);
            lv_canvas_draw_text(canvas, xOffset - 8, yOffset - 11, 16, &style_label_18_center_bg, profileNumber);
        } else if (profileAdvertising) {
            lv_draw_img_dsc_t img_dsc;
            lv_draw_img_dsc_init(&img_dsc);
            lv_canvas_draw_img(canvas, xOffset - 14, yOffset - 14, &gear_icon, &img_dsc);
            lv_canvas_draw_text(canvas, xOffset - 8, yOffset - 11, 16, &style_label_18_center_bg, profileNumber);
        } else {
            lv_draw_img_dsc_t img_dsc;
            lv_draw_img_dsc_init(&img_dsc);
            lv_canvas_draw_img(canvas, xOffset - 14, yOffset - 14, &disconnected_icon, &img_dsc);
            lv_canvas_draw_text(canvas, xOffset - 8, yOffset - 11, 16, &style_label_18_center_bg, profileNumber);
        }
    } else {
        // Draw circles
        uint8_t xOffset = 15;
        uint8_t yOffset = 15;
        for (int i = 0; i < 5; i++) {
            bool selected = i == widget->active_profile_index;
            char label[2];
            snprintf(label, sizeof(label), "%d", i + 1);

            if (selected) {
                if (profileAdvertising) {
                    lv_draw_img_dsc_t img_dsc;
                    lv_draw_img_dsc_init(&img_dsc);
                    lv_canvas_draw_img(canvas, xOffset - 14, yOffset - 14, &gear_icon, &img_dsc);
                    lv_canvas_draw_text(canvas, xOffset - 8, yOffset - 11, 16, &style_label_18_center_bg, label);
                } else if (profileConnected) {
                    lv_canvas_draw_arc(canvas, xOffset, yOffset, 13, 0, 359, &style_arc_2);
                    lv_canvas_draw_arc(canvas, xOffset, yOffset, 9, 0, 359, &style_arc_9);
                    lv_canvas_draw_text(canvas, xOffset - 8, yOffset - 11, 16, &style_label_18_center_bg, label);
                } else {
                    lv_draw_img_dsc_t img_dsc;
                    lv_draw_img_dsc_init(&img_dsc);
                    lv_canvas_draw_img(canvas, xOffset - 14, yOffset - 14, &disconnected_icon, &img_dsc);
                    lv_canvas_draw_text(canvas, xOffset - 8, yOffset - 11, 16, &style_label_18_center_bg, label);
                }
            } else {
                lv_canvas_draw_arc(canvas, xOffset, yOffset, 13, 0, 359, &style_arc_2);
                lv_canvas_draw_text(canvas, xOffset - 8, yOffset - 11, 16, &style_label_18_center, label);
            }
            xOffset += 29;
        }
    }
}


/* ========================================================================= */
/*                    EVENT LISTENERS AND STATE UPDATES                      */
/* ========================================================================= */
static void set_output_status(struct zmk_widget_output_status *widget, const struct output_status_state *state) {
    widget->selected_endpoint = state->selected_endpoint;
    widget->active_profile_index = state->active_profile_index;
    widget->active_profile_connected = state->active_profile_connected;
    widget->active_profile_bonded = state->active_profile_bonded;
    draw_output_info(widget);
}


static void output_status_update_cb(struct output_status_state state) {
    struct zmk_widget_output_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_output_status(widget, &state); }
}


static struct output_status_state output_status_get_state(const zmk_event_t *_eh) {
    return (struct output_status_state){
        .selected_endpoint = zmk_endpoints_selected(),
        .active_profile_index = zmk_ble_active_profile_index(),
        .active_profile_connected = zmk_ble_active_profile_is_connected(),
        .active_profile_bonded = !zmk_ble_active_profile_is_open(),
    };
}


ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state, output_status_update_cb,
                            output_status_get_state)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);
#endif
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
#endif

/* ========================================================================= */
/*                              INITIALIZATION                               */
/* ========================================================================= */
int zmk_widget_output_status_init(struct zmk_widget_output_status *widget, lv_obj_t *canvas, lv_color_t *cbuf) {
    widget->canvas = canvas;
    widget->cbuf = cbuf;
    sys_slist_append(&widgets, &widget->node);
    widget_output_status_init();
    return 0;
}
