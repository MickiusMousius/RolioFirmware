/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * @file output.h
 * @brief Interface for the output and connectivity status widget.
 * @details Defines the state struct for the output widget, tracking the selected endpoint and active BLE profile
 * status.
 */

/* ========================================================================= */
/*                        INCLUDES AND DEPENDENCIES                          */
/* ========================================================================= */
#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zmk/endpoints.h>

/* ========================================================================= */
/*                            STATE STRUCTURES                               */
/* ========================================================================= */
struct zmk_widget_output_status {
    sys_snode_t node;
    lv_obj_t *canvas;
    lv_color_t *cbuf;

    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
};


/* ========================================================================= */
/*                               PUBLIC API                                  */
/* ========================================================================= */
int zmk_widget_output_status_init(struct zmk_widget_output_status *widget, lv_obj_t *canvas, lv_color_t *cbuf);
