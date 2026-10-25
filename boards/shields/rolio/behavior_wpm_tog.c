#define DT_DRV_COMPAT zmk_behavior_wpm_tog

#include <drivers/behavior.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* The display functions only exist on the central, the peripheral has its own simpler display */
#define VISTA508_CENTRAL_DISPLAY                                                                                       \
    (IS_ENABLED(CONFIG_SHIELD_VISTA508) && IS_ENABLED(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM) &&                      \
     (!IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)))

#if VISTA508_CENTRAL_DISPLAY
extern void vista508_wpm_mode_toggle(void);
#endif

static int behavior_wpm_tog_init(const struct device *dev) { return 0; }


static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
#if VISTA508_CENTRAL_DISPLAY
    vista508_wpm_mode_toggle();
#endif
    return ZMK_BEHAVIOR_OPAQUE;
}


static int on_keymap_binding_released(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}


static const struct behavior_driver_api behavior_wpm_tog_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};


BEHAVIOR_DT_INST_DEFINE(0, behavior_wpm_tog_init, NULL, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_wpm_tog_driver_api);
