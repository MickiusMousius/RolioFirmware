# Vista508

The Vista508 is a low-power, high refresh rate display meant to replace I2C OLEDs traditionally used.

This shield requires that an `&nice_view_spi` labeled SPI bus is provided with _at least_ MOSI, SCK, and CS pins defined.

## Disable custom widget

The Vista508 shield includes a custom widget. To use the built-in ZMK one, add the following item to your `.conf` file:

```
CONFIG_ZMK_DISPLAY_STATUS_SCREEN_BUILT_IN=y
CONFIG_ZMK_LV_FONT_DEFAULT_SMALL_MONTSERRAT_20=y
CONFIG_LV_FONT_DEFAULT_MONTSERRAT_20=y
CONFIG_ZMK_WIDGET_BATTERY_STATUS_SHOW_PERCENTAGE=y
CONFIG_ZMK_WIDGET_WPM_STATUS=y
```
# Attribution

Significant portions of the code in this directory came from the original ZMK nice!view code.

The original code can be found here: https://github.com/zmkfirmware/zmk/tree/main/app/boards/shields/nice_view

This repository must preserve the original MIT licence.
