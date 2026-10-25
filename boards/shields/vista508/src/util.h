/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * @file util.h
 * @brief Macros, constants, and inline utility functions for the Vista508 shield display graphics.
 * @details Contains shared macros, UI constants (like background/foreground colors), and inline function
 * implementations utilized by the various drawing routines. These functions are inlined here to reduce call overhead
 * during UI rendering.
 */

/* ========================================================================= */
/*                        INCLUDES AND DEPENDENCIES                          */
/* ========================================================================= */
#pragma once

#include <lvgl.h>
#include <zmk/endpoints.h>

#define CANVAS_SIZE 144
#define DISPLAY_WIDTH 144
#define DISPLAY_HEIGHT 168

#define WPM_SAMPLES 20

#define LVGL_BACKGROUND IS_ENABLED(CONFIG_VISTA508_WIDGET_INVERTED) ? lv_color_black() : lv_color_white()
#define LVGL_FOREGROUND IS_ENABLED(CONFIG_VISTA508_WIDGET_INVERTED) ? lv_color_white() : lv_color_black()

/* ========================================================================= */
/*                            STATE STRUCTURES                               */
/* ========================================================================= */
struct battery_info {
    uint8_t source;
    uint8_t level;
    bool usb_present;
};
