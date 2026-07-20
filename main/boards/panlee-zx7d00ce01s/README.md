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

## Audio

This module ships with no on-board microphone, speaker, or audio codec —
the datasheet has no such section, and the WT32-S3-WROVER module itself
has no built-in codec.

Panlee sells a matching accessory for this, **ZXACC-EXTSG-V12**: a small
board with an NS4168 mono I2S class-D amplifier whose 40-pin connector
plugs directly onto the main board's "Extended IO Interface" header (no
loose wiring for the digital side), plus a 2-pin JST for the speaker
itself. Its silkscreen gives the exact signal mapping, already wired up in
`config.h`/`panlee_zx7d00ce01s.cc`:

| Signal | Pin |
|---|---|
| BCLK | GPIO_19 |
| LRCK | GPIO_20 |
| DOUT | GPIO_04 |
| MUTE (NS4168 CTRL/L-R-select pin) | AW9523 LSIO_P07 |

This accessory is **speaker-only** — there's still no microphone. `config.h`
assumes a separate external I2S mic (e.g. INMP441) wired to the extension
GPIOs left free after the speaker claims GPIO19/20/4; adjust
`AUDIO_I2S_MIC_*` if you're using different pins or a different mic module.
Note GPIO19/GPIO20 are the pins the datasheet warns not to use if the
board's native USB is in play as a USB interface — using ZXACC-EXTSG-V12
means you can't also use native USB (fine, since flashing goes through the
debug header anyway).

The MUTE/CTRL line is driven high by default in `InitializeIoExpander()`.
If the speaker stays silent or sounds wrong, try driving it low instead —
its correct idle level for this specific board isn't documented anywhere
public; "MUTE" on the silkscreen is Panlee's label for what is actually the
NS4168's L/R channel-select pin, repurposed as an enable line.

There is likewise no dedicated BOOT/chat button on this board (BOOT/EN are
only broken out on the debug header, for use with the ZXACC-ESPDB
downloader). `BOOT_BUTTON_GPIO` defaults to `EXT_GPIO_2`; wire a momentary
button between that pin and GND, or repoint it at any other free GPIO.

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
