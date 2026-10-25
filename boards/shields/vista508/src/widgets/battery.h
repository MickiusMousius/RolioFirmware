/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * @file battery.h
 * @brief Interface for the battery status widget.
 * @details Declares the battery widget structure and initialization function, maintaining the state array of battery
 * levels.
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
struct zmk_widget_battery_status {
    sys_snode_t node;
    lv_obj_t *canvas;
    lv_color_t *cbuf;
    lv_obj_t *warning_art;

    struct battery_info batteries[2];
};


/* ========================================================================= */
/*                               PUBLIC API                                  */
/* ========================================================================= */
int zmk_widget_battery_status_init(struct zmk_widget_battery_status *widget, lv_obj_t *canvas, lv_color_t *cbuf,
                                   lv_obj_t *warning_art);
