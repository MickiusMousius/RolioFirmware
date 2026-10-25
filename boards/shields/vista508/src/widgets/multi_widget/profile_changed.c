/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * @file profile_changed.c
 * @brief Logic for displaying profile/endpoint changes on the multi-widget.
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
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>

/* ========================================================================= */
/*                            STATE AND GLOBALS                              */
/* ========================================================================= */

/**
 * @brief Special constant to indicate that the active transport hasn't been initialized yet.
 * This prevents the notification from popping up during the initial boot sequence.
 */
#define TRANSPORT_UNINITIALIZED 255

static uint8_t active_transport = TRANSPORT_UNINITIALIZED;
static bool profile_changed_active = false;

/* ========================================================================= */
/*                            TIMERS AND WORKERS                             */
/* ========================================================================= */

/**
 * @brief Handler called when the profile change notification timeout expires.
 * Clears the active flag and triggers a redraw to clear the notification.
 */
static void profile_changed_timeout_handler(struct k_work *work) {
    profile_changed_active = false;
    zmk_widget_multi_status_update_async();
}


K_WORK_DELAYABLE_DEFINE(profile_changed_timeout_work, profile_changed_timeout_handler);

/* ========================================================================= */
/*                         UI RENDERING AND DRAWING                          */
/* ========================================================================= */

bool profile_changed_is_active(void) { return profile_changed_active; }


LV_FONT_DECLARE(lv_font_montserrat_85_profiles);

/**
 * @brief Renders the profile changed notification text or icon.
 * Applies custom pixel offsets to correct for differences in font side bearings and baselines.
 */
void draw_profile_changed(lv_obj_t *canvas) {
    static lv_draw_label_dsc_t label_dsc;
    static bool init = false;

    /* Initialize the label descriptor once to save CPU cycles */
    if (!init) {
        lv_draw_label_dsc_init(&label_dsc);
        label_dsc.color = LVGL_FOREGROUND;
        label_dsc.font = &lv_font_montserrat_85_profiles;
        label_dsc.align = LV_TEXT_ALIGN_CENTER;
        init = true;
    }

    char text[4];
    int y_offset;
    int x_offset;

    /* Determine the text and apply specific optical centering offsets */
    if (active_transport == ZMK_TRANSPORT_USB) {
        strcpy(text, LV_SYMBOL_USB);
        y_offset = 8;
    } else {
        /* Add 1 since profile index is 0-based but UI should be 1-based */
        snprintf(text, sizeof(text), "%d", (uint8_t)(zmk_ble_active_profile_index() + 1));
        y_offset = 6; // Nudge numbers slightly higher than the USB icon
    }

    // We apply a global +2px X offset since the physical screen bezel requires it
    x_offset = 2;

    /* Draw text into the designated area using the determined offsets */
    lv_canvas_draw_text(canvas, x_offset, y_offset, DISPLAY_WIDTH, &label_dsc, text);
}


/* ========================================================================= */
/*                    EVENT LISTENERS AND STATE UPDATES                      */
/* ========================================================================= */

/**
 * @brief Listens for endpoint and bluetooth profile changes.
 * When a change occurs, it triggers the notification overlay and starts the display timer.
 */
int multi_profile_changed_listener(const zmk_event_t *eh) {
    bool transport_changed = false;
    bool profile_changed = false;
    struct zmk_endpoint_instance endpoint = zmk_endpoints_selected();

    /* Check if the event is a bluetooth profile change */
    if (as_zmk_ble_active_profile_changed(eh)) {
        profile_changed = true;
    }

    /* Check if the event is a transport endpoint change (e.g., USB <-> BLE) */
    if (as_zmk_endpoint_changed(eh)) {
        transport_changed = true;
    }

    /* Process logic if either relevant event occurred */
    if (transport_changed || profile_changed) {
        /* Only trigger notification if we have completed initial boot (active_transport is initialized) */
        if (active_transport != TRANSPORT_UNINITIALIZED) {
            bool is_new_transport = (active_transport != endpoint.transport);
            bool is_new_ble_profile = (endpoint.transport == ZMK_TRANSPORT_BLE && profile_changed);

            if (is_new_transport || is_new_ble_profile) {
                profile_changed_active = true;

                /* Start or restart the timeout timer */
                k_work_reschedule(&profile_changed_timeout_work, K_MSEC(CONFIG_VISTA508_MESSAGE_DISPLAY_MS));

                /* Trigger an immediate redraw of the multi-widget */
                zmk_widget_multi_status_update_async();
            }
        }

        /* Update the tracked state to the new transport */
        active_transport = endpoint.transport;
    }

    return ZMK_EV_EVENT_BUBBLE;
}


ZMK_LISTENER(multi_profile_changed, multi_profile_changed_listener);
ZMK_SUBSCRIPTION(multi_profile_changed, zmk_endpoint_changed);
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(multi_profile_changed, zmk_ble_active_profile_changed);
#endif
