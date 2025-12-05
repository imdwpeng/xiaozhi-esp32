#include "simple_touch_manager.h"
#include <esp_log.h>
#include <cstring>

static const char* TAG = "SimpleTouchManager";

SimpleTouchManager& SimpleTouchManager::GetInstance() {
    static SimpleTouchManager instance;
    return instance;
}

bool SimpleTouchManager::Initialize() {
    if (initialized_) {
        return true;
    }
    
    ESP_LOGI(TAG, "Initializing simple touch manager");
    
    // 初始化触摸外设
    esp_err_t ret = touch_pad_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize touch pad: %s", esp_err_to_name(ret));
        return false;
    }
    
    // 设置触摸电压配置
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    
    // 启动触摸传感器FSM
    ret = touch_pad_fsm_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start touch pad FSM: %s", esp_err_to_name(ret));
        return false;
    }
    
    memset(buttons_, 0, sizeof(buttons_));
    button_count_ = 0;
    initialized_ = true;
    running_ = false;
    
    ESP_LOGI(TAG, "Simple touch manager initialized");
    return true;
}

bool SimpleTouchManager::AddButton(touch_pad_t pad_num, uint32_t threshold, const char* name) {
    if (!initialized_ || button_count_ >= MAX_BUTTONS) {
        return false;
    }
    
    // 配置触摸通道
    esp_err_t ret = touch_pad_config(pad_num);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to config touch pad %d: %s", pad_num, esp_err_to_name(ret));
        return false;
    }
    
    // 设置阈值
    ret = touch_pad_set_thresh(pad_num, threshold);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set threshold for touch pad %d: %s", pad_num, esp_err_to_name(ret));
        return false;
    }
    
    // 配置滤波器
    touch_filter_config_t filter_config = {
        .mode = TOUCH_PAD_FILTER_IIR_16,
        .debounce_cnt = 3,
        .noise_thr = 2,
        .jitter_step = 5,
        .smh_lvl = TOUCH_PAD_SMOOTH_IIR_4,
    };
    
    ret = touch_pad_filter_set_config(&filter_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to config filter: %s", esp_err_to_name(ret));
        return false;
    }
    
    // 启用滤波器
    ret = touch_pad_filter_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable filter: %s", esp_err_to_name(ret));
        return false;
    }
    
    buttons_[button_count_] = {
        .pad = pad_num,
        .threshold = threshold,
        .name = name ? name : "Unknown",
        .last_state = false,
        .debounce_count = 0
    };
    
    ESP_LOGI(TAG, "Added touch button %d: %s (pad %d, threshold %lu)", 
             button_count_, buttons_[button_count_].name, pad_num, threshold);
    
    button_count_++;
    return true;
}

void SimpleTouchManager::SetCallback(SimpleTouchCallback callback) {
    callback_ = callback;
}

void SimpleTouchManager::SetGameModeCallback(SimpleTouchCallback callback) {
    game_callback_ = callback;
}

void SimpleTouchManager::SetMode(int mode) {
    current_mode_ = mode;
    ESP_LOGI(TAG, "Touch mode switched to: %d", mode);
}

bool SimpleTouchManager::Start() {
    if (!initialized_ || running_) {
        return false;
    }
    
    ESP_LOGI(TAG, "Starting touch detection task");
    
    BaseType_t ret = xTaskCreate(TouchTask, "simple_touch", 4096, this, 5, &task_handle_);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create touch task");
        return false;
    }
    
    running_ = true;
    return true;
}

void SimpleTouchManager::Stop() {
    if (!running_) {
        return;
    }
    
    ESP_LOGI(TAG, "Stopping touch detection task");
    running_ = false;
    
    if (task_handle_) {
        vTaskDelete(task_handle_);
        task_handle_ = nullptr;
    }
}

void SimpleTouchManager::TouchTask(void* arg) {
    SimpleTouchManager* manager = static_cast<SimpleTouchManager*>(arg);
    manager->ProcessTouch();
}

void SimpleTouchManager::ProcessTouch() {
    while (running_) {
        for (int i = 0; i < button_count_; i++) {
            uint32_t touch_value;
            esp_err_t ret = touch_pad_read_raw_data(buttons_[i].pad, &touch_value);
            if (ret != ESP_OK) {
                continue;
            }
            
            // 判断按键状态 (触摸时值变小)
            bool current_state = (touch_value < buttons_[i].threshold);
            
            // 防抖处理
            if (current_state != buttons_[i].last_state) {
                if (current_state) {
                    buttons_[i].debounce_count++;
                    if (buttons_[i].debounce_count >= 3) {
                        // 按键按下
                        SimpleTouchCallback active_callback = (current_mode_ == 1 && game_callback_) ? game_callback_ : callback_;
                        if (active_callback) {
                            active_callback(i, SIMPLE_TOUCH_PRESS);
                        }
                        buttons_[i].last_state = true;
                        buttons_[i].debounce_count = 0;
                    }
                } else {
                    buttons_[i].debounce_count++;
                    if (buttons_[i].debounce_count >= 3) {
                        // 按键释放
                        SimpleTouchCallback active_callback = (current_mode_ == 1 && game_callback_) ? game_callback_ : callback_;
                        if (active_callback) {
                            active_callback(i, SIMPLE_TOUCH_RELEASE);
                        }
                        buttons_[i].last_state = false;
                        buttons_[i].debounce_count = 0;
                    }
                }
            } else {
                buttons_[i].debounce_count = 0;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(20)); // 50Hz采样率
    }
}