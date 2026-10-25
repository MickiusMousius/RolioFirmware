/*
 * Copyright (c) 2024 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include "../../status_screen.h"
#include "../../styles.h"
#include "../../util.h"
#include "../multi_widget.h"
#include "views.h"

#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>
#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/hid_indicators.h>
#endif
#if IS_ENABLED(CONFIG_DT_HAS_ZMK_BEHAVIOR_CAPS_WORD_ENABLED)
#include <zmk/events/caps_word_state_changed.h>
#endif

/* ========================================================================= */
/*                            STATE AND GLOBALS                              */
/* ========================================================================= */
static bool mod_shift = false;
static bool mod_ctrl = false;
static bool mod_alt = false;
static bool mod_gui = false;
static bool caps_lock = false;
static bool caps_word = false;
/* ========================================================================= */
/*                         UI RENDERING AND DRAWING                          */
/* ========================================================================= */

extern const lv_img_dsc_t *shift_frames[];
extern const lv_img_dsc_t *shift_frames_inv[];
extern const lv_img_dsc_t *ctrl_frames[];
extern const lv_img_dsc_t *ctrl_frames_inv[];
extern const lv_img_dsc_t *alt_frames[];
extern const lv_img_dsc_t *alt_frames_inv[];
extern const lv_img_dsc_t *gui_frames[];
extern const lv_img_dsc_t *gui_frames_inv[];
extern const lv_img_dsc_t *caps_frames[];
extern const lv_img_dsc_t *caps_frames_inv[];
extern const lv_img_dsc_t *capsw_frames[];
extern const lv_img_dsc_t *capsw_frames_inv[];

static void draw_pill_img(lv_obj_t *canvas, int x, int y, int w, int h, const lv_img_dsc_t *img,
                          const lv_img_dsc_t *img_inv, bool active) {
    lv_draw_img_dsc_t img_draw;
    lv_draw_img_dsc_init(&img_draw);
    if (active) {
        lv_canvas_draw_rect(canvas, x, y, w, h, &style_rect_fg);
        int img_x = x + (w - img_inv->header.w) / 2;
        int img_y = y + (h - img_inv->header.h) / 2;
        lv_canvas_draw_img(canvas, img_x, img_y, img_inv, &img_draw);
    } else {
        lv_canvas_draw_rect(canvas, x, y, w, h, &style_rect_border);
        int img_x = x + (w - img->header.w) / 2;
        int img_y = y + (h - img->header.h) / 2;
        lv_canvas_draw_img(canvas, img_x, img_y, img, &img_draw);
    }
}


void draw_modifier_indicator(lv_obj_t *canvas) {

    // We want a 3x2 grid of pills.
    // Display is 144px wide.
    const int pill_w = 45;
    const int pill_h = 39;
    const int pad_x = 4;
    const int pad_y = 6;

    const int start_x = 1; // (144 - (45*3 + 4*2)) / 2 = 0, shift right by 1 to center the 143px block
    const int start_y = 0; // Starts from 0 to fill exactly 82px height (38 + 6 + 38 = 82)

    // Top row (3 pills)
    int top_y = start_y + 2;
    draw_pill_img(canvas, start_x, top_y, pill_w, pill_h, gui_frames[0], gui_frames_inv[0], mod_gui);
    draw_pill_img(canvas, start_x + pill_w + pad_x, top_y, pill_w, pill_h, ctrl_frames[0], ctrl_frames_inv[0],
                  mod_ctrl);
    draw_pill_img(canvas, start_x + 2 * (pill_w + pad_x), top_y, pill_w, pill_h, alt_frames[0], alt_frames_inv[0],
                  mod_alt);

    // Bottom row (3 pills)
    int bottom_y = start_y + pill_h + pad_y;
    draw_pill_img(canvas, start_x, bottom_y, pill_w, pill_h, shift_frames[0], shift_frames_inv[0], mod_shift);
    draw_pill_img(canvas, start_x + pill_w + pad_x, bottom_y, pill_w, pill_h, capsw_frames[0], capsw_frames_inv[0],
                  caps_word);
    draw_pill_img(canvas, start_x + 2 * (pill_w + pad_x), bottom_y, pill_w, pill_h, caps_frames[0], caps_frames_inv[0],
                  caps_lock);
}


bool modifier_indicator_is_active(void) {
    return mod_shift || mod_ctrl || mod_alt || mod_gui || caps_lock || caps_word;
}


/* ========================================================================= */
/*                    EVENT LISTENERS AND STATE UPDATES                      */
/* ========================================================================= */

static void fetch_indicators(void) {
#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
    zmk_hid_indicators_t indicators = zmk_hid_indicators_get_current_profile();
    caps_lock = (indicators & HID_USAGE_LED_CAPS_LOCK) != 0;
#else
    caps_lock = false;
#endif
}


int modifier_indicator_mods_listener(const zmk_event_t *eh) {
    zmk_mod_flags_t explicit_mods = zmk_hid_get_explicit_mods();
    bool new_shift = (explicit_mods & (MOD_LSFT | MOD_RSFT)) != 0;
    bool new_ctrl = (explicit_mods & (MOD_LCTL | MOD_RCTL)) != 0;
    bool new_alt = (explicit_mods & (MOD_LALT | MOD_RALT)) != 0;
    bool new_gui = (explicit_mods & (MOD_LGUI | MOD_RGUI)) != 0;

    if (mod_shift != new_shift || mod_ctrl != new_ctrl || mod_alt != new_alt || mod_gui != new_gui) {
        mod_shift = new_shift;
        mod_ctrl = new_ctrl;
        mod_alt = new_alt;
        mod_gui = new_gui;
        if (zmk_widget_multi_should_redraw_for_modifiers()) {
            zmk_widget_multi_status_update_async();
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}


ZMK_LISTENER(widget_mod_indicator_mods, modifier_indicator_mods_listener);
ZMK_SUBSCRIPTION(widget_mod_indicator_mods, zmk_keycode_state_changed);

#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
int modifier_indicator_leds_listener(const zmk_event_t *eh) {
    bool old_caps = caps_lock;
    fetch_indicators();
    if (old_caps != caps_lock) {
        if (zmk_widget_multi_should_redraw_for_modifiers()) {
            zmk_widget_multi_status_update_async();
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}


ZMK_LISTENER(widget_mod_indicator_leds, modifier_indicator_leds_listener);
ZMK_SUBSCRIPTION(widget_mod_indicator_leds, zmk_hid_indicators_changed);
#endif

#if IS_ENABLED(CONFIG_DT_HAS_ZMK_BEHAVIOR_CAPS_WORD_ENABLED)
int modifier_indicator_caps_word_listener(const zmk_event_t *eh) {
    const struct zmk_caps_word_state_changed *ev = as_zmk_caps_word_state_changed(eh);
    if (ev) {
        if (caps_word != ev->active) {
            caps_word = ev->active;
            if (zmk_widget_multi_should_redraw_for_modifiers()) {
                zmk_widget_multi_status_update_async();
            }
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}


ZMK_LISTENER(widget_mod_indicator_caps_word, modifier_indicator_caps_word_listener);
ZMK_SUBSCRIPTION(widget_mod_indicator_caps_word, zmk_caps_word_state_changed);
#endif

void modifier_indicator_init(void) {
    // Initial fetch of state
    zmk_mod_flags_t explicit_mods = zmk_hid_get_explicit_mods();
    mod_shift = (explicit_mods & (MOD_LSFT | MOD_RSFT)) != 0;
    mod_ctrl = (explicit_mods & (MOD_LCTL | MOD_RCTL)) != 0;
    mod_alt = (explicit_mods & (MOD_LALT | MOD_RALT)) != 0;
    mod_gui = (explicit_mods & (MOD_LGUI | MOD_RGUI)) != 0;

    fetch_indicators();
}
