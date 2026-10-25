/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * @file layer.c
 * @brief Implementation of the active layer widget, updating the display based on the current ZMK keymap layer.
 * @details Listens to ZMK layer state changes and uses the keymap API to fetch the highest active layer's name,
 * rendering it onto the bottom LVGL canvas section.
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
#include "layer.h"
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>

/* ========================================================================= */
/*                            STATE AND GLOBALS                              */
/* ========================================================================= */
static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct layer_status_state {
    uint8_t index;
    const char *label;
};


/* ========================================================================= */
/*                         UI RENDERING AND DRAWING                          */
/* ========================================================================= */
static void draw_layer_info(struct zmk_widget_layer_status *widget) {
    lv_obj_t *canvas = widget->canvas;

    // Fill background
    lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);

    // Draw layer
    if (widget->layer_label == NULL) {
        char text[10] = {};
        snprintf(text, sizeof(text), "LAYER %i", widget->layer_index);
        lv_canvas_draw_text(canvas, 0, 1, DISPLAY_WIDTH, &style_label_18_center, text);
    } else {
        lv_canvas_draw_text(canvas, 0, 1, DISPLAY_WIDTH, &style_label_18_center, widget->layer_label);
    }
}


/* ========================================================================= */
/*                    EVENT LISTENERS AND STATE UPDATES                      */
/* ========================================================================= */
static void set_layer_status(struct zmk_widget_layer_status *widget, struct layer_status_state state) {
    widget->layer_index = state.index;
    widget->layer_label = state.label;
    draw_layer_info(widget);
}


static void layer_status_update_cb(struct layer_status_state state) {
    struct zmk_widget_layer_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer_status(widget, state); }
}


static struct layer_status_state layer_status_get_state(const zmk_event_t *eh) {
    uint8_t index = zmk_keymap_highest_layer_active();
    return (struct layer_status_state){.index = index, .label = zmk_keymap_layer_name(index)};
}


ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_status, struct layer_status_state, layer_status_update_cb,
                            layer_status_get_state)
ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);

/* ========================================================================= */
/*                              INITIALIZATION                               */
/* ========================================================================= */
int zmk_widget_layer_status_init(struct zmk_widget_layer_status *widget, lv_obj_t *canvas, lv_color_t *cbuf) {
    widget->canvas = canvas;
    widget->cbuf = cbuf;
    sys_slist_append(&widgets, &widget->node);
    widget_layer_status_init();
    return 0;
}
