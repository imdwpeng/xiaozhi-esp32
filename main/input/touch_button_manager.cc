#include "touch_button_manager.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <cstring>

static const char* TAG = "TouchButtonManager";

TouchButtonManager& TouchButtonManager::GetInstance() {
    static TouchButtonManager instance;
    return instance;
}

bool TouchButtonManager::Initialize() {
    if (initialized_) {
        return true;
    }
    
    ESP_LOGI(TAG, "Initializing touch button manager for ESP32-S3");
    
    // 初始化触摸外设
    touch_pad_init();
    
    // ESP32-S3 特定的触摸电压配置
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    
    // 启动触摸传感器FSM
    touch_pad_fsm_start();
    
    // 设置触摸睡眠模式配置
    touch_pad_sleep_channel_enable(TOUCH_PAD_NUM1, false);
    touch_pad_sleep_channel_enable(TOUCH_PAD_NUM2, false);
    touch_pad_sleep_channel_enable(TOUCH_PAD_NUM3, false);
    touch_pad_sleep_channel_enable(TOUCH_PAD_NUM4, false);
    
    // 创建事件队列
    event_queue_ = xQueueCreate(10, sizeof(int));
    if (!event_queue_) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return false;
    }
    
    // 初始化滤波数组
    memset(filter_values_, 0, sizeof(filter_values_));
    memset(filter_index_, 0, sizeof(filter_index_));
    memset(debounce_counter_, 0, sizeof(debounce_counter_));
    memset(last_state_, 0, sizeof(last_state_));
    
    button_count_ = 0;
    initialized_ = true;
    
    ESP_LOGI(TAG, "Touch button manager initialized for ESP32-S3");
    return true;
}

void TouchButtonManager::Deinitialize() {
    Stop();
    
    if (event_queue_) {
        vQueueDelete(event_queue_);
        event_queue_ = nullptr;
    }
    
    // 删除长按定时器
    for (int i = 0; i < MAX_TOUCH_BUTTONS; i++) {
        if (long_press_timer_[i]) {
            esp_timer_delete(long_press_timer_[i]);
            long_press_timer_[i] = nullptr;
        }
    }
    
    // 停止触摸传感器FSM
    touch_pad_fsm_stop();
    touch_pad_deinit();
    initialized_ = false;
    
    ESP_LOGI(TAG, "Touch button manager deinitialized");
}

bool TouchButtonManager::AddButton(touch_pad_t touch_num, float threshold, const char* name) {
    if (!initialized_ || button_count_ >= MAX_TOUCH_BUTTONS) {
        ESP_LOGE(TAG, "Cannot add button: not initialized or too many buttons");
        return false;
    }
    
    // 配置触摸通道 - 使用新的API
    esp_err_t ret = touch_pad_config(touch_num);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to config touch pad %d: %s", touch_num, esp_err_to_name(ret));
        return false;
    }
    
    // 设置阈值
    ret = touch_pad_set_thresh(touch_num, (uint32_t)threshold);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set threshold for touch pad %d: %s", touch_num, esp_err_to_name(ret));
        return false;
    }
    
    // 配置滤波器 - 使用新的API
    touch_filter_config_t filter_config = {
        .mode = TOUCH_PAD_FILTER_IIR_16,
        .debounce_cnt = 3,
        .noise_thr = 2,
        .jitter_step = 5,
        .smh_lvl = TOUCH_PAD_SMOOTH_IIR_4
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
    
    // 创建长按定时器
    esp_timer_handle_t timer;
    esp_timer_create_args_t timer_args = {
        .callback = LongPressTimerCallback,
        .arg = reinterpret_cast<void*>(static_cast<intptr_t>(button_count_)),
        .dispatch_method = ESP_TIMER_TASK,
        .name = "long_press_timer",
        .skip_unhandled_events = true,
    };
    
    ret = esp_timer_create(&timer_args, &timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create long press timer: %s", esp_err_to_name(ret));
        return false;
    }
    
    // 保存配置
    buttons_[button_count_] = {
        .touch_num = touch_num,
        .threshold = threshold,
        .name = name ? name : "Unknown"
    };
    
    long_press_timer_[button_count_] = timer;
    
    ESP_LOGI(TAG, "Added touch button %d: %s (pad %d, threshold %.3f)", 
             button_count_, buttons_[button_count_].name, touch_num, threshold);
    
    button_count_++;
    return true;
}

void TouchButtonManager::SetCallback(TouchButtonCallback callback) {
    callback_ = callback;
}

bool TouchButtonManager::Start() {
    if (!initialized_ || running_) {
        return false;
    }
    
    ESP_LOGI(TAG, "Starting touch button task");
    
    // 创建触摸任务
    BaseType_t ret = xTaskCreate(TouchTask, "touch_task", 4096, this, 5, &task_handle_);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create touch task");
        return false;
    }
    
    running_ = true;
    return true;
}

void TouchButtonManager::Stop() {
    if (!running_) {
        return;
    }
    
    ESP_LOGI(TAG, "Stopping touch button task");
    
    running_ = false;
    
    if (task_handle_) {
        vTaskDelete(task_handle_);
        task_handle_ = nullptr;
    }
}

bool TouchButtonManager::IsButtonPressed(int button_id) {
    if (button_id < 0 || button_id >= button_count_) {
        return false;
    }
    
    uint32_t touch_value;
    esp_err_t ret = touch_pad_read_raw_data(buttons_[button_id].touch_num, &touch_value);
    if (ret != ESP_OK) {
        return false;
    }
    
    return touch_value < buttons_[button_id].threshold;
}

const char* TouchButtonManager::GetButtonName(int button_id) {
    if (button_id < 0 || button_id >= button_count_) {
        return "Invalid";
    }
    
    return buttons_[button_id].name;
}

void TouchButtonManager::TouchTask(void* arg) {
    TouchButtonManager* manager = static_cast<TouchButtonManager*>(arg);
    manager->ProcessTouch();
}

void TouchButtonManager::ProcessTouch() {
    while (running_) {
        for (int i = 0; i < button_count_; i++) {
            uint32_t touch_value;
            esp_err_t ret = touch_pad_read_raw_data(buttons_[i].touch_num, &touch_value);
            if (ret != ESP_OK) {
                continue;
            }
            
            // 滤波处理
            filter_values_[i][filter_index_[i]] = touch_value;
            filter_index_[i] = (filter_index_[i] + 1) % FILTER_COUNT;
            
            // 计算平均值
            uint32_t avg_value = 0;
            for (int j = 0; j < FILTER_COUNT; j++) {
                avg_value += filter_values_[i][j];
            }
            avg_value /= FILTER_COUNT;
            
            // 判断按键状态
            bool current_state = (avg_value < buttons_[i].threshold);
            
            // 防抖处理
            if (current_state != last_state_[i]) {
                if (current_state) {
                    debounce_counter_[i]++;
                    if (debounce_counter_[i] >= DEBOUNCE_COUNT) {
                        // 按键按下
                        HandleButtonEvent(i, TOUCH_BUTTON_EVT_PRESS);
                        last_state_[i] = true;
                        debounce_counter_[i] = 0;
                        
                        // 启动长按定时器
                        if (long_press_timer_[i]) {
                            esp_timer_start_once(long_press_timer_[i], LONG_PRESS_TIME_MS * 1000);
                        }
                    }
                } else {
                    debounce_counter_[i]++;
                    if (debounce_counter_[i] >= DEBOUNCE_COUNT) {
                        // 按键释放
                        HandleButtonEvent(i, TOUCH_BUTTON_EVT_RELEASE);
                        last_state_[i] = false;
                        debounce_counter_[i] = 0;
                        
                        // 停止长按定时器
                        if (long_press_timer_[i]) {
                            esp_timer_stop(long_press_timer_[i]);
                        }
                    }
                }
            } else {
                debounce_counter_[i] = 0;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(20)); // 50Hz采样率
    }
}

void TouchButtonManager::HandleButtonEvent(int button_id, touch_button_event_t event) {
    ESP_LOGD(TAG, "Button %d (%s) event: %d", button_id, GetButtonName(button_id), event);
    
    if (callback_) {
        callback_(button_id, event);
    }
}

void TouchButtonManager::LongPressTimerCallback(void* arg) {
    int button_id = static_cast<int>(reinterpret_cast<intptr_t>(arg));
    GetInstance().HandleButtonEvent(button_id, TOUCH_BUTTON_EVT_LONG_PRESS);
}