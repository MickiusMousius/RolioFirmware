/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * @file battery.c
 * @brief Implementation of the battery status widget, displaying battery level and charging state.
 * @details Listens to ZMK battery and USB connection state events. It parses both central and peripheral battery
 * levels, drawing them to a dedicated LVGL canvas along with charging animations when plugged in.
 */

/* ========================================================================= */
/*                        INCLUDES AND DEPENDENCIES                          */
/* ========================================================================= */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include "../status_screen.h"
#include "../styles.h"
#include "../util.h"
#include "battery.h"
#include <zmk/battery.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#if IS_ENABLED(CONFIG_ZMK_BQ24075)
#include <zmk/events/bq24075_state_changed.h>
#endif
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/usb.h>

/* ========================================================================= */
/*                            STATE AND GLOBALS                              */
/* ========================================================================= */
LV_IMG_DECLARE(bolt);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);
static lv_timer_t *battery_anim_timer = NULL;
static uint32_t battery_anim_tick = 0;
extern bool board_is_active; /* Exported by status_screen.c */

struct battery_state {
    uint8_t source;
    uint8_t level;
    bool usb_present;
};


/* ========================================================================= */
/*                        CHARGE STATE SOURCES                               */
/* ========================================================================= */
#if !IS_ENABLED(CONFIG_ZMK_BQ24075) && IS_ENABLED(CONFIG_VISTA508_PERIPHERAL_CHARGE_HEURISTIC)
#define PERIPHERAL_CHARGE_RISES 2 /* consecutive increases needed to infer charging */
#define PERIPHERAL_CHARGE_JUMP 3  /* a single increase this large infers charging immediately */

static struct {
    bool has_level;
    uint8_t last_level;
    uint8_t rises;
    bool charging;
    int64_t last_rise_ms;
} peripheral_charge;

static bool peripheral_charge_update(uint8_t level) {
    int64_t now = k_uptime_get();

    if (!peripheral_charge.has_level) {
        peripheral_charge.has_level = true;
    } else if (level > peripheral_charge.last_level) {
        peripheral_charge.rises++;
        peripheral_charge.last_rise_ms = now;
        if (peripheral_charge.rises >= PERIPHERAL_CHARGE_RISES ||
            level - peripheral_charge.last_level >= PERIPHERAL_CHARGE_JUMP) {
            peripheral_charge.charging = true;
        }
    } else if (level < peripheral_charge.last_level) {
        peripheral_charge.rises = 0;
        peripheral_charge.charging = false;
    }
    peripheral_charge.last_level = level;
    return peripheral_charge.charging;
}


/* Battery reports only arrive on change, so a stalled charge is detected at draw time */
static void peripheral_charge_expire(struct zmk_widget_battery_status *widget) {
    if (peripheral_charge.charging &&
        k_uptime_get() - peripheral_charge.last_rise_ms > CONFIG_VISTA508_PERIPHERAL_CHARGE_TIMEOUT_S * 1000LL) {
        peripheral_charge.charging = false;
        peripheral_charge.rises = 0;
        widget->batteries[1].usb_present = false;
    }
}


#endif

static bool half_usb_present(const struct zmk_widget_battery_status *widget, int i) {
#if IS_ENABLED(CONFIG_ZMK_BQ24075)
    return widget->bq24075_states[i].usb_present;
#else
    /* Central: zmk_usb_is_powered(). Peripheral: heuristic, or always false */
    return widget->batteries[i].usb_present;
#endif
}


static bool half_is_charging(const struct zmk_widget_battery_status *widget, int i) {
#if IS_ENABLED(CONFIG_ZMK_BQ24075)
    return widget->bq24075_states[i].is_charging;
#else
    return half_usb_present(widget, i) && widget->batteries[i].level < CONFIG_VISTA508_CHARGE_FULL_PCT;
#endif
}


/* ========================================================================= */
/*                         UI RENDERING AND DRAWING                          */
/* ========================================================================= */
static void draw_battery(lv_obj_t *canvas, uint8_t xOffset, struct battery_info batt_info, bool is_charging,
                         bool usb_present) {

    uint8_t yOffset = 1;
    uint8_t display_level = batt_info.level;

    if (is_charging && batt_info.level < 100) {
        uint8_t frames = (100 - batt_info.level + 9) / 10;
        uint8_t current_frame = battery_anim_tick % (frames + 1);
        display_level = batt_info.level + current_frame * 10;
        if (display_level > 100)
            display_level = 100;
    }

    lv_canvas_draw_rect(canvas, xOffset + 0, yOffset + 2, 29, 12, &style_rect_fg_square);
    lv_canvas_draw_rect(canvas, xOffset + 1, yOffset + 3, 27, 10, &style_rect_bg);
    lv_canvas_draw_rect(canvas, xOffset + 2, yOffset + 4, (display_level + 2) / 4, 8, &style_rect_fg_square);
    lv_canvas_draw_rect(canvas, xOffset + 29, yOffset + 5, 3, 6, &style_rect_fg_square);
    lv_canvas_draw_rect(canvas, xOffset + 28, yOffset + 6, 3, 4, &style_rect_bg);
    if (display_level >= 98) {
        lv_canvas_draw_rect(canvas, xOffset + 27, yOffset + 7, 3, 2, &style_rect_fg_square);
    }

    if (usb_present) {
        lv_draw_img_dsc_t img_dsc;
        lv_draw_img_dsc_init(&img_dsc);
        lv_canvas_draw_img(canvas, xOffset + 9, yOffset - 1, &bolt, &img_dsc);
    }
}


static void draw_battery_info(struct zmk_widget_battery_status *widget) {
    lv_obj_t *canvas = widget->canvas;

#if !IS_ENABLED(CONFIG_ZMK_BQ24075) && IS_ENABLED(CONFIG_VISTA508_PERIPHERAL_CHARGE_HEURISTIC)
    peripheral_charge_expire(widget);
#endif

    lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);

    char charge_text[6] = {};

    draw_battery(canvas, 0, widget->batteries[0], half_is_charging(widget, 0), half_usb_present(widget, 0));
    snprintf(charge_text, sizeof(charge_text), "%d", widget->batteries[0].level);
    lv_canvas_draw_text(canvas, 36, 0, 36, &style_label_16_left, charge_text);

    if (widget->batteries[1].level < 2) {
        if (lv_obj_get_y(widget->warning_art) < 0) {
            lv_obj_align(widget->warning_art, LV_ALIGN_TOP_RIGHT, 0, 0);
            lv_animimg_set_duration(widget->warning_art, 1000);
#ifdef CONFIG_ZMK_IDLE_TIMEOUT
            lv_animimg_set_repeat_count(widget->warning_art, CONFIG_ZMK_IDLE_TIMEOUT / 1000);
#else
            lv_animimg_set_repeat_count(widget->warning_art, 30);
#endif
            lv_animimg_start(widget->warning_art);
        }
    } else {
        if (lv_obj_get_y(widget->warning_art) >= 0) {
            // Hide and stop the animation
            lv_obj_align(widget->warning_art, LV_ALIGN_TOP_RIGHT, 0, -30);
            lv_animimg_set_repeat_count(widget->warning_art, 1);
            lv_animimg_start(widget->warning_art);
        }
    }

    draw_battery(canvas, 112, widget->batteries[1], half_is_charging(widget, 1), half_usb_present(widget, 1));
    snprintf(charge_text, sizeof(charge_text), "%d", widget->batteries[1].level);
    lv_canvas_draw_text(canvas, DISPLAY_WIDTH / 2, 0, 36, &style_label_16_right, charge_text);
}


static void battery_anim_timer_cb(lv_timer_t *timer) {
    battery_anim_tick++;
    struct zmk_widget_battery_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { draw_battery_info(widget); }
}


static void battery_draw_work_cb(struct k_work *work) {
    struct zmk_widget_battery_status *widget;
    bool central_usb = false;
    bool peripheral_usb = false;

    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        draw_battery_info(widget);
        if (half_usb_present(widget, 0))
            central_usb = true;
        if (half_usb_present(widget, 1))
            peripheral_usb = true;
    }

    bool should_run_timer = central_usb || (peripheral_usb && board_is_active);

    if (should_run_timer && battery_anim_timer == NULL) {
        battery_anim_timer = lv_timer_create(battery_anim_timer_cb, 500, NULL);
    } else if (!should_run_timer && battery_anim_timer != NULL) {
        lv_timer_del(battery_anim_timer);
        battery_anim_timer = NULL;
        battery_anim_tick = 0;
    }
}


K_WORK_DEFINE(battery_draw_work, battery_draw_work_cb);

/* ========================================================================= */
/*                    EVENT LISTENERS AND STATE UPDATES                      */
/* ========================================================================= */
static void set_battery_status(struct zmk_widget_battery_status *widget, struct battery_state state) {
    if (state.source >= 2)
        return;
    widget->batteries[state.source].level = state.level;
    widget->batteries[state.source].usb_present = state.usb_present;
    if (zmk_display_is_initialized()) {
        k_work_submit_to_queue(zmk_display_work_q(), &battery_draw_work);
    }
}


void battery_status_update_cb(struct battery_state state) {
    struct zmk_widget_battery_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_status(widget, state); }
}


static struct battery_state peripheral_battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev = as_zmk_peripheral_battery_state_changed(eh);
    return (struct battery_state){
        .source = ev->source + 1,
        .level = ev->state_of_charge,
#if !IS_ENABLED(CONFIG_ZMK_BQ24075) && IS_ENABLED(CONFIG_VISTA508_PERIPHERAL_CHARGE_HEURISTIC)
        .usb_present = peripheral_charge_update(ev->state_of_charge),
#endif
    };
}


static struct battery_state central_battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = (eh != NULL) ? as_zmk_battery_state_changed(eh) : NULL;
    return (struct battery_state){
        .source = 0,
        .level = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif
    };
}


static struct battery_state battery_status_get_state(const zmk_event_t *eh) {
    /* eh is NULL when the widget is first initialised */
    if (eh != NULL && as_zmk_peripheral_battery_state_changed(eh) != NULL) {
        return peripheral_battery_status_get_state(eh);
    } else {
        return central_battery_status_get_state(eh);
    }
}


ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct battery_state, battery_status_update_cb,
                            battery_status_get_state)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_peripheral_battery_state_changed);
ZMK_SUBSCRIPTION(widget_battery_status, zmk_battery_state_changed);
ZMK_SUBSCRIPTION(widget_battery_status, zmk_usb_conn_state_changed);

#if IS_ENABLED(CONFIG_ZMK_BQ24075)
static int bq24075_redraw_event_handler(const zmk_event_t *eh) {
    bool dirty = false;
    struct zmk_widget_battery_status *widget;

    if (as_zmk_bq24075_state_changed(eh) != NULL) {
        const struct zmk_bq24075_state_changed *ev = as_zmk_bq24075_state_changed(eh);
        SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
            widget->bq24075_states[0].is_charging = ev->is_charging;
            widget->bq24075_states[0].usb_present = ev->usb_present;
            widget->bq24075_states[0].charge_limit_mv = ev->charge_limit_mv;
            widget->bq24075_states[0].battery_mv = ev->battery_mv;
            dirty = true;
        }
    } else if (as_zmk_peripheral_bq24075_state_changed(eh) != NULL) {
        const struct zmk_peripheral_bq24075_state_changed *ev = as_zmk_peripheral_bq24075_state_changed(eh);
        SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
            widget->bq24075_states[ev->source + 1].is_charging = ev->is_charging;
            widget->bq24075_states[ev->source + 1].usb_present = ev->usb_present;
            widget->bq24075_states[ev->source + 1].charge_limit_mv = ev->charge_limit_mv;
            widget->bq24075_states[ev->source + 1].battery_mv = ev->battery_mv;
            dirty = true;
        }
    }

    if (dirty && zmk_display_is_initialized()) {
        k_work_submit_to_queue(zmk_display_work_q(), &battery_draw_work);
    }
    return ZMK_EV_EVENT_BUBBLE;
}


ZMK_LISTENER(widget_bq24075_redraw, bq24075_redraw_event_handler);
ZMK_SUBSCRIPTION(widget_bq24075_redraw, zmk_bq24075_state_changed);
ZMK_SUBSCRIPTION(widget_bq24075_redraw, zmk_peripheral_bq24075_state_changed);
#endif


/* ========================================================================= */
/*                              INITIALIZATION                               */
/* ========================================================================= */
int zmk_widget_battery_status_init(struct zmk_widget_battery_status *widget, lv_obj_t *canvas, lv_color_t *cbuf,
                                   lv_obj_t *warning_art) {
    widget->canvas = canvas;
    widget->cbuf = cbuf;
    widget->warning_art = warning_art;
    sys_slist_append(&widgets, &widget->node);
    widget_battery_status_init();
    return 0;
}
