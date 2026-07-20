#include "wifi_board.h"
#include "display/lcd_display.h"
#include "codecs/no_audio_codec.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "mcp_server.h"

#include <esp_log.h>
#include <esp_timer.h>
#include <driver/i2c_master.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_touch_gt911.h>
#include <esp_lvgl_port.h>
#include <esp_io_expander_tca95xx_16bit.h>
#include <lvgl.h>

#define TAG "PanleeZx7d00ce01s"

// The AW9523B on this board only drives the LCD and touch reset lines
// (plus the unused extended-IO header pins). It is register-compatible
// with the TCA9535/TCA9539-style 16-bit IO expander driver already used
// elsewhere in this project, so we reuse that driver rather than writing
// a bespoke AW9523 driver.
class PanleeZx7d00ce01s : public WifiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_ = nullptr;
    esp_io_expander_handle_t io_expander_ = nullptr;
    Button boot_button_;
    LcdDisplay* display_ = nullptr;
    esp_lcd_touch_handle_t touch_ = nullptr;
    esp_timer_handle_t touch_wake_timer_ = nullptr;
    bool touch_was_pressed_ = false;

    void InitializeI2c() {
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = DISPLAY_I2C_SDA_PIN,
            .scl_io_num = DISPLAY_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus_));
    }

    void InitializeIoExpander() {
        ESP_ERROR_CHECK(esp_io_expander_new_i2c_tca95xx_16bit(i2c_bus_, AW9523_I2C_ADDR, &io_expander_));

        // Drive LCD_RST and TP_RST as outputs, idle high (not in reset).
        esp_io_expander_set_dir(io_expander_, AW9523_LCD_RST_PIN | AW9523_TOUCH_RST_PIN, IO_EXPANDER_OUTPUT);
        esp_io_expander_set_level(io_expander_, AW9523_LCD_RST_PIN | AW9523_TOUCH_RST_PIN, 1);

        // Pulse both resets low then release, as required by the LCD
        // driver and the GT911 touch controller.
        esp_io_expander_set_level(io_expander_, AW9523_LCD_RST_PIN | AW9523_TOUCH_RST_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(20));
        esp_io_expander_set_level(io_expander_, AW9523_LCD_RST_PIN | AW9523_TOUCH_RST_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        });
    }

    void InitializeRgbDisplay() {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;

        esp_lcd_rgb_panel_config_t panel_config = {};
        panel_config.clk_src = LCD_CLK_SRC_DEFAULT;
        panel_config.timings.pclk_hz = DISPLAY_PCLK_HZ;
        panel_config.timings.h_res = DISPLAY_WIDTH;
        panel_config.timings.v_res = DISPLAY_HEIGHT;
        panel_config.timings.hsync_pulse_width = DISPLAY_HSYNC_PULSE_WIDTH;
        panel_config.timings.hsync_back_porch = DISPLAY_HSYNC_BACK_PORCH;
        panel_config.timings.hsync_front_porch = DISPLAY_HSYNC_FRONT_PORCH;
        panel_config.timings.vsync_pulse_width = DISPLAY_VSYNC_PULSE_WIDTH;
        panel_config.timings.vsync_back_porch = DISPLAY_VSYNC_BACK_PORCH;
        panel_config.timings.vsync_front_porch = DISPLAY_VSYNC_FRONT_PORCH;
        panel_config.data_width = 16;
        panel_config.bits_per_pixel = 16;
        panel_config.num_fbs = 2;
        panel_config.bounce_buffer_size_px = DISPLAY_WIDTH * 10;
        panel_config.dma_burst_size = 64;
        panel_config.hsync_gpio_num = DISPLAY_HSYNC_PIN;
        panel_config.vsync_gpio_num = DISPLAY_VSYNC_PIN;
        panel_config.de_gpio_num = DISPLAY_DE_PIN;
        panel_config.pclk_gpio_num = DISPLAY_PCLK_PIN;
        panel_config.disp_gpio_num = DISPLAY_DISP_PIN;
        panel_config.data_gpio_nums[0] = DISPLAY_DATA0_PIN;
        panel_config.data_gpio_nums[1] = DISPLAY_DATA1_PIN;
        panel_config.data_gpio_nums[2] = DISPLAY_DATA2_PIN;
        panel_config.data_gpio_nums[3] = DISPLAY_DATA3_PIN;
        panel_config.data_gpio_nums[4] = DISPLAY_DATA4_PIN;
        panel_config.data_gpio_nums[5] = DISPLAY_DATA5_PIN;
        panel_config.data_gpio_nums[6] = DISPLAY_DATA6_PIN;
        panel_config.data_gpio_nums[7] = DISPLAY_DATA7_PIN;
        panel_config.data_gpio_nums[8] = DISPLAY_DATA8_PIN;
        panel_config.data_gpio_nums[9] = DISPLAY_DATA9_PIN;
        panel_config.data_gpio_nums[10] = DISPLAY_DATA10_PIN;
        panel_config.data_gpio_nums[11] = DISPLAY_DATA11_PIN;
        panel_config.data_gpio_nums[12] = DISPLAY_DATA12_PIN;
        panel_config.data_gpio_nums[13] = DISPLAY_DATA13_PIN;
        panel_config.data_gpio_nums[14] = DISPLAY_DATA14_PIN;
        panel_config.data_gpio_nums[15] = DISPLAY_DATA15_PIN;
        panel_config.flags.fb_in_psram = 1;

        ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel));

        display_ = new RgbLcdDisplay(panel_io, panel,
                                      DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                      DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y,
                                      DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    }

    void InitializeTouch() {
        esp_lcd_panel_io_handle_t tp_io_handle = nullptr;
        esp_lcd_panel_io_i2c_config_t tp_io_config = {};
        tp_io_config.scl_speed_hz = 400 * 1000;
        tp_io_config.dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS;
        tp_io_config.control_phase_bytes = 1;
        tp_io_config.dc_bit_offset = 0;
        tp_io_config.lcd_cmd_bits = 16;
        tp_io_config.flags.disable_control_phase = 1;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus_, &tp_io_config, &tp_io_handle));

        // TP_RST is wired to the AW9523 (already pulsed in
        // InitializeIoExpander), and TP_INT is not connected on this
        // board, so both are left as "not controlled by this driver".
        esp_lcd_touch_config_t tp_cfg = {
            .x_max = DISPLAY_WIDTH - 1,
            .y_max = DISPLAY_HEIGHT - 1,
            .rst_gpio_num = GPIO_NUM_NC,
            .int_gpio_num = GPIO_NUM_NC,
            .levels = {
                .reset = 0,
                .interrupt = 0,
            },
            .flags = {
                .swap_xy = 0,
                .mirror_x = 0,
                .mirror_y = 0,
            },
        };

        esp_err_t ret = esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &touch_);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "GT911 touch controller not detected, continuing without touch");
            return;
        }

        const lvgl_port_touch_cfg_t touch_cfg = {
            .disp = lv_display_get_default(),
            .handle = touch_,
        };
        lvgl_port_add_touch(&touch_cfg);
        ESP_LOGI(TAG, "Touch panel initialized");
    }

    // TP_INT is not wired on this board, so we can't get an interrupt when
    // the panel is touched. Instead, poll the GT911 at a low rate on a
    // software timer and treat the rising edge (finger just landed on the
    // glass) as equivalent to a boot-button click. This gives "tap the
    // screen to wake/toggle chat" without needing any extra wiring.
    void InitializeTouchWake() {
        if (touch_ == nullptr) {
            // GT911 wasn't detected in InitializeTouch(); nothing to poll.
            return;
        }

        const esp_timer_create_args_t timer_args = {
            .callback = [](void* arg) {
                static_cast<PanleeZx7d00ce01s*>(arg)->PollTouchWake();
            },
            .arg = this,
            .name = "touch_wake",
        };
        ESP_ERROR_CHECK(esp_timer_create(&timer_args, &touch_wake_timer_));
        // 20 Hz is plenty to catch a tap and cheap enough on the shared I2C
        // bus alongside LVGL's own touch polling.
        ESP_ERROR_CHECK(esp_timer_start_periodic(touch_wake_timer_, 50 * 1000));
    }

    void PollTouchWake() {
        uint16_t x = 0;
        uint16_t y = 0;
        uint8_t touch_count = 0;

        esp_lcd_touch_read_data(touch_);
        bool pressed = esp_lcd_touch_get_coordinates(touch_, &x, &y, nullptr, &touch_count, 1) &&
                       touch_count > 0;

        if (pressed && !touch_was_pressed_) {
            // Rising edge only, so a held finger or a drag doesn't fire
            // repeatedly - one tap toggles chat once, same as a button click.
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
            } else {
                app.ToggleChatState();
            }
        }
        touch_was_pressed_ = pressed;
    }

    void InitializeTools() {
        // Register additional MCP tools here, e.g. screen brightness.
        // See docs/mcp-usage.md for examples.
    }

public:
    PanleeZx7d00ce01s() : boot_button_(BOOT_BUTTON_GPIO) {
        InitializeI2c();
        InitializeIoExpander();
        InitializeRgbDisplay();
        InitializeTouch();
        InitializeTouchWake();
        InitializeButtons();
        InitializeTools();
        GetBacklight()->RestoreBrightness();
    }

    virtual AudioCodec* GetAudioCodec() override {
        // NOTE: this board has no on-board audio codec. This wires a
        // generic I2S microphone and I2S amplifier through the extension
        // header - update config.h to match your actual wiring, or swap
        // in a real codec class if you add one via I2C.
        static NoAudioCodecSimplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK, AUDIO_I2S_SPK_GPIO_WS, AUDIO_I2S_SPK_GPIO_DOUT,
            AUDIO_I2S_MIC_GPIO_SCK, AUDIO_I2S_MIC_GPIO_WS, AUDIO_I2S_MIC_GPIO_DIN);
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }

    virtual Backlight* GetBacklight() override {
        static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
        return &backlight;
    }
};

DECLARE_BOARD(PanleeZx7d00ce01s);