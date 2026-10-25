/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * @file layer.h
 * @brief Interface for the layer status widget.
 * @details Defines the layer widget state structure and provides its initialization prototype.
 */

/* ========================================================================= */
/*                        INCLUDES AND DEPENDENCIES                          */
/* ========================================================================= */
#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

/* ========================================================================= */
/*                            STATE STRUCTURES                               */
/* ========================================================================= */
struct zmk_widget_layer_status {
    sys_snode_t node;
    lv_obj_t *canvas;
    lv_color_t *cbuf;
    uint8_t layer_index;
    const char *layer_label;
};


/* ========================================================================= */
/*                               PUBLIC API                                  */
/* ========================================================================= */
int zmk_widget_layer_status_init(struct zmk_widget_layer_status *widget, lv_obj_t *canvas, lv_color_t *cbuf);
