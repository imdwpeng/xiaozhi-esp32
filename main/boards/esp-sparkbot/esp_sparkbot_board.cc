#include "wifi_board.h"
#include "codecs/es8311_audio_codec.h"
#include "display/lcd_display.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "mcp_server.h"
#include "settings.h"
#include "display/screen_manager.h"
#include "display/screens.h"
#include "input/simple_touch_manager.h"

#include <wifi_station.h>
#include <esp_log.h>
#include <esp_lcd_panel_vendor.h>
#include <driver/i2c_master.h>
#include <driver/spi_common.h>
#include <driver/uart.h>
#include <cstring>

#include "esp32_camera.h"
#include "touch_element/touch_button.h"

#define TAG "esp_sparkbot"

/* Touch buttons handle */
static touch_button_handle_t button_handle[3];
static bool touch_initialized = false;

// Static callback function for touch button events
static void button_callback(touch_button_handle_t out_handle, touch_button_message_t *out_message, void *arg) {
    int channel = (int)arg;
    int button_id = 0;
    
    // Map touch pad channels to button IDs
    if (channel == TOUCH_PAD_NUM2) {
        button_id = 1; // Left button
    } else if (channel == TOUCH_PAD_NUM3) {
        button_id = 2; // Right button  
    } else if (channel == TOUCH_PAD_NUM1) {
        button_id = 0; // Center button
    }
    
    if (out_message->event == TOUCH_BUTTON_EVT_ON_PRESS) {
        auto& screen_manager = ScreenManager::GetInstance();
        ESP_LOGI(TAG, "Touch button %d (channel %d) pressed", button_id, channel);
        
        if (button_id == 1) {  // Left
            ESP_LOGI(TAG, "Left touch button - switching to previous screen");
            screen_manager.HandleEvent(SCREEN_EVENT_PREVIOUS);
        } else if (button_id == 2) {  // Right
            ESP_LOGI(TAG, "Right touch button - switching to next screen");
            screen_manager.HandleEvent(SCREEN_EVENT_NEXT);
        } else if (button_id == 0) {  // Center
            ESP_LOGI(TAG, "Center touch button - confirm action");
        }
    } else if (out_message->event == TOUCH_BUTTON_EVT_ON_LONGPRESS) {
        ESP_LOGI(TAG, "Touch button %d (channel %d) long pressed", button_id, channel);
        // Handle long press if needed
    }
}

class SparkBotEs8311AudioCodec : public Es8311AudioCodec {
private:    

public:
    SparkBotEs8311AudioCodec(void* i2c_master_handle, i2c_port_t i2c_port, int input_sample_rate, int output_sample_rate,
                        gpio_num_t mclk, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din,
                        gpio_num_t pa_pin, uint8_t es8311_addr, bool use_mclk = true)
        : Es8311AudioCodec(i2c_master_handle, i2c_port, input_sample_rate, output_sample_rate,
                             mclk,  bclk,  ws,  dout,  din,pa_pin,  es8311_addr,  use_mclk = true) {}

    void EnableOutput(bool enable) override {
        if (enable == output_enabled_) {
            return;
        }
        if (enable) {
            Es8311AudioCodec::EnableOutput(enable);
        } else {
           // Nothing todo because the display io and PA io conflict
        }
    }
};

class EspSparkBot : public WifiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_;
    Button boot_button_;
    Display* display_;
    Esp32Camera* camera_;
    light_mode_t light_mode_ = LIGHT_MODE_ALWAYS_ON;

    void InitializeI2c() {
        // Initialize I2C peripheral
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = AUDIO_CODEC_I2C_SDA_PIN,
            .scl_io_num = AUDIO_CODEC_I2C_SCL_PIN,
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

    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_MOSI_GPIO;
        buscfg.miso_io_num = GPIO_NUM_NC;
        buscfg.sclk_io_num = DISPLAY_CLK_GPIO;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting && !WifiStation::GetInstance().IsConnected()) {
                ResetWifiConfiguration();
            }
            app.ToggleChatState();
        });
        
        // 初始化触摸按键系统 - 使用Touch Element库（与factory_demo_v1一致）
        ESP_LOGI(TAG, "Initializing touch buttons using Touch Element library (same as factory_demo_v1)");
        InitializeTouchButtons();
    }

    void InitializeDisplay() {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;

        // 液晶屏控制IO初始化
        ESP_LOGD(TAG, "Install panel IO");
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_CS_GPIO;
        io_config.dc_gpio_num = DISPLAY_DC_GPIO;
        io_config.spi_mode = 0;
        io_config.pclk_hz = 40 * 1000 * 1000;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI3_HOST, &io_config, &panel_io));

        // 初始化液晶屏驱动芯片
        ESP_LOGD(TAG, "Install LCD driver");

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = GPIO_NUM_NC;
        panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
        panel_config.bits_per_pixel = 16;
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));
        
        esp_lcd_panel_reset(panel);
        esp_lcd_panel_init(panel);
        esp_lcd_panel_invert_color(panel, true);
        esp_lcd_panel_disp_on_off(panel, true);
        display_ = new SpiLcdDisplay(panel_io, panel,
                                    DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    }

    void InitializeCamera() {
        // TODO: 摄像头初始化需要更新为ESP-IDF v5.4兼容的API
        // 暂时注释掉摄像头初始化
        ESP_LOGI(TAG, "Camera temporarily disabled - API update needed for ESP-IDF v5.4");
        camera_ = nullptr;
        /*
        // 复用 I2C 总线
        esp_video_init_sccb_config_t sccb_config = {
            .init_sccb = false,  // 不初始化新的 SCCB，使用现有的 I2C 总线
            .i2c_handle = i2c_bus_,  // 使用现有的 I2C 总线句柄
            .freq = 100000,  // 100kHz
        };

        // DVP pin configuration
        static esp_cam_ctlr_dvp_pin_config_t dvp_pin_config = {
            .data_width = CAM_CTLR_DATA_WIDTH_8,
            .data_io = {
                [0] = SPARKBOT_CAMERA_D0,
                [1] = SPARKBOT_CAMERA_D1,
                [2] = SPARKBOT_CAMERA_D2,
                [3] = SPARKBOT_CAMERA_D3,
                [4] = SPARKBOT_CAMERA_D4,
                [5] = SPARKBOT_CAMERA_D5,
                [6] = SPARKBOT_CAMERA_D6,
                [7] = SPARKBOT_CAMERA_D7,
            },
            .vsync_io = SPARKBOT_CAMERA_VSYNC,
            .de_io = SPARKBOT_CAMERA_HSYNC,
            .pclk_io = SPARKBOT_CAMERA_PCLK,
            .xclk_io = SPARKBOT_CAMERA_XCLK,
        };

        // DVP configuration
        esp_video_init_dvp_config_t dvp_config = {
            .sccb_config = sccb_config,
            .reset_pin = SPARKBOT_CAMERA_RESET,
            .pwdn_pin = SPARKBOT_CAMERA_PWDN,
            .dvp_pin = dvp_pin_config,
            .xclk_freq = SPARKBOT_CAMERA_XCLK_FREQ,
        };

        // Main video configuration
        esp_video_init_config_t video_config = {
            .dvp = &dvp_config,
        };
        
        camera_ = new Esp32Camera(video_config);

        Settings settings("sparkbot", false);
        // 考虑到部分复刻使用了不可动摄像头的设计，默认启用翻转
        bool camera_flipped = static_cast<bool>(settings.GetInt("camera-flipped", 1));
        camera_->SetHMirror(camera_flipped);
        camera_->SetVFlip(camera_flipped);
        */
    }

    /*
        ESP-SparkBot 的底座
        https://gitee.com/esp-friends/esp_sparkbot/tree/master/example/tank/c2_tracked_chassis
    */
    void InitializeEchoUart() {
        uart_config_t uart_config = {
            .baud_rate = ECHO_UART_BAUD_RATE,
            .data_bits = UART_DATA_8_BITS,
            .parity    = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT,
        };
        int intr_alloc_flags = 0;

        ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
        ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
        ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, UART_ECHO_TXD, UART_ECHO_RXD, UART_ECHO_RTS, UART_ECHO_CTS));

        SendUartMessage("w2");
    }

    void SendUartMessage(const char * command_str) {
        uint8_t len = strlen(command_str);
        uart_write_bytes(ECHO_UART_PORT_NUM, command_str, len);
        ESP_LOGI(TAG, "Sent command: %s", command_str);
    }

    void InitializeTouchButtons() {
        ESP_LOGI(TAG, "=== Initializing Touch Buttons using Touch Element Library ===");
        ESP_LOGI(TAG, "This matches the factory_demo_v1 implementation");
        
        // Touch buttons channel array (same as factory_demo_v1)
        static const touch_pad_t channel_array[3] = {
            TOUCH_PAD_NUM1,  // Center button
            TOUCH_PAD_NUM2,  // Left button  
            TOUCH_PAD_NUM3,  // Right button
        };
        
        // Touch buttons channel sensitivity array (same as factory_demo_v1)
        static const float channel_sens_array[3] = {
            0.035F,  // Center button sensitivity
            0.08F,   // Left button sensitivity
            0.08F,   // Right button sensitivity
        };
        
        // Initialize Touch Element library
        touch_elem_global_config_t global_config = TOUCH_ELEM_GLOBAL_DEFAULT_CONFIG();
        esp_err_t ret = touch_element_install(&global_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to install touch element library: %s", esp_err_to_name(ret));
            return;
        }
        ESP_LOGI(TAG, "Touch element library installed successfully");
        
        touch_button_global_config_t button_global_config = TOUCH_BUTTON_GLOBAL_DEFAULT_CONFIG();
        ret = touch_button_install(&button_global_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to install touch button: %s", esp_err_to_name(ret));
            return;
        }
        ESP_LOGI(TAG, "Touch button installed successfully");
        
        for (int i = 0; i < 3; i++) {
            ESP_LOGI(TAG, "Creating touch button %d on channel %d with sensitivity %.3f", 
                     i, channel_array[i], channel_sens_array[i]);
            
            touch_button_config_t button_config = {
                .channel_num = channel_array[i],
                .channel_sens = channel_sens_array[i]
            };
            
            // Create Touch buttons
            ret = touch_button_create(&button_config, &button_handle[i]);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to create touch button %d: %s", i, esp_err_to_name(ret));
                return;
            }
            ESP_LOGI(TAG, "Touch button %d created successfully", i);
            
            // Subscribe touch button events
            ret = touch_button_subscribe_event(button_handle[i],
                                              TOUCH_ELEM_EVENT_ON_PRESS | TOUCH_ELEM_EVENT_ON_RELEASE | TOUCH_ELEM_EVENT_ON_LONGPRESS,
                                              (void *)channel_array[i]);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to subscribe events for button %d: %s", i, esp_err_to_name(ret));
                return;
            }
            
            // Set callback dispatch method
            ret = touch_button_set_dispatch_method(button_handle[i], TOUCH_ELEM_DISP_CALLBACK);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to set dispatch method for button %d: %s", i, esp_err_to_name(ret));
                return;
            }
            
            // Set LongPress event trigger threshold time (2 seconds)
            ret = touch_button_set_longpress(button_handle[i], 2000);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to set longpress for button %d: %s", i, esp_err_to_name(ret));
                return;
            }
        }
        
        // No need to declare callback here, will use static function
        
        // Register the callback for all buttons
        for (int i = 0; i < 3; i++) {
            ret = touch_button_set_callback(button_handle[i], button_callback);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to set callback for button %d: %s", i, esp_err_to_name(ret));
                return;
            }
        }
        
        // Start the touch element library
        ret = touch_element_start();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to start touch element library: %s", esp_err_to_name(ret));
            return;
        }
        
        ESP_LOGI(TAG, "Touch element library started successfully!");
        ESP_LOGI(TAG, "Touch buttons are now ready - using same implementation as factory_demo_v1");
        touch_initialized = true;
    }

    void InitializeTools() {
        auto& mcp_server = McpServer::GetInstance();
        // 定义设备的属性
        mcp_server.AddTool("self.chassis.get_light_mode", "获取灯光效果编号", PropertyList(), [this](const PropertyList& properties) -> ReturnValue {
            if (light_mode_ < 2) {
                return 1;
            } else {
                return light_mode_ - 2;
            }
        });

        mcp_server.AddTool("self.chassis.go_forward", "前进", PropertyList(), [this](const PropertyList& properties) -> ReturnValue {
            SendUartMessage("x0.0 y1.0");
            return true;
        });

        mcp_server.AddTool("self.chassis.go_back", "后退", PropertyList(), [this](const PropertyList& properties) -> ReturnValue {
            SendUartMessage("x0.0 y-1.0");
            return true;
        });

        mcp_server.AddTool("self.chassis.turn_left", "向左转", PropertyList(), [this](const PropertyList& properties) -> ReturnValue {
            SendUartMessage("x-1.0 y0.0");
            return true;
        });

        mcp_server.AddTool("self.chassis.turn_right", "向右转", PropertyList(), [this](const PropertyList& properties) -> ReturnValue {
            SendUartMessage("x1.0 y0.0");
            return true;
        });
        
        mcp_server.AddTool("self.chassis.dance", "跳舞", PropertyList(), [this](const PropertyList& properties) -> ReturnValue {
            SendUartMessage("d1");
            light_mode_ = LIGHT_MODE_MAX;
            return true;
        });

        mcp_server.AddTool("self.chassis.switch_light_mode", "打开灯光效果", PropertyList({
            Property("light_mode", kPropertyTypeInteger, 1, 6)
        }), [this](const PropertyList& properties) -> ReturnValue {
            char command_str[5] = {'w', 0, 0};
            char mode = static_cast<light_mode_t>(properties["light_mode"].value<int>());

            ESP_LOGI(TAG, "Switch Light Mode: %c", (mode + '0'));

            if (mode >= 3 && mode <= 8) {
                command_str[1] = mode + '0';
                SendUartMessage(command_str);
                return true;
            }
            throw std::runtime_error("Invalid light mode");
        });

        mcp_server.AddTool("self.camera.set_camera_flipped", "翻转摄像头图像方向", PropertyList(), [this](const PropertyList& properties) -> ReturnValue {
            Settings settings("sparkbot", true);
            // 考虑到部分复刻使用了不可动摄像头的设计，默认启用翻转
            bool flipped = !static_cast<bool>(settings.GetInt("camera-flipped", 1));
            
            camera_->SetHMirror(flipped);
            camera_->SetVFlip(flipped);
            
            settings.SetInt("camera-flipped", flipped ? 1 : 0);
            
            return true;
        });
    }

public:
    EspSparkBot() : boot_button_(BOOT_BUTTON_GPIO) {
        InitializeI2c();
        InitializeSpi();
        InitializeDisplay();
        InitializeButtons();
        InitializeCamera();
        InitializeEchoUart();
        InitializeTools();
        GetBacklight()->RestoreBrightness();
        
        // 初始化屏幕管理器
        ESP_LOGI(TAG, "Initializing screen manager");
        ScreenFactory::InitializeScreens();
    }

    virtual AudioCodec* GetAudioCodec() override {
         static SparkBotEs8311AudioCodec audio_codec(i2c_bus_, I2C_NUM_0, AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_MCLK, AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN,
            AUDIO_CODEC_PA_PIN, AUDIO_CODEC_ES8311_ADDR);
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }

    virtual Backlight* GetBacklight() override {
        static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
        return &backlight;
    }

    virtual Camera* GetCamera() override {
        return camera_;
    }
};

DECLARE_BOARD(EspSparkBot);
