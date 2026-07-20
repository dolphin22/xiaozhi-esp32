#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

// ---------------------------------------------------------------------------
// This board (Smart Panlee ZX7D00CE01S / "SC05") is sold as a bare LCD+touch
// module. It has no on-board audio, but Panlee sells a matching plug-in
// speaker accessory, ZXACC-EXTSG-V12: an NS4168 mono I2S class-D amp on a
// small board whose 40-pin J1 connector plugs directly onto the main
// board's "Extended IO Interface" header (no loose wiring needed), with the
// speaker itself wired to its 2-pin J2 (SPEAKER +/-). Its silkscreen gives
// the exact pin mapping used below (BCLK:GPIO_19, LRCK:GPIO_20,
// DOUT:GPIO_04, MUTE:LSIO_P07 - i.e. the mute/channel-select line goes
// through the AW9523 IO expander, not a bare ESP32 GPIO).
//
// ZXACC-EXTSG-V12 is speaker-only - there is still no microphone on this
// setup. AUDIO_I2S_MIC_* below assumes a separate external I2S mic (e.g.
// INMP441) wired to the remaining free extension-header GPIOs; adjust if
// you're using different pins or a different mic module.
// ---------------------------------------------------------------------------

#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

// Speaker (I2S TX) -> ZXACC-EXTSG-V12 (NS4168), per its silkscreen
#define AUDIO_I2S_SPK_GPIO_BCLK  GPIO_NUM_19   // EXT_GPIO_1 - "BCLK:GPIO_19"
#define AUDIO_I2S_SPK_GPIO_WS    GPIO_NUM_20   // EXT_GPIO_0 - "LRCK:GPIO_20"
#define AUDIO_I2S_SPK_GPIO_DOUT  GPIO_NUM_4    // EXT_GPIO_4 - "DOUT:GPIO_04"

// NS4168 MUTE/channel-select line -> AW9523 P07 ("MUTE:LSIO_P07"). The
// NS4168 itself has no dedicated mute pin - this is its CTRL (L/R select)
// pin, repurposed by Panlee as an enable/mute line. Driven from board.cc,
// not from the audio codec class, since it lives behind the IO expander.
#define AW9523_SPK_MUTE_PIN      IO_EXPANDER_PIN_NUM_7

// Microphone (I2S RX) -> separate external mic (e.g. INMP441), NOT part of
// ZXACC-EXTSG-V12. Uses the extension-header GPIOs left free after the
// speaker accessory claims GPIO19/20/4.
#define AUDIO_I2S_MIC_GPIO_SCK   GPIO_NUM_42   // EXT_GPIO_5
#define AUDIO_I2S_MIC_GPIO_WS    GPIO_NUM_1    // EXT_GPIO_6
#define AUDIO_I2S_MIC_GPIO_DIN   GPIO_NUM_2    // EXT_GPIO_7

// Boot / chat button. This board has no dedicated boot/reset button; a
// USB-to-serial downloader board (ZXACC-ESPDB) provides BOOT/EN, so there
// is no natural GPIO left free for a physical chat button either. GPIO0 is
// already consumed as LCD_RGB_D6, and GPIO19/20 are now claimed by the
// speaker accessory, so this uses another free extension GPIO. Wire a
// momentary button between EXT_GPIO_2 and GND, or change this if you use
// that pin for something else.
#define BOOT_BUTTON_GPIO         GPIO_NUM_41   // EXT_GPIO_2

// Shared I2C bus (touch controller + AW9523B IO expander)
#define DISPLAY_I2C_SDA_PIN      GPIO_NUM_48   // IIC_SDA
#define DISPLAY_I2C_SCL_PIN      GPIO_NUM_47   // IIC_SCL

// AW9523B on-board IO expander. Datasheet does not print the I2C address
// (set by AD0/AD1 strap pins); 0x58 is the AW9523B default with both
// address pins tied low. If the expander does not respond, scan the I2C
// bus (0x58-0x5B) and update this.
#define AW9523_I2C_ADDR           0x5B
#define AW9523_LCD_RST_PIN        IO_EXPANDER_PIN_NUM_10  // AW9523 P10
#define AW9523_TOUCH_RST_PIN      IO_EXPANDER_PIN_NUM_11  // AW9523 P11

// LCD backlight - direct PWM-capable GPIO (BL_PWM), not via IO expander
#define DISPLAY_BACKLIGHT_PIN            GPIO_NUM_45
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT  false

// Display panel: 800x480 RGB TN panel, RGB888 driven over a 16-bit bus
#define DISPLAY_WIDTH    800
#define DISPLAY_HEIGHT   480
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY  false
#define DISPLAY_OFFSET_X 0
#define DISPLAY_OFFSET_Y 0

// RGB timing signals
#define DISPLAY_PCLK_PIN  GPIO_NUM_9
#define DISPLAY_HSYNC_PIN GPIO_NUM_5
#define DISPLAY_VSYNC_PIN GPIO_NUM_38
#define DISPLAY_DE_PIN    GPIO_NUM_39
#define DISPLAY_DISP_PIN  GPIO_NUM_NC

// RGB panel timings, taken from the vendor's own BSP
// (smartpanle/QMSD-ESP32-BSP, board ZX7D00CE01S-8048,
// qmsd_board_def.h). These are NOT in the printed datasheet - the
// values previously guessed here (16MHz / 8,8 porches, borrowed from
// an unrelated panel) clipped/shifted the visible image. Use these.
#define DISPLAY_PCLK_HZ              (24 * 1000 * 1000)
#define DISPLAY_HSYNC_PULSE_WIDTH    1
#define DISPLAY_HSYNC_BACK_PORCH     87
#define DISPLAY_HSYNC_FRONT_PORCH    20
#define DISPLAY_VSYNC_PULSE_WIDTH    1
#define DISPLAY_VSYNC_BACK_PORCH     31
#define DISPLAY_VSYNC_FRONT_PORCH    5

// RGB data bus, D0 (LSB) .. D15 (MSB) for RGB565
#define DISPLAY_DATA0_PIN  GPIO_NUM_17  // LCD_RGB_D0
#define DISPLAY_DATA1_PIN  GPIO_NUM_16  // LCD_RGB_D1
#define DISPLAY_DATA2_PIN  GPIO_NUM_15  // LCD_RGB_D2
#define DISPLAY_DATA3_PIN  GPIO_NUM_7  // LCD_RGB_D3
#define DISPLAY_DATA4_PIN  GPIO_NUM_6  // LCD_RGB_D4
#define DISPLAY_DATA5_PIN  GPIO_NUM_21  // LCD_RGB_D5
#define DISPLAY_DATA6_PIN  GPIO_NUM_0  // LCD_RGB_D6
#define DISPLAY_DATA7_PIN  GPIO_NUM_46  // LCD_RGB_D7
#define DISPLAY_DATA8_PIN  GPIO_NUM_3  // LCD_RGB_D8
#define DISPLAY_DATA9_PIN  GPIO_NUM_8  // LCD_RGB_D9
#define DISPLAY_DATA10_PIN GPIO_NUM_18  // LCD_RGB_D10
#define DISPLAY_DATA11_PIN GPIO_NUM_10  // LCD_RGB_D11
#define DISPLAY_DATA12_PIN GPIO_NUM_11  // LCD_RGB_D12
#define DISPLAY_DATA13_PIN GPIO_NUM_12  // LCD_RGB_D13
#define DISPLAY_DATA14_PIN GPIO_NUM_13  // LCD_RGB_D14
#define DISPLAY_DATA15_PIN GPIO_NUM_14  // LCD_RGB_D15

// GT911 capacitive touch (per vendor's PanelLan_esp32_arduino board table,
// SC05/ZX7D00CE01S uses a GT911). TP_INT is not connected on this board,
// so the touch driver must run in I2C-polling mode with reset only.

#endif // _BOARD_CONFIG_H_
