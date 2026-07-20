# Panlee ZX7D00CE01S ("SC05")

Smart Panlee / Shenzhen QM Smart Panlee 7-inch smart serial LCD module,
built around a WT32-S3-WROVER-N16R8 (ESP32-S3, 16MB flash, 8MB octal PSRAM).

- Display: 7" TN panel, 800x480, RGB888 driven over a 16-bit RGB565 bus
- Touch: capacitive, GT911 controller over I2C (reset via on-board IO expander)
- IO expander: AW9523B on the shared I2C bus, drives LCD_RST / TP_RST
- Backlight: direct PWM GPIO (BL_PWM)
- Extension headers: 2.0mm and 2.54mm 2x20P headers exposing 5V/3.3V, GND,
  8 IO-expander lines, 8 ESP32-S3 GPIOs, and the shared I2C bus

Source: vendor datasheet `005403_ZX7D00CE01S Datasheet` and the vendor's
[PanelLan_esp32_arduino](https://github.com/smartpanle/PanelLan_esp32_arduino)
board table (confirms GT911 as the touch controller for this specific model).

## Important limitation: no on-board audio

This module is sold purely as a display + touch board — the datasheet has
no microphone, speaker, or audio codec section, and the WT32-S3-WROVER
module itself has no built-in codec. To use it as a XiaoZhi voice
assistant you must wire an external I2S microphone (e.g. INMP441) and an
external I2S amplifier (e.g. MAX98357A) to the "Extended IO Interface"
header, then update the `AUDIO_I2S_*` pin definitions in `config.h` to
match your wiring. The defaults in `config.h` use `EXT_GPIO_2..EXT_GPIO_7`
and avoid `EXT_GPIO_0`/`EXT_GPIO_1` (GPIO19/20), which the datasheet
reserves for native USB.

There is likewise no dedicated BOOT/chat button on this board (BOOT/EN are
only broken out on the debug header, for use with the ZXACC-ESPDB
downloader). `BOOT_BUTTON_GPIO` defaults to `EXT_GPIO_0`; wire a momentary
button between that pin and GND, or repoint it at any other free GPIO.

As an alternative to a physical button, the firmware also polls the GT911
touchscreen directly (`InitializeTouchWake()` / `PollTouchWake()` in
`panlee_zx7d00ce01s.cc`) and toggles chat on a tap, the same way the boot
button does. This works even though `TP_INT` isn't wired: it polls the
touch controller on a 50 ms software timer and fires on the touch-down
edge only, so a single tap toggles chat once (held/dragged touches don't
retrigger). Both the physical button and touch-to-wake are active at the
same time - wire the button too if you want a backup input, or remove the
`InitializeTouchWake()` call in the constructor if you'd rather taps only
drive the on-screen UI.

## Unverified details — check before relying on them

- **AW9523B I2C address**: not printed in the datasheet (it depends on the
  AD0/AD1 strap pins). `config.h` defaults to `0x58`; if the expander
  doesn't respond, I2C-scan the bus and try `0x59`-`0x5B`.
- **GT911 I2C address / INT behavior**: `TP_INT` is not connected on this
  board, so the touch driver runs on the default GT911 address with reset
  only (no interrupt line). If touch doesn't come up, double-check the
  GT911 address strap used by this particular board revision.

## Flashing

This board has no on-board USB-to-serial chip; use the vendor's
ZXACC-ESPDB downloader connected to the 7-pin debug header, or any
USB-to-serial adapter wired to `ESP_TXD`/`ESP_RXD`/`EN`/`BOOT`/`GND` on
that header, then flash normally:

```
python scripts/release.py panlee-zx7d00ce01s
```

or

```
idf.py set-target esp32s3
idf.py menuconfig   # Xiaozhi Assistant -> Board Type -> Panlee ZX7D00CE01S
idf.py build flash monitor
```
