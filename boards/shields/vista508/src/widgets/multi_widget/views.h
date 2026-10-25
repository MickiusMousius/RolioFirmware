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

void pairing_init(void);
void pairing_animations_stop(void);
bool pairing_celebration_is_active(void);
void draw_dance_animation(lv_obj_t *canvas);
bool pairing_is_active(void);
void draw_pairing_passkey(lv_obj_t *canvas);
bool pairing_failed_is_active(void);
void draw_pairing_failed(lv_obj_t *canvas);
bool profile_cleared_is_active(void);
void draw_profile_cleared(lv_obj_t *canvas);
bool profile_disconnected_is_active(void);
void draw_profile_disconnected(lv_obj_t *canvas);
bool profile_changed_is_active(void);
void draw_profile_changed(lv_obj_t *canvas);

void hello_animation_init(void);
bool hello_animation_is_active(void);
void hello_animation_stop(void);
void draw_hello_animation(lv_obj_t *canvas);

void typing_animation_init(void);
void typing_animation_start(void);
void typing_animation_stop(void);
void draw_typing_animation(lv_obj_t *canvas);
