/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * @file multi_widget.c
 * @brief Implementation of the multi-view widget that switches between different screens (typing, wpm, pairing,
 * settings) based on user interaction or state changes.
 * @details Subscribes to various ZMK events (WPM, Bluetooth passkey, layer, etc.) and routes them to draw specific
 * sub-views in the central display area. It manages timers to revert to default views like typing animation or WPM
 * chart when temporary views timeout.
 */

/* ========================================================================= */
/*                        INCLUDES AND DEPENDENCIES                          */
/* ========================================================================= */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include "../status_screen.h"
#include "../styles.h"
#include "multi_widget.h"
#include "multi_widget/views.h"
#include <zephyr/settings/settings.h>
#include <zmk/ble.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/wpm.h>

/* ========================================================================= */
/*                            STATE AND GLOBALS                              */
/* ========================================================================= */
extern bool board_is_active;

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);


enum multi_widget_display_mode {
    MULTI_WIDGET_MODE_WPM_STEVE,
    MULTI_WIDGET_MODE_WPM_GRAPH,
    MULTI_WIDGET_MODE_MODIFIERS
};


static enum multi_widget_display_mode display_mode = MULTI_WIDGET_MODE_WPM_GRAPH;
static bool wpm_mode_text_active = false;
static void wpm_mode_text_timeout_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(wpm_mode_text_timeout_work, wpm_mode_text_timeout_handler);

#if IS_ENABLED(CONFIG_SETTINGS)
static void wpm_mode_save_work_fn(struct k_work *_work);
static K_WORK_DELAYABLE_DEFINE(wpm_mode_save_work, wpm_mode_save_work_fn);

static void mod_auto_save_work_fn(struct k_work *_work);
static K_WORK_DELAYABLE_DEFINE(mod_auto_save_work, mod_auto_save_work_fn);
#endif

static bool mod_auto_show = true;
static bool mod_auto_text_active = false;
static void mod_auto_text_timeout_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(mod_auto_text_timeout_work, mod_auto_text_timeout_handler);

/* ========================================================================= */
/*                         UI RENDERING AND DRAWING                          */
/* ========================================================================= */
static void draw_multi_widget(struct zmk_widget_multi_status *widget) {
    lv_obj_t *canvas = widget->canvas;

    /* 1. Draw base background and border */
    lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);
    const int WIDGET_HEIGHT = 82;
    lv_canvas_draw_rect(canvas, 1, 1, DISPLAY_WIDTH - 1, WIDGET_HEIGHT, &style_rect_border);

    /* 2. Handle inactive board / sleep state */
    if (!board_is_active && display_mode == MULTI_WIDGET_MODE_WPM_STEVE) {
        draw_sleep_image(canvas);
        return;
    }

    /* 3. Handle one-time boot animation */
    if (hello_animation_is_active() && display_mode != MULTI_WIDGET_MODE_WPM_GRAPH) {
        draw_hello_animation(canvas);
        return;
    }

    /* 4. Handle Bluetooth pairing and profile states */
    if (pairing_celebration_is_active()) {
        draw_dance_animation(canvas);
        return;
    }
    if (pairing_is_active()) {
        draw_pairing_passkey(canvas);
        return;
    }
    if (pairing_failed_is_active()) {
        draw_pairing_failed(canvas);
        return;
    }
    if (profile_cleared_is_active()) {
        draw_profile_cleared(canvas);
        return;
    }
    if (profile_disconnected_is_active()) {
        draw_profile_disconnected(canvas);
        return;
    }
    if (profile_changed_is_active()) {
        draw_profile_changed(canvas);
        return;
    }

    /* 5. Handle settings layer notifications (WPM mode & modifier auto show toggles) */
    bool is_settings_layer = (widget->layer_index == CONFIG_VISTA508_SETTINGS_LAYER);

    if (wpm_mode_text_active && is_settings_layer) {
        draw_wpm_text_notification(canvas, display_mode);
        return;
    }
    if (mod_auto_text_active && is_settings_layer) {
        draw_mod_auto_text_notification(canvas, mod_auto_show);
        return;
    }

    /* 6. Default view: WPM Chart, Modifiers or Typing Animation */
    bool show_mods_auto = mod_auto_show && modifier_indicator_is_active();

    if (display_mode == MULTI_WIDGET_MODE_WPM_GRAPH && !show_mods_auto) {
        draw_wpm_chart(canvas);
    } else if (display_mode == MULTI_WIDGET_MODE_MODIFIERS || show_mods_auto) {
        lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);
        draw_modifier_indicator(canvas);
    } else {
        draw_typing_animation(canvas);
    }
}


static void layer_status_update_cb(uint8_t index) {
    struct zmk_widget_multi_status *widget;
    bool needs_redraw = false;

    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        bool previous_was_settings = (widget->layer_index == CONFIG_VISTA508_SETTINGS_LAYER);
        widget->layer_index = index;

        bool now_is_settings = (widget->layer_index == CONFIG_VISTA508_SETTINGS_LAYER);

        if (previous_was_settings != now_is_settings) {
            needs_redraw = true;
        }
    }

    if (needs_redraw) {
        zmk_widget_multi_status_update_async();
    }
}


int multi_layer_status_listener(const zmk_event_t *eh) {
    uint8_t index = zmk_keymap_highest_layer_active();
    layer_status_update_cb(index);
    return ZMK_EV_EVENT_BUBBLE;
}


ZMK_LISTENER(multi_layer_status, multi_layer_status_listener);
ZMK_SUBSCRIPTION(multi_layer_status, zmk_layer_state_changed);


void vista508_wpm_mode_toggle(void) {
    display_mode = (display_mode + 1) % 3;

    struct zmk_widget_multi_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        if (widget->layer_index == CONFIG_VISTA508_SETTINGS_LAYER) {
            wpm_mode_text_active = true;
            k_work_reschedule(&wpm_mode_text_timeout_work, K_MSEC(CONFIG_VISTA508_MESSAGE_DISPLAY_MS));
        }
    }
    zmk_widget_multi_status_update_async();

#if IS_ENABLED(CONFIG_SETTINGS)
    k_work_reschedule(&wpm_mode_save_work, K_MSEC(500));
#endif
}


static void wpm_mode_text_timeout_handler(struct k_work *work) {
    wpm_mode_text_active = false;
    zmk_widget_multi_status_update_async();
}


void vista508_mod_auto_show_toggle(void) {
    mod_auto_show = !mod_auto_show;

    struct zmk_widget_multi_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        if (widget->layer_index == CONFIG_VISTA508_SETTINGS_LAYER) {
            mod_auto_text_active = true;
            k_work_reschedule(&mod_auto_text_timeout_work, K_MSEC(CONFIG_VISTA508_MESSAGE_DISPLAY_MS));
        }
    }
    zmk_widget_multi_status_update_async();

#if IS_ENABLED(CONFIG_SETTINGS)
    k_work_reschedule(&mod_auto_save_work, K_MSEC(500));
#endif
}


static void mod_auto_text_timeout_handler(struct k_work *work) {
    mod_auto_text_active = false;
    zmk_widget_multi_status_update_async();
}


#if IS_ENABLED(CONFIG_SETTINGS)
static int wpm_mode_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    const char *next;
    if (settings_name_steq(name, "graph", &next) && !next) {
        if (len != sizeof(display_mode))
            return -EINVAL;
        return read_cb(cb_arg, &display_mode, sizeof(display_mode));
    }
    return -ENOENT;
}


static int mod_auto_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    const char *next;
    if (settings_name_steq(name, "auto", &next) && !next) {
        if (len != sizeof(mod_auto_show))
            return -EINVAL;
        return read_cb(cb_arg, &mod_auto_show, sizeof(mod_auto_show));
    }
    return -ENOENT;
}


SETTINGS_STATIC_HANDLER_DEFINE(vista508_wpm_mode, "vista508/wpm_mode", NULL, wpm_mode_settings_set, NULL, NULL);
SETTINGS_STATIC_HANDLER_DEFINE(vista508_mod_auto, "vista508/mod_auto", NULL, mod_auto_settings_set, NULL, NULL);

static void wpm_mode_save_work_fn(struct k_work *_work) {
    settings_save_one("vista508/wpm_mode/graph", &display_mode, sizeof(display_mode));
}


static void mod_auto_save_work_fn(struct k_work *_work) {
    settings_save_one("vista508/mod_auto/auto", &mod_auto_show, sizeof(mod_auto_show));
}


#endif


/* ========================================================================= */
/*                              INITIALIZATION                               */
/* ========================================================================= */
static void multi_widget_draw_work_cb(struct k_work *work) { zmk_widget_multi_status_update(); }


K_WORK_DEFINE(multi_widget_draw_work, multi_widget_draw_work_cb);

void zmk_widget_multi_status_update_async(void) {
    if (zmk_display_is_initialized()) {
        k_work_submit_to_queue(zmk_display_work_q(), &multi_widget_draw_work);
    }
}


sys_slist_t *zmk_widget_multi_get_list(void) { return &widgets; }


bool zmk_widget_multi_is_wpm_chart_active(void) {
    bool show_mods_auto = mod_auto_show && modifier_indicator_is_active();
    return display_mode == MULTI_WIDGET_MODE_WPM_GRAPH && !show_mods_auto;
}


bool zmk_widget_multi_should_redraw_for_modifiers(void) {
    if (display_mode == MULTI_WIDGET_MODE_MODIFIERS)
        return true;
    if (mod_auto_show)
        return true;
    return false;
}


/*
 * This routine is called whenever the board transitions to IDLE/SLEEP state.
 * LVGL timers run autonomously in the background and can prevent the MCU from
 * entering deep sleep if they continue firing. This explicitly halts all
 * visual animation timers so the CPU can rest.
 */
void zmk_widget_multi_halt_animations(void) {
    typing_animation_stop();
    hello_animation_stop();
    pairing_animations_stop();
}


int zmk_widget_multi_status_init(struct zmk_widget_multi_status *widget, lv_obj_t *canvas, lv_color_t *cbuf) {
    widget->canvas = canvas;
    widget->cbuf = cbuf;
    sys_slist_append(&widgets, &widget->node);

    hello_animation_init();
    pairing_init();
    typing_animation_init();
    modifier_indicator_init();

    draw_multi_widget(widget);

    return 0;
}


void zmk_widget_multi_status_update(void) {
    struct zmk_widget_multi_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { draw_multi_widget(widget); }
}
