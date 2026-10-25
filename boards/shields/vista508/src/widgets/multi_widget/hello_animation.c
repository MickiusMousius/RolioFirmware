/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * @file hello_animation.c
 * @brief Implementation of the hello animation view for the multi-widget, triggered once on boot.
 */

/* ========================================================================= */
/*                        INCLUDES AND DEPENDENCIES                          */
/* ========================================================================= */
#include "../../status_screen.h"
#include "../../styles.h"
#include "../../util.h"
#include "views.h"

/* ========================================================================= */
/*                            STATE AND GLOBALS                              */
/* ========================================================================= */
extern const lv_img_dsc_t *hello_frames[];
extern const uint32_t hello_frame_count;

static lv_timer_t *boot_anim_timer = NULL;
static bool boot_anim_active = false;
static uint8_t boot_frame_index = 0;

/* ========================================================================= */
/*                         UI RENDERING AND DRAWING                          */
/* ========================================================================= */

static void boot_anim_timer_cb(lv_timer_t *timer) {
    if (boot_anim_active) {
        boot_frame_index++;
        if (boot_frame_index >= hello_frame_count) {
            boot_anim_active = false;
            lv_timer_del(boot_anim_timer);
            boot_anim_timer = NULL;
        }
        zmk_widget_multi_status_update();
    }
}


void hello_animation_init(void) {
    boot_anim_active = true;
    boot_frame_index = 0;
    if (boot_anim_timer == NULL) {
        boot_anim_timer = lv_timer_create(boot_anim_timer_cb, 100, NULL);
    }
}


bool hello_animation_is_active(void) { return boot_anim_active; }

void hello_animation_stop(void) {
    if (boot_anim_timer != NULL) {
        lv_timer_del(boot_anim_timer);
        boot_anim_timer = NULL;
    }
    boot_anim_active = false;
}


void draw_hello_animation(lv_obj_t *canvas) {
    const uint8_t yOffset = 0;
    const int WIDGET_HEIGHT = 82;

    lv_draw_img_dsc_t img_dsc;
    lv_draw_img_dsc_init(&img_dsc);

    uint8_t frame_index = boot_frame_index;
    if (frame_index >= hello_frame_count) {
        frame_index = hello_frame_count - 1;
    }

    const lv_img_dsc_t *frame = hello_frames[frame_index];
    int x_offset = (DISPLAY_WIDTH - frame->header.w) / 2;
    int y_offset = yOffset + (WIDGET_HEIGHT - frame->header.h) / 2;

    lv_canvas_draw_img(canvas, x_offset, y_offset, frame, &img_dsc);
}
