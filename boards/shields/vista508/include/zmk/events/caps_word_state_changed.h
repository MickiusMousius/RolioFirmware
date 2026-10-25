#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

struct zmk_caps_word_state_changed {
    bool active;
};


ZMK_EVENT_DECLARE(zmk_caps_word_state_changed);