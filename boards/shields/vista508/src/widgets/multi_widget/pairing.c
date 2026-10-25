/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * @file pairing.c
 * @brief Implementation of the Bluetooth pairing view for the multi-widget.
 * @details Renders the Bluetooth passkey digits provided by ZMK pairing events, displaying the pairing PIN code or a
 * failure/success animation.
 */

/* ========================================================================= */
/*                        INCLUDES AND DEPENDENCIES                          */
/* ========================================================================= */
#include "../../status_screen.h"
#include "../../styles.h"
#include "../../util.h"
#include "views.h"
#include <stdio.h>
#include <string.h>

#include <zmk/ble.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/ble_pairing_complete.h>
#include <zmk/events/ble_passkey_digits_changed.h>
#include <zmk/events/ble_passkey_state_changed.h>

/* ========================================================================= */
/*                            STATE AND GLOBALS                              */
/* ========================================================================= */
extern const lv_img_dsc_t *dance_frames[];
extern const uint32_t dance_frame_count;

static bool pairing_active = false;
static char passkey[7] = "";
static uint8_t passkey_len = 0;
static bool pairing_celebration_active = false;
static uint8_t celebration_frame_index = 0;
static bool pairing_failed_active = false;
static bool profile_cleared_active = false;
static bool profile_disconnected_active = false;
static uint8_t cleared_profile_index = 0;
static uint8_t disconnected_profile_index = 0;
static int active_profile_index = 0;
static bool active_profile_bonded = false;
static bool active_profile_connected = false;

static lv_timer_t *pairing_cursor_timer = NULL;
static lv_timer_t *celebration_anim_timer = NULL;

/* ========================================================================= */
/*                            TIMERS AND WORKERS                             */
/* ========================================================================= */

static void profile_cleared_timeout_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(profile_cleared_timeout_work, profile_cleared_timeout_handler);

static void profile_cleared_timeout_handler(struct k_work *work) {
    profile_cleared_active = false;
    zmk_widget_multi_status_update_async();
}


static void profile_disconnected_timeout_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(profile_disconnected_timeout_work, profile_disconnected_timeout_handler);

static void profile_disconnected_timeout_handler(struct k_work *work) {
    profile_disconnected_active = false;
    zmk_widget_multi_status_update_async();
}


static void pairing_failed_timeout_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(pairing_failed_timeout_work, pairing_failed_timeout_handler);

static void pairing_failed_timeout_handler(struct k_work *work) {
    pairing_failed_active = false;
    zmk_widget_multi_status_update_async();
}


static void pairing_cursor_anim_timer_cb(lv_timer_t *timer) {
    if (pairing_active && !pairing_celebration_active) {
        zmk_widget_multi_status_update();
    }
}


static void celebration_anim_timer_cb(lv_timer_t *timer) {
    if (pairing_celebration_active) {
        celebration_frame_index++;
        if (celebration_frame_index >= dance_frame_count) {
            pairing_celebration_active = false;
        }
        zmk_widget_multi_status_update();
    }
}


void pairing_init(void) {}


void pairing_animations_stop(void) {
    if (pairing_cursor_timer != NULL) {
        lv_timer_del(pairing_cursor_timer);
        pairing_cursor_timer = NULL;
    }
    if (celebration_anim_timer != NULL) {
        lv_timer_del(celebration_anim_timer);
        celebration_anim_timer = NULL;
    }
}


bool pairing_celebration_is_active(void) { return pairing_celebration_active; }
bool pairing_is_active(void) { return pairing_active; }
bool pairing_failed_is_active(void) { return pairing_failed_active; }
bool profile_cleared_is_active(void) { return profile_cleared_active; }
bool profile_disconnected_is_active(void) { return profile_disconnected_active; }

/* ========================================================================= */
/*                    EVENT LISTENERS AND STATE UPDATES                      */
/* ========================================================================= */

static void set_passkey_state(bool active) {
    pairing_active = active;
    if (active) {
        pairing_failed_active = false;
        k_work_cancel_delayable(&pairing_failed_timeout_work);
        profile_cleared_active = false;
        k_work_cancel_delayable(&profile_cleared_timeout_work);
        profile_disconnected_active = false;
        k_work_cancel_delayable(&profile_disconnected_timeout_work);
    }
    zmk_widget_multi_status_update_async();
}


int passkey_state_listener(const zmk_event_t *eh) {
    const struct ble_passkey_state_changed *ev = as_ble_passkey_state_changed(eh);
    set_passkey_state(ev->active);
    return ZMK_EV_EVENT_BUBBLE;
}


ZMK_LISTENER(display_passkey_state_listener, passkey_state_listener);
ZMK_SUBSCRIPTION(display_passkey_state_listener, ble_passkey_state_changed);

static void set_passkey_digits(const struct ble_passkey_digits_changed *ev) {
    passkey_len = ev->digits_len;
    strncpy(passkey, ev->passkey, sizeof(passkey));
    zmk_widget_multi_status_update_async();
}


int passkey_digits_listener(const zmk_event_t *eh) {
    const struct ble_passkey_digits_changed *ev = as_ble_passkey_digits_changed(eh);
    set_passkey_digits(ev);
    return ZMK_EV_EVENT_BUBBLE;
}


ZMK_LISTENER(display_passkey_digits_listener, passkey_digits_listener);
ZMK_SUBSCRIPTION(display_passkey_digits_listener, ble_passkey_digits_changed);

static void set_pairing_complete(bool bonded) {
    if (bonded) {
        pairing_celebration_active = true;
        celebration_frame_index = 0;
        pairing_failed_active = false;
        k_work_cancel_delayable(&pairing_failed_timeout_work);
        profile_cleared_active = false;
        k_work_cancel_delayable(&profile_cleared_timeout_work);
        profile_disconnected_active = false;
        k_work_cancel_delayable(&profile_disconnected_timeout_work);
        zmk_widget_multi_status_update_async();
    } else {
        pairing_celebration_active = false;
        pairing_failed_active = true;
        k_work_reschedule(&pairing_failed_timeout_work, K_MSEC(CONFIG_VISTA508_MESSAGE_DISPLAY_MS));
        zmk_widget_multi_status_update_async();
    }
}


int display_pairing_complete_listener(const zmk_event_t *eh) {
    const struct ble_pairing_complete *ev = as_ble_pairing_complete(eh);
    set_pairing_complete(ev->bonded);
    return ZMK_EV_EVENT_BUBBLE;
}


ZMK_LISTENER(display_pairing_complete_sub, display_pairing_complete_listener);
ZMK_SUBSCRIPTION(display_pairing_complete_sub, ble_pairing_complete);

static void set_output_status(int active_profile_index_val, bool active_profile_connected_val,
                              bool active_profile_bonded_val) {
    if (active_profile_index == active_profile_index_val) {
        if (active_profile_bonded && !active_profile_bonded_val) {
            profile_cleared_active = true;
            cleared_profile_index = active_profile_index_val;
            k_work_reschedule(&profile_cleared_timeout_work, K_MSEC(CONFIG_VISTA508_MESSAGE_DISPLAY_MS));
            pairing_failed_active = false;
            k_work_cancel_delayable(&pairing_failed_timeout_work);
        }

        if (active_profile_connected && !active_profile_connected_val) {
            profile_disconnected_active = true;
            disconnected_profile_index = active_profile_index_val;
            k_work_reschedule(&profile_disconnected_timeout_work, K_MSEC(CONFIG_VISTA508_MESSAGE_DISPLAY_MS));
            pairing_failed_active = false;
            k_work_cancel_delayable(&pairing_failed_timeout_work);
        }
    } else {
        profile_cleared_active = false;
        k_work_cancel_delayable(&profile_cleared_timeout_work);
        profile_disconnected_active = false;
        k_work_cancel_delayable(&profile_disconnected_timeout_work);
    }

    active_profile_index = active_profile_index_val;
    active_profile_bonded = active_profile_bonded_val;
    active_profile_connected = active_profile_connected_val;

    zmk_widget_multi_status_update_async();
}


int multi_output_status_listener(const zmk_event_t *eh) {
    const struct zmk_ble_active_profile_changed *profile_ev = as_zmk_ble_active_profile_changed(eh);
    if (profile_ev) {
        set_output_status(profile_ev->index, zmk_ble_active_profile_is_connected(), !zmk_ble_active_profile_is_open());
    }
    return ZMK_EV_EVENT_BUBBLE;
}


ZMK_LISTENER(multi_output_status, multi_output_status_listener);
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(multi_output_status, zmk_ble_active_profile_changed);
#endif

/* ========================================================================= */
/*                         UI RENDERING AND DRAWING                          */
/* ========================================================================= */
void draw_pairing_passkey(lv_obj_t *canvas) {
    const uint8_t yOffset = 0;

    if (pairing_cursor_timer == NULL) {
        pairing_cursor_timer = lv_timer_create(pairing_cursor_anim_timer_cb, 500, NULL);
    }

    if (passkey_len == 6) {
        char passkey_display[16] = "";
        strncpy(passkey_display, passkey, sizeof(passkey_display) - 1);

        lv_canvas_draw_text(canvas, 0, yOffset + 7, DISPLAY_WIDTH, &style_label_16_center, passkey_display);
        lv_canvas_draw_text(canvas, 0, yOffset + 33, DISPLAY_WIDTH, &style_label_18_center, "Press Enter\nTo Complete");
    } else {
        lv_canvas_draw_text(canvas, 0, yOffset + 15, DISPLAY_WIDTH, &style_label_18_center, "Enter Passkey");

        lv_point_t line_pts[2];
        line_pts[0].x = 30;
        line_pts[0].y = yOffset + 68;
        line_pts[1].x = DISPLAY_WIDTH - 30;
        line_pts[1].y = yOffset + 68;
        lv_canvas_draw_line(canvas, line_pts, 2, &style_line_1);

        char passkey_display[16] = "";
        strncpy(passkey_display, passkey, sizeof(passkey_display) - 1);

        bool show_cursor = (k_uptime_get_32() % 1000) < 500;
        if (show_cursor) {
            strncat(passkey_display, "|", 2);
        }

        lv_canvas_draw_text(canvas, 34, yOffset + 45, DISPLAY_WIDTH - 68, &style_label_18_left, passkey_display);
    }
}


void draw_pairing_failed(lv_obj_t *canvas) {
    const uint8_t yOffset = 0;
    lv_canvas_draw_text(canvas, 0, yOffset + 20, DISPLAY_WIDTH, &style_label_18_center, "Pairing\nFailed");
}


void draw_profile_cleared(lv_obj_t *canvas) {
    const uint8_t yOffset = 0;

    char text[48];
    snprintf(text, sizeof(text), "Cleared\nBluetooth\nProfile %d", cleared_profile_index + 1);

    lv_canvas_draw_text(canvas, 0, yOffset + 9, DISPLAY_WIDTH, &style_label_18_center, text);
}


void draw_profile_disconnected(lv_obj_t *canvas) {
    const uint8_t yOffset = 0;

    char text[48];
    snprintf(text, sizeof(text), "Disconnected\nFrom Bluetooth\nProfile %d", disconnected_profile_index + 1);

    lv_canvas_draw_text(canvas, 0, yOffset + 9, DISPLAY_WIDTH, &style_label_16_center, text);
}


void draw_dance_animation(lv_obj_t *canvas) {
    const uint8_t yOffset = 0;
    const int WIDGET_HEIGHT = 82;
    lv_draw_img_dsc_t img_dsc;

    if (celebration_anim_timer == NULL) {
        celebration_anim_timer = lv_timer_create(celebration_anim_timer_cb, 100, NULL);
    }
    lv_draw_img_dsc_init(&img_dsc);

    const lv_img_dsc_t *frame = dance_frames[celebration_frame_index];
    int x_offset = (DISPLAY_WIDTH - frame->header.w) / 2;
    int y_offset = yOffset + (WIDGET_HEIGHT - frame->header.h) / 2 + 1;

    lv_canvas_draw_img(canvas, x_offset, y_offset, frame, &img_dsc);
}
