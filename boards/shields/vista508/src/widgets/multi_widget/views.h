/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * @file views.h
 * @brief Shared interface and definitions for the various sub-views within the multi-widget.
 * @details Declares the function prototypes for drawing the various sub-views (pairing, settings notices, WPM, typing)
 * used by the main multi-widget controller.
 */

/* ========================================================================= */
/*                        INCLUDES AND DEPENDENCIES                          */
/* ========================================================================= */
#pragma once

#include "../multi_widget.h"
#include <lvgl.h>

/* ========================================================================= */
/*                               PUBLIC API                                  */
/* ========================================================================= */
void draw_sleep_image(lv_obj_t *canvas);
void draw_wpm_chart(lv_obj_t *canvas);
void draw_wpm_text_notification(lv_obj_t *canvas, int display_mode);

void hello_animation_init(void);
bool hello_animation_is_active(void);
void hello_animation_stop(void);
void draw_hello_animation(lv_obj_t *canvas);

void typing_animation_init(void);
void typing_animation_start(void);
void typing_animation_stop(void);
void draw_typing_animation(lv_obj_t *canvas);
