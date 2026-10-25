/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * @file battery.c
 * @brief Implementation of the battery status widget, displaying the central's battery level and the peripheral's
 * connection state.
 * @details Listens to ZMK battery and USB connection state events, drawing the central's battery level to a dedicated
 * LVGL canvas along with a charging animation when plugged in. The peripheral has its own display showing its battery,
 * so only its connection state is shown here, along with a warning should its battery run critically low.
 */

/* ========================================================================= */
/*                        INCLUDES AND DEPENDENCIES                          */
/* ========================================================================= */
#include <zephyr/bluetooth/conn.h>
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
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/usb.h>

/* ========================================================================= */
/*                            STATE AND GLOBALS                              */
/* ========================================================================= */
LV_IMG_DECLARE(bolt);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);
static lv_timer_t *battery_anim_timer = NULL;
static uint32_t battery_anim_tick = 0;

/* The peripheral's battery level is only meaningful once it has reported one over the split connection */
static bool peripheral_level_known = false;
static bool split_connected = false;

struct battery_state {
    uint8_t source;
    uint8_t level;
    bool usb_present;
};


static bool central_is_charging(const struct zmk_widget_battery_status *widget) {
    return widget->batteries[0].usb_present && widget->batteries[0].level < CONFIG_VISTA508_CHARGE_FULL_PCT;
}


/* ========================================================================= */
/*                         SPLIT CONNECTION STATE                            */
/* ========================================================================= */
#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE) && IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
/* Connections where we are the BT central are to the split peripheral, host connections are the other way around */
static void count_split_connection(struct bt_conn *conn, void *data) {
    struct bt_conn_info info;
    if (bt_conn_get_info(conn, &info) == 0 && info.role == BT_CONN_ROLE_CENTRAL &&
        info.state == BT_CONN_STATE_CONNECTED) {
        (*(int *)data)++;
    }
}


static bool split_is_connected(void) {
    int count = 0;
    bt_conn_foreach(BT_CONN_TYPE_LE, count_split_connection, &count);
    return count > 0;
}


#else
static bool split_is_connected(void) { return false; }
#endif


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

    lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);

    char charge_text[6] = {};

    draw_battery(canvas, 0, widget->batteries[0], central_is_charging(widget), widget->batteries[0].usb_present);
    snprintf(charge_text, sizeof(charge_text), "%d", widget->batteries[0].level);
    lv_canvas_draw_text(canvas, 36, 0, 36, &style_label_16_left, charge_text);

    if (split_connected && peripheral_level_known && widget->batteries[1].level < 2) {
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

    lv_canvas_draw_text(canvas, 0, 0, DISPLAY_WIDTH, &style_label_16_right,
                        split_connected ? LV_SYMBOL_OK : LV_SYMBOL_CLOSE);
}


static void battery_anim_timer_cb(lv_timer_t *timer) {
    battery_anim_tick++;
    struct zmk_widget_battery_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { draw_battery_info(widget); }
}


static void battery_draw_work_cb(struct k_work *work) {
    struct zmk_widget_battery_status *widget;
    bool central_usb = false;

    split_connected = split_is_connected();

    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        draw_battery_info(widget);
        if (widget->batteries[0].usb_present)
            central_usb = true;
    }

    if (central_usb && battery_anim_timer == NULL) {
        battery_anim_timer = lv_timer_create(battery_anim_timer_cb, 500, NULL);
    } else if (!central_usb && battery_anim_timer != NULL) {
        lv_timer_del(battery_anim_timer);
        battery_anim_timer = NULL;
        battery_anim_tick = 0;
    }
}


K_WORK_DEFINE(battery_draw_work, battery_draw_work_cb);

static void battery_redraw_async(void) {
    if (zmk_display_is_initialized()) {
        k_work_submit_to_queue(zmk_display_work_q(), &battery_draw_work);
    }
}


/* ========================================================================= */
/*                    EVENT LISTENERS AND STATE UPDATES                      */
/* ========================================================================= */
static void set_battery_status(struct zmk_widget_battery_status *widget, struct battery_state state) {
    if (state.source >= 2)
        return;
    widget->batteries[state.source].level = state.level;
    widget->batteries[state.source].usb_present = state.usb_present;
    battery_redraw_async();
}


void battery_status_update_cb(struct battery_state state) {
    struct zmk_widget_battery_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_status(widget, state); }
}


static struct battery_state peripheral_battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev = as_zmk_peripheral_battery_state_changed(eh);
    peripheral_level_known = true;
    return (struct battery_state){
        .source = ev->source + 1,
        .level = ev->state_of_charge,
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

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE) && IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
/* Called from the Bluetooth stack, so the connection state is read & drawn later on the display work queue */
static void split_conn_connected(struct bt_conn *conn, uint8_t err) { battery_redraw_async(); }


static void split_conn_disconnected(struct bt_conn *conn, uint8_t reason) { battery_redraw_async(); }


BT_CONN_CB_DEFINE(widget_battery_split_conn_callbacks) = {
    .connected = split_conn_connected,
    .disconnected = split_conn_disconnected,
};


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
    split_connected = split_is_connected();
    widget_battery_status_init();
    return 0;
}
