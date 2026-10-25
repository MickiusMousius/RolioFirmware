/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * @file status_screen.h
 * @brief Definitions and interface for the Vista508 custom status screen initialization.
 * @details Exposes the initialization function for the status screen and macros/constants required by the main UI and
 * its sub-widgets.
 */

#pragma once

/* ========================================================================= */
/*                        INCLUDES AND DEPENDENCIES                          */
/* ========================================================================= */
#include "util.h"
#include "widgets/battery.h"
#include "widgets/layer.h"
#include "widgets/multi_widget.h"
#include "widgets/output.h"
#include <lvgl.h>
#include <zephyr/kernel.h>

/* ========================================================================= */
/*                            STATE STRUCTURES                               */
/* ========================================================================= */
struct zmk_widget_status {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_color_t cbuf0[DISPLAY_WIDTH * 20];
    lv_color_t cbuf1[DISPLAY_WIDTH * 90];
    lv_color_t cbuf2[DISPLAY_WIDTH * 30];
    lv_color_t cbuf3[DISPLAY_WIDTH * 26];

    struct zmk_widget_battery_status battery_widget;
    struct zmk_widget_multi_status multi_widget;
    struct zmk_widget_output_status output_widget;
    struct zmk_widget_layer_status layer_widget;
};


/* ========================================================================= */
/*                               PUBLIC API                                  */
/* ========================================================================= */
int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget);
