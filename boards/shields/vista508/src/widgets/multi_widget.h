/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * @file multi_widget.h
 * @brief Interface and state definitions for the dynamic multi-view widget.
 * @details Defines the multi-widget data structure that holds the aggregated state required to draw any of the
 * sub-views.
 */

/* ========================================================================= */
/*                        INCLUDES AND DEPENDENCIES                          */
/* ========================================================================= */
#pragma once

#include "../util.h"
#include <lvgl.h>
#include <zephyr/kernel.h>

/* ========================================================================= */
/*                            STATE STRUCTURES                               */
/* ========================================================================= */
struct zmk_widget_multi_status {
    sys_snode_t node;
    lv_obj_t *canvas;
    lv_color_t *cbuf;

    uint8_t layer_index;
};


/* ========================================================================= */
/*                               PUBLIC API                                  */
/* ========================================================================= */
int zmk_widget_multi_status_init(struct zmk_widget_multi_status *widget, lv_obj_t *canvas, lv_color_t *cbuf);
void zmk_widget_multi_status_update(void);
void zmk_widget_multi_status_update_async(void);
sys_slist_t *zmk_widget_multi_get_list(void);

bool zmk_widget_multi_is_wpm_chart_active(void);

void zmk_widget_multi_halt_animations(void);

void vista508_wpm_mode_toggle(void);
