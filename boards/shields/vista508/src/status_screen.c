/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * @file status_screen.c
 * @brief Main implementation of the custom status screen for the Vista508 shield, handling widget initialization and
 * display layout.
 * @details Allocates LVGL canvas objects for the different widget areas (battery, multi-view, output info, and layer
 * status), assigns memory buffers, and positions them on the main display screen.
 */

/* ========================================================================= */
/*                        INCLUDES AND DEPENDENCIES                          */
/* ========================================================================= */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include "status_screen.h"
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>

/* ========================================================================= */
/*                            STATE AND GLOBALS                              */
/* ========================================================================= */
LV_IMG_DECLARE(warning_dark);
LV_IMG_DECLARE(warning_light);
const lv_img_dsc_t *warning_imgs[] = {
    &warning_dark,
    &warning_light,
};


bool board_is_active = true;

/* ========================================================================= */
/*                    EVENT LISTENERS AND STATE UPDATES                      */
/* ========================================================================= */
int widget_activity_state_listener(const zmk_event_t *eh) {
    const struct zmk_activity_state_changed *ev = as_zmk_activity_state_changed(eh);
    if (ev == NULL)
        return ZMK_EV_EVENT_BUBBLE;
    if (ev->state == ZMK_ACTIVITY_ACTIVE) {
        board_is_active = true;
    } else {
        board_is_active = false;
        zmk_widget_multi_halt_animations();
    }

    zmk_widget_multi_status_update();

    return ZMK_EV_EVENT_BUBBLE;
}


ZMK_LISTENER(widget_activity_state, widget_activity_state_listener);
ZMK_SUBSCRIPTION(widget_activity_state, zmk_activity_state_changed);


/* ========================================================================= */
/*                              INITIALIZATION                               */
/* ========================================================================= */
int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    lv_obj_t *batteryArea = lv_canvas_create(widget->obj);
    lv_obj_align(batteryArea, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_canvas_set_buffer(batteryArea, widget->cbuf0, DISPLAY_WIDTH, 20, LV_IMG_CF_TRUE_COLOR);

    lv_obj_t *multiArea = lv_canvas_create(widget->obj);
    lv_obj_align(multiArea, LV_ALIGN_TOP_LEFT, 0, 22);
    lv_canvas_set_buffer(multiArea, widget->cbuf1, DISPLAY_WIDTH, 90, LV_IMG_CF_TRUE_COLOR);

    lv_obj_t *outputInfoArea = lv_canvas_create(widget->obj);
    lv_obj_align(outputInfoArea, LV_ALIGN_TOP_LEFT, 0, 112);
    lv_canvas_set_buffer(outputInfoArea, widget->cbuf2, DISPLAY_WIDTH, 30, LV_IMG_CF_TRUE_COLOR);

    lv_obj_t *bottom = lv_canvas_create(widget->obj);
    lv_obj_align(bottom, LV_ALIGN_TOP_LEFT, 0, 143);
    lv_canvas_set_buffer(bottom, widget->cbuf3, DISPLAY_WIDTH, 26, LV_IMG_CF_TRUE_COLOR);

    lv_obj_t *art = lv_animimg_create(widget->obj);
    lv_obj_align(art, LV_ALIGN_TOP_RIGHT, 0, -30);
    lv_animimg_set_src(art, (const void **)warning_imgs, 2);

    zmk_widget_battery_status_init(&widget->battery_widget, batteryArea, widget->cbuf0, art);
    zmk_widget_multi_status_init(&widget->multi_widget, multiArea, widget->cbuf1);
    zmk_widget_output_status_init(&widget->output_widget, outputInfoArea, widget->cbuf2);
    zmk_widget_layer_status_init(&widget->layer_widget, bottom, widget->cbuf3);

    return 0;
}


static struct zmk_widget_status status_widget;

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen;
    screen = lv_obj_create(NULL);

    zmk_widget_status_init(&status_widget, screen);
    lv_obj_align(zmk_widget_status_obj(&status_widget), LV_ALIGN_TOP_LEFT, 0, 0);

    return screen;
}


lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) { return widget->obj; }
