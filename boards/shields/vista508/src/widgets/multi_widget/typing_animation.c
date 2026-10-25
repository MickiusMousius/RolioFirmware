/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * @file typing_animation.c
 * @brief Implementation of the typing animation view for the multi-widget, triggered by key press events.
 * @details Draws a sequence of animation frames based on the current WPM to visualize typing activity, advancing frames
 * proportional to typing speed.
 */

/* ========================================================================= */
/*                        INCLUDES AND DEPENDENCIES                          */
/* ========================================================================= */
#include "../../status_screen.h"
#include "../../styles.h"
#include "../../util.h"
#include "views.h"
#include <stdio.h>

#include <zmk/event_manager.h>
#include <zmk/events/wpm_state_changed.h>

/* ========================================================================= */
/*                            STATE AND GLOBALS                              */
/* ========================================================================= */
extern const lv_img_dsc_t *typing_frames[];
extern const uint32_t typing_frame_count;

static uint32_t typing_frame_index = 0;
static uint32_t typing_anim_progress = 0;
static lv_timer_t *typing_anim_timer = NULL;
static uint8_t display_wpm = 0;
extern bool board_is_active;

#include <zmk/events/position_state_changed.h>

static int64_t last_keypress_time = 0;

static void typing_anim_timer_cb(lv_timer_t *timer) {
    uint8_t old_wpm = display_wpm;

    // Check how long it has been since the last physical keypress
    int64_t idle_time = k_uptime_get() - last_keypress_time;

    if (idle_time > 750) { // 0.75 seconds without a keypress
        // Fast decay mode: When the user stops typing, ZMK's internal WPM
        // decays slowly (over a ~5 second moving average). To make the UI
        // feel snappy and responsive, we artificially bleed off the `display_wpm`
        // 7.5x faster (subtracting 15 WPM every ~66ms instead of 2).
        // This drops a 100+ WPM score to 0 in under half a second.
        if (display_wpm >= 23) {
            display_wpm -= 23;
        } else {
            display_wpm = 0;
        }
    } else {
        // Normal smoothing mode: While actively typing, we apply a gentle decay
        // (2 WPM per 66ms) to smooth out the frame progression and prevent
        // jitter if the upstream ZMK WPM calculation pauses briefly.
        if (display_wpm > 0) {
            if (display_wpm >= 3) {
                display_wpm -= 3;
            } else {
                display_wpm = 0;
            }
        }
    }

    if (board_is_active) {
        if (display_wpm == 0) {
            // If typing has completely stopped, reset to the first frame
            if (typing_frame_index != 0 || old_wpm != display_wpm) {
                typing_frame_index = 0;
                typing_anim_progress = 0;
                zmk_widget_multi_status_update();
            }
        } else {
            typing_anim_progress += display_wpm;
            if (typing_anim_progress >= typing_frame_count * 100) {
                typing_anim_progress -= typing_frame_count * 100;
            }
            uint32_t new_frame_index = (typing_anim_progress / 100) % typing_frame_count;
            if (new_frame_index != typing_frame_index || old_wpm != display_wpm) {
                typing_frame_index = new_frame_index;
                zmk_widget_multi_status_update();
            }
        }
    }
}


static void set_wpm_state(uint8_t current_wpm) {
    int64_t idle_time = k_uptime_get() - last_keypress_time;
    if (idle_time > 750 && current_wpm > 0) {
        // Ignore upstream WPM updates if we are in fast-decay idle mode.
        // Since ZMK's WPM decays slowly over 5 seconds, it will continue to
        // emit events with WPM > 0 long after the user has stopped typing.
        // We reject these late updates so they don't snap our fast-decayed
        // `display_wpm` back up, which would cause the animation to stutter.
        return;
    }

    if (current_wpm > display_wpm) {
        display_wpm = current_wpm;
    }

    // Lower bound the smoothing by the raw current WPM
    if (display_wpm < current_wpm) {
        display_wpm = current_wpm;
    }

    if (current_wpm > 0) {
        typing_animation_start();
    } else {
        display_wpm = 0;
        typing_animation_stop();
        zmk_widget_multi_status_update_async();
    }
}


int typing_anim_wpm_listener(const zmk_event_t *eh) {
    const struct zmk_wpm_state_changed *ev = as_zmk_wpm_state_changed(eh);
    if (ev) {
        set_wpm_state(ev->state);
    }
    return ZMK_EV_EVENT_BUBBLE;
}


ZMK_LISTENER(widget_typing_anim_wpm, typing_anim_wpm_listener);
ZMK_SUBSCRIPTION(widget_typing_anim_wpm, zmk_wpm_state_changed);

int typing_anim_position_listener(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (ev && ev->state) { // Key pressed
        last_keypress_time = k_uptime_get();
        // If animation was stopped, ensure we wake it up next WPM tick
    }
    return ZMK_EV_EVENT_BUBBLE;
}


ZMK_LISTENER(widget_typing_anim_pos, typing_anim_position_listener);
ZMK_SUBSCRIPTION(widget_typing_anim_pos, zmk_position_state_changed);


void typing_animation_init(void) {
    // Timer created on demand but we leave this here for consistency with other widgets
}


void typing_animation_start(void) {
    if (typing_anim_timer == NULL) {
        typing_anim_timer = lv_timer_create(typing_anim_timer_cb, 100, NULL);
    }
}


void typing_animation_stop(void) {
    if (typing_anim_timer != NULL) {
        lv_timer_del(typing_anim_timer);
        typing_anim_timer = NULL;
        typing_frame_index = 0;
        typing_anim_progress = 0;
    }
}


/* ========================================================================= */
/*                         UI RENDERING AND DRAWING                          */
/* ========================================================================= */
void draw_typing_animation(lv_obj_t *canvas) {
    const uint8_t yOffset = 0;
    const int WIDGET_HEIGHT = 82;

    lv_draw_img_dsc_t img_dsc;
    lv_draw_img_dsc_init(&img_dsc);

    const lv_img_dsc_t *frame = typing_frames[typing_frame_index];
    int x_offset = (DISPLAY_WIDTH - frame->header.w) / 2;
    int y_offset = yOffset + (WIDGET_HEIGHT - frame->header.h) / 2;

    lv_canvas_draw_img(canvas, x_offset, y_offset, frame, &img_dsc);

    char wpm_text[6] = {};
    snprintf(wpm_text, sizeof(wpm_text), "%d", display_wpm);
    lv_canvas_draw_text(canvas, DISPLAY_WIDTH - 44, yOffset + 5, 40, &style_label_16_right, wpm_text);
}
