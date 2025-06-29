/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/battery.h>
#include <zmk/display.h>
#include "status.h"
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include <zmk/wpm.h>
#include <zmk/split/central.h>


#define SOURCE_OFFSET 1


static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct battery_state {
    uint8_t source;
    uint8_t level;
    bool usb_present;
};

struct battery_object {
    lv_obj_t *symbol;
    lv_obj_t *label;
} battery_objects[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET];
    
static lv_color_t battery_image_buffer[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET][5 * 8];


struct output_status_state {
    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
};

struct layer_status_state {
    uint8_t index;
    const char *label;
};

struct wpm_status_state {
    uint8_t wpm;
};

static void draw_battery(lv_obj_t *canvas, uint8_t level, bool usb_present) {
    return;
    // lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);
    
    // lv_draw_rect_dsc_t rect_fill_dsc;
    // lv_draw_rect_dsc_init(&rect_fill_dsc);

    // if (usb_present) {
    //     rect_fill_dsc.bg_opa = LV_OPA_TRANSP;
    //     rect_fill_dsc.border_color = lv_color_white();
    //     rect_fill_dsc.border_width = 1;
    // }

    // lv_canvas_set_px(canvas, 0, 0, lv_color_white());
    // lv_canvas_set_px(canvas, 4, 0, lv_color_white());

    // if (level <= 10 || usb_present) {
    //     lv_canvas_draw_rect(canvas, 1, 2, 3, 5, &rect_fill_dsc);
    // } else if (level <= 30) {
    //     lv_canvas_draw_rect(canvas, 1, 2, 3, 4, &rect_fill_dsc);
    // } else if (level <= 50) {
    //     lv_canvas_draw_rect(canvas, 1, 2, 3, 3, &rect_fill_dsc);
    // } else if (level <= 70) {
    //     lv_canvas_draw_rect(canvas, 1, 2, 3, 2, &rect_fill_dsc);
    // } else if (level <= 90) {
    //     lv_canvas_draw_rect(canvas, 1, 2, 3, 1, &rect_fill_dsc);
    // }
}


static void draw_wpm(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 1);

    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_16, LV_TEXT_ALIGN_RIGHT);
    lv_draw_label_dsc_t label_dsc_wpm;
    init_label_dsc(&label_dsc_wpm, LVGL_FOREGROUND, &lv_font_montserrat_12, LV_TEXT_ALIGN_RIGHT);
    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);
    lv_draw_rect_dsc_t rect_white_dsc;
    init_rect_dsc(&rect_white_dsc, LVGL_FOREGROUND);
    lv_draw_line_dsc_t line_thick_dsc;
    init_line_dsc(&line_thick_dsc, LVGL_FOREGROUND, 2);
    lv_draw_arc_dsc_t arc_dsc;
    init_arc_dsc(&arc_dsc, LVGL_FOREGROUND, 2);
    lv_draw_line_dsc_t line_dsc;
    init_line_dsc(&line_dsc, LVGL_FOREGROUND, 1);

    // Fill background
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_black_dsc);

    // Draw WPM
    const uint8_t yOffset = 0;
    const int WPM_HEIGHT = 82;
    const uint8_t cornerRadius = 4;
    // Draw the WPM Boundary - top
    lv_point_t boxPoints[2];
    boxPoints[0].x = 1 + cornerRadius;
    boxPoints[0].y = yOffset;
    boxPoints[1].x = CANVAS_SIZE - cornerRadius - 1;
    boxPoints[1].y = yOffset;
    lv_canvas_draw_line(canvas, boxPoints, 2, &line_thick_dsc);
    lv_canvas_draw_arc(canvas, boxPoints[0].x, boxPoints[0].y + cornerRadius, cornerRadius, 180, 270, &arc_dsc);
    lv_canvas_draw_arc(canvas, boxPoints[1].x, boxPoints[1].y + cornerRadius, cornerRadius, 270, 0, &arc_dsc);
    // Draw the WPM Boundary - bottom
    boxPoints[0].y = yOffset + WPM_HEIGHT;
    boxPoints[1].y = yOffset + WPM_HEIGHT;
    lv_canvas_draw_line(canvas, boxPoints, 2, &line_thick_dsc);
    lv_canvas_draw_arc(canvas, boxPoints[0].x, boxPoints[0].y - cornerRadius, cornerRadius, 90, 180, &arc_dsc);
    lv_canvas_draw_arc(canvas, boxPoints[1].x, boxPoints[1].y - cornerRadius, cornerRadius, 0, 90, &arc_dsc);
    // Draw the sides
    boxPoints[0].x = 1;
    boxPoints[0].y = cornerRadius + yOffset;
    boxPoints[1].x = 1;
    boxPoints[1].y = yOffset + WPM_HEIGHT - cornerRadius;
    lv_canvas_draw_line(canvas, boxPoints, 2, &line_thick_dsc);
    boxPoints[0].x = CANVAS_SIZE - 1;
    boxPoints[1].x = CANVAS_SIZE - 1;
    lv_canvas_draw_line(canvas, boxPoints, 2, &line_thick_dsc);

    char wpm_text[6] = {};
    snprintf(wpm_text, sizeof(wpm_text), "%d", state->wpm[WPM_SAMPLES - 1]);
    lv_canvas_draw_text(canvas, CANVAS_SIZE - 29, 3 + WPM_HEIGHT - 21, 24, &label_dsc_wpm, wpm_text);

    int max = 0;
    int min = 256;

    for (int i = 0; i < WPM_SAMPLES; i++) {
        if (state->wpm[i] > max) {
            max = state->wpm[i];
        }
        if (state->wpm[i] < min) {
            min = state->wpm[i];
        }
    }

    int range = max - min;
    if (range == 0) {
        range = 1;
    }

    const int WPM_COLUMN_WIDTH = (CANVAS_SIZE - 4) / WPM_SAMPLES;
    lv_point_t points[WPM_SAMPLES];
    for (int i = 0; i < WPM_SAMPLES; i++) {
        points[i].x = 3 + i * WPM_COLUMN_WIDTH;
        points[i].y = yOffset - 1 + WPM_HEIGHT - (state->wpm[i] - min) * (WPM_HEIGHT - 7) / range;
    }
    lv_canvas_draw_line(canvas, points, WPM_SAMPLES, &line_dsc);
}

static void draw_output_info(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 2);

    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);
    lv_draw_rect_dsc_t rect_white_dsc;
    init_rect_dsc(&rect_white_dsc, LVGL_FOREGROUND);
    lv_draw_arc_dsc_t arc_dsc;
    init_arc_dsc(&arc_dsc, LVGL_FOREGROUND, 2);
    lv_draw_arc_dsc_t arc_dsc_thin;
    init_arc_dsc(&arc_dsc_thin, LVGL_FOREGROUND, 1);
    lv_draw_arc_dsc_t arc_dsc_filled;
    init_arc_dsc(&arc_dsc_filled, LVGL_FOREGROUND, 9);
    lv_draw_arc_dsc_t arc_dsc_filled_background;
    init_arc_dsc(&arc_dsc_filled_background, LVGL_BACKGROUND, 9);
    lv_draw_line_dsc_t line_dsc;
    init_line_dsc(&line_dsc, LVGL_FOREGROUND, 2);
    lv_draw_line_dsc_t line_thick_dsc;
    init_line_dsc(&line_thick_dsc, LVGL_FOREGROUND, 5);
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_18, LV_TEXT_ALIGN_CENTER);
    lv_draw_label_dsc_t status_dsc;
    init_label_dsc(&status_dsc, LVGL_FOREGROUND, &lv_font_montserrat_28, LV_TEXT_ALIGN_CENTER);
    lv_draw_label_dsc_t label_dsc_black;
    init_label_dsc(&label_dsc_black, LVGL_BACKGROUND, &lv_font_montserrat_18, LV_TEXT_ALIGN_CENTER);

    // Fill background
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_black_dsc);

    bool usingUsb = false;
    bool profileConnected = false;
    bool profileAdvertising = false;
    if (state->selected_endpoint.transport == ZMK_TRANSPORT_USB) {
        usingUsb = true;
    }
    if (state->active_profile_bonded) {
        if (state->active_profile_connected) {
            profileConnected = true;
        }
    } else {
        profileAdvertising = true;
    }
 
    if (usingUsb) {
        // Draw the pill outline
        lv_canvas_draw_arc(canvas, 15, 14, 13, 90, 270, &arc_dsc);
        lv_canvas_draw_arc(canvas, 95, 14, 13, 270, 90, &arc_dsc);
        lv_point_t points[2];
        points[0].x = 15;
        points[0].y = 2;
        points[1].x = 95;
        points[1].y = 2;
        lv_canvas_draw_line(canvas, points, 2, &line_dsc);
        points[0].y = 26;
        points[1].y = 26;
        lv_canvas_draw_line(canvas, points, 2, &line_dsc);
        // Draw the pill fill
        lv_canvas_draw_arc(canvas, 15, 14, 9, 0, 359, &arc_dsc_filled);
        lv_canvas_draw_arc(canvas, 95, 14, 9, 0, 359, &arc_dsc_filled);
        lv_canvas_draw_rect(canvas, 15, 5, 80, 18, &rect_white_dsc);
        // Draw the label
        lv_canvas_draw_text(canvas, 5, 4, 100, &label_dsc_black, "USB Out");
        // Draw the current profile indicator
        uint8_t xOffset = 129;
        uint8_t yOffset = 14;
        char profileNumber[4] = {};
        sprintf(profileNumber, "%d", (uint8_t)state->active_profile_index + 1);
        if (profileConnected) {
            lv_canvas_draw_arc(canvas, xOffset, yOffset, 13, 0, 359, &arc_dsc);
            lv_canvas_draw_arc(canvas, xOffset, yOffset, 9, 0, 359, &arc_dsc_filled);
            lv_canvas_draw_text(canvas, xOffset - 8, yOffset - 11, 16, &label_dsc_black, profileNumber);
        }
        else if (profileAdvertising) {
            lv_canvas_draw_text(canvas, xOffset - 8, yOffset - 15, 16, &status_dsc, LV_SYMBOL_SETTINGS);
            lv_canvas_draw_arc(canvas, xOffset, yOffset, 9, 0, 359, &arc_dsc_filled);
            lv_canvas_draw_text(canvas, xOffset - 8, yOffset - 11, 16, &label_dsc_black, profileNumber);
        }
        else {
            lv_canvas_draw_arc(canvas, xOffset, yOffset, 13, 0, 359, &arc_dsc_thin);
            lv_canvas_draw_arc(canvas, xOffset, yOffset, 10, 0, 359, &arc_dsc_thin);
            lv_canvas_draw_text(canvas, xOffset - 8, yOffset - 11, 16, &label_dsc, profileNumber);
        }
        return;
    }

    // Draw circles
    uint8_t xOffset = 15;
    uint8_t yOffset = 15;
    for (int i = 0; i < 5; i++) {
        bool selected = i == state->active_profile_index;

        char label[2];
        snprintf(label, sizeof(label), "%d", i + 1);

        if (selected) {
            if (profileAdvertising) {
                lv_canvas_draw_text(canvas, xOffset - 8, yOffset - 15, 16, &status_dsc, LV_SYMBOL_SETTINGS);
                lv_canvas_draw_arc(canvas, xOffset, yOffset, 9, 0, 359, &arc_dsc_filled);
                lv_canvas_draw_text(canvas, xOffset - 8, yOffset - 11, 16, &label_dsc_black, label);
            }
            else if (profileConnected) {
                lv_canvas_draw_arc(canvas, xOffset, yOffset, 13, 0, 359, &arc_dsc);
                lv_canvas_draw_arc(canvas, xOffset, yOffset, 9, 0, 359, &arc_dsc_filled);
                lv_canvas_draw_text(canvas, xOffset - 8, yOffset - 11, 16, &label_dsc_black, label);
            }
            else {
                lv_canvas_draw_arc(canvas, xOffset, yOffset, 13, 0, 359, &arc_dsc_thin);
                lv_canvas_draw_arc(canvas, xOffset, yOffset, 10, 0, 359, &arc_dsc_thin);
                lv_canvas_draw_text(canvas, xOffset - 8, yOffset - 11, 16, &label_dsc, label);
            }
        }
        else {
            lv_canvas_draw_arc(canvas, xOffset, yOffset, 13, 0, 359, &arc_dsc);
            lv_canvas_draw_text(canvas, xOffset - 8, yOffset - 11, 16, &label_dsc, label);
        }
        xOffset +=29;
    }
}

static void draw_layer_info(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 3);

    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_18, LV_TEXT_ALIGN_CENTER);

    // Fill background
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_black_dsc);

    // Draw layer
    if (state->layer_label == NULL) {
        char text[10] = {};

        sprintf(text, "LAYER %i", state->layer_index);

        lv_canvas_draw_text(canvas, 0, 5, CANVAS_SIZE, &label_dsc, text);
    } else {
        lv_canvas_draw_text(canvas, 0, 5, CANVAS_SIZE, &label_dsc, state->layer_label);
    }
}





static void set_battery_status(lv_obj_t *widget, struct battery_state state) {
    if (state.source >= ZMK_SPLIT_BLE_PERIPHERAL_COUNT + SOURCE_OFFSET) {
        return;
    }
    LOG_DBG("DERP DERP DERP source: %d, level: %d, usb: %d", state.source, state.level, state.usb_present);
    lv_obj_t *symbol = battery_objects[state.source].symbol;
    lv_obj_t *label = battery_objects[state.source].label;

    // draw_battery(symbol, state.level, state.usb_present);
    // lv_label_set_text_fmt(label, "%4u%%", state.level);
    
    // if (state.level > 0 || state.usb_present) {
    //     lv_obj_clear_flag(symbol, LV_OBJ_FLAG_HIDDEN);
    //     lv_obj_move_foreground(symbol);
    //     lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
    //     lv_obj_move_foreground(label);
    // } else {
    //     lv_obj_add_flag(symbol, LV_OBJ_FLAG_HIDDEN);
    //     lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
    // }
}

void battery_status_update_cb(struct battery_state state) {
    struct zmk_widget_dongle_battery_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_status(widget->obj, state); }
}

static struct battery_state peripheral_battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev = as_zmk_peripheral_battery_state_changed(eh);
    return (struct battery_state){
        .source = ev->source + SOURCE_OFFSET,
        .level = ev->state_of_charge,
    };
}

static struct battery_state central_battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    return (struct battery_state) {
        .source = 0,
        .level = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
    };
}

static struct battery_state battery_status_get_state(const zmk_event_t *eh) { 
    if (as_zmk_peripheral_battery_state_changed(eh) != NULL) {
        return peripheral_battery_status_get_state(eh);
    } else {
        return central_battery_status_get_state(eh);
    }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_dongle_battery_status, struct battery_state,
                            battery_status_update_cb, battery_status_get_state)

ZMK_SUBSCRIPTION(widget_dongle_battery_status, zmk_peripheral_battery_state_changed);
ZMK_SUBSCRIPTION(widget_dongle_battery_status, zmk_battery_state_changed);
ZMK_SUBSCRIPTION(widget_dongle_battery_status, zmk_usb_conn_state_changed);



int zmk_widget_dongle_battery_status_init(struct zmk_widget_dongle_battery_status *widget, lv_obj_t *parent) {
    // widget->obj = lv_obj_create(parent);

    // lv_obj_set_size(widget->obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    
    // for (int i = 0; i < ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET; i++) {
    //     lv_obj_t *image_canvas = lv_canvas_create(widget->obj);
    //     lv_obj_t *battery_label = lv_label_create(widget->obj);

    //     lv_canvas_set_buffer(image_canvas, battery_image_buffer[i], 5, 8, LV_IMG_CF_TRUE_COLOR);

    //     lv_obj_align(image_canvas, LV_ALIGN_TOP_RIGHT, 0, i * 10);
    //     lv_obj_align(battery_label, LV_ALIGN_TOP_RIGHT, -7, i * 10);

    //     lv_obj_add_flag(image_canvas, LV_OBJ_FLAG_HIDDEN);
    //     lv_obj_add_flag(battery_label, LV_OBJ_FLAG_HIDDEN);
        
    //     battery_objects[i] = (struct battery_object){
    //         .symbol = image_canvas,
    //         .label = battery_label,
    //     };
    // }

    // sys_slist_append(&widgets, &widget->node);

    widget_dongle_battery_status_init();

    return 0;
}

lv_obj_t *zmk_widget_dongle_battery_status_obj(struct zmk_widget_dongle_battery_status *widget) {
    return widget->obj;
}



static void set_output_status(struct zmk_widget_status *widget,
                              const struct output_status_state *state) {
    widget->state.selected_endpoint = state->selected_endpoint;
    widget->state.active_profile_index = state->active_profile_index;
    widget->state.active_profile_connected = state->active_profile_connected;
    widget->state.active_profile_bonded = state->active_profile_bonded;

    draw_wpm(widget->obj, widget->cbuf1, &widget->state);
    draw_output_info(widget->obj, widget->cbuf2, &widget->state);
}

static void output_status_update_cb(struct output_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_output_status(widget, &state); }
}

static struct output_status_state output_status_get_state(const zmk_event_t *_eh) {
    return (struct output_status_state){
        .selected_endpoint = zmk_endpoints_selected(),
        .active_profile_index = zmk_ble_active_profile_index(),
        .active_profile_connected = zmk_ble_active_profile_is_connected(),
        .active_profile_bonded = !zmk_ble_active_profile_is_open(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, output_status_get_state)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);
#endif
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
#endif

static void set_layer_status(struct zmk_widget_status *widget, struct layer_status_state state) {
    widget->state.layer_index = state.index;
    widget->state.layer_label = state.label;

    draw_layer_info(widget->obj, widget->cbuf3, &widget->state);
}

static void layer_status_update_cb(struct layer_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer_status(widget, state); }
}

static struct layer_status_state layer_status_get_state(const zmk_event_t *eh) {
    uint8_t index = zmk_keymap_highest_layer_active();
    return (struct layer_status_state){.index = index, .label = zmk_keymap_layer_name(index)};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_status, struct layer_status_state, layer_status_update_cb,
                            layer_status_get_state)

ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);

static void set_wpm_status(struct zmk_widget_status *widget, struct wpm_status_state state) {
    for (int i = 0; i < WPM_SAMPLES - 1; i++) {
        widget->state.wpm[i] = widget->state.wpm[i + 1];
    }
    widget->state.wpm[WPM_SAMPLES - 1] = state.wpm;

    draw_wpm(widget->obj, widget->cbuf1, &widget->state);
}

static void wpm_status_update_cb(struct wpm_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_wpm_status(widget, state); }
}

struct wpm_status_state wpm_status_get_state(const zmk_event_t *eh) {
    return (struct wpm_status_state){.wpm = zmk_wpm_get_state()};
};

ZMK_DISPLAY_WIDGET_LISTENER(widget_wpm_status, struct wpm_status_state, wpm_status_update_cb,
                            wpm_status_get_state)
ZMK_SUBSCRIPTION(widget_wpm_status, zmk_wpm_state_changed);

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 144, 168);

    lv_obj_t *batteryArea = lv_canvas_create(widget->obj);
    lv_obj_align(batteryArea, LV_ALIGN_TOP_LEFT, 0, 1);
    lv_canvas_set_buffer(batteryArea, widget->cbuf0, CANVAS_SIZE, 20, LV_IMG_CF_TRUE_COLOR);

    lv_obj_t *wpmArea = lv_canvas_create(widget->obj);
    lv_obj_align(wpmArea, LV_ALIGN_TOP_LEFT, 0, 21);
    lv_canvas_set_buffer(wpmArea, widget->cbuf1, CANVAS_SIZE, 91, LV_IMG_CF_TRUE_COLOR);

    lv_obj_t *outputInfoArea = lv_canvas_create(widget->obj);
    lv_obj_align(outputInfoArea, LV_ALIGN_TOP_LEFT, 0, 112);
    lv_canvas_set_buffer(outputInfoArea, widget->cbuf2, CANVAS_SIZE, 30, LV_IMG_CF_TRUE_COLOR);

    lv_obj_t *bottom = lv_canvas_create(widget->obj);
    lv_obj_align(bottom, LV_ALIGN_TOP_LEFT, 0, 142);
    lv_canvas_set_buffer(bottom, widget->cbuf3, CANVAS_SIZE, 26, LV_IMG_CF_TRUE_COLOR);

    sys_slist_append(&widgets, &widget->node);
    widget_output_status_init();
    widget_layer_status_init();
    widget_wpm_status_init();

    return 0;
}

lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) { return widget->obj; }
