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

#if IS_ENABLED(CONFIG_ZMK_BQ24075)
    struct {
        bool is_charging;
        bool usb_present;
        uint16_t charge_limit_mv;
        uint16_t battery_mv;
    } bq24075_states[2];
#endif

    struct battery_info batteries[2];
};


/* ========================================================================= */
/*                               PUBLIC API                                  */
/* ========================================================================= */
int zmk_widget_battery_status_init(struct zmk_widget_battery_status *widget, lv_obj_t *canvas, lv_color_t *cbuf,
                                   lv_obj_t *warning_art);
