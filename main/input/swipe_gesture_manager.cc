#include "swipe_gesture_manager.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <algorithm>
#include <cmath>
#include <cstring>

static const char* TAG = "SwipeGestureManager";

SwipeGestureManager& SwipeGestureManager::GetInstance() {
    static SwipeGestureManager instance;
    return instance;
}

bool SwipeGestureManager::Initialize() {
    if (initialized_) {
        return true;
    }
    
    ESP_LOGI(TAG, "Initializing swipe gesture manager for ESP32-S3");
    
    // 初始化触摸传感器
    if (!InitializeTouchSensor()) {
        ESP_LOGE(TAG, "Failed to initialize touch sensor");
        return false;
    }
    
    // 创建事件队列
    event_queue_ = xQueueCreate(5, sizeof(swipe_gesture_event_t));
    if (!event_queue_) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return false;
    }
    
    // 初始化滤波器
    x_filter_.fill(0.0f);
    y_filter_.fill(0.0f);
    filter_index_ = 0;
    filter_filled_ = false;
    
    // 初始化触摸状态
    memset(&touch_state_, 0, sizeof(touch_state_));
    
    channel_count_ = 0;
    touch_trail_.clear();
    touch_active_ = false;
    
    initialized_ = true;
    
    ESP_LOGI(TAG, "Swipe gesture manager initialized");
    return true;
}

void SwipeGestureManager::Deinitialize() {
    Stop();
    
    if (event_queue_) {
        vQueueDelete(event_queue_);
        event_queue_ = nullptr;
    }
    
    // 反初始化触摸传感器
    touch_pad_deinit();
    
    ClearTouchTrail();
    
    initialized_ = false;
    
    ESP_LOGI(TAG, "Swipe gesture manager deinitialized");
}

bool SwipeGestureManager::AddTouchChannel(touch_pad_t channel, float threshold, const char* name) {
    if (!initialized_ || channel_count_ >= MAX_TOUCH_CHANNELS) {
        ESP_LOGE(TAG, "Cannot add channel: not initialized or too many channels");
        return false;
    }
    
    // 配置触摸通道
    esp_err_t ret = touch_pad_config(channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to config touch pad %d: %s", channel, esp_err_to_name(ret));
        return false;
    }
    
    // 设置阈值
    ret = touch_pad_set_thresh(channel, (uint32_t)threshold);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set threshold for touch pad %d: %s", channel, esp_err_to_name(ret));
        return false;
    }
    
    // 配置滤波器
    touch_filter_config_t filter_config = {
        .mode = TOUCH_PAD_FILTER_IIR_16,
        .debounce_cnt = 1,
        .noise_thr = 0,
        .jitter_step = 1,
        .smh_lvl = TOUCH_PAD_SMOOTH_IIR_2,
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
    
    channels_[channel_count_] = {
        .channel = channel,
        .threshold = threshold,
        .name = name ? name : "Unknown"
    };
    
    ESP_LOGI(TAG, "Added touch channel %d: %s (threshold %.3f)", 
             channel_count_, channels_[channel_count_].name, threshold);
    
    channel_count_++;
    return true;
}

void SwipeGestureManager::SetSwipeCallback(SwipeGestureCallback callback) {
    swipe_callback_ = callback;
}

bool SwipeGestureManager::Start() {
    if (!initialized_ || running_) {
        return false;
    }
    
    ESP_LOGI(TAG, "Starting swipe gesture detection");
    
    // 触摸传感器已通过touch_pad_fsm_start启动
    
    // 创建触摸任务
    BaseType_t task_ret = xTaskCreate(TouchTask, "swipe_gesture_task", 4096, this, 6, &task_handle_);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create swipe gesture task");
        touch_pad_fsm_stop();
        return false;
    }
    
    running_ = true;
    return true;
}

void SwipeGestureManager::Stop() {
    if (!running_) {
        return;
    }
    
    ESP_LOGI(TAG, "Stopping swipe gesture detection");
    
    running_ = false;
    
    if (task_handle_) {
        vTaskDelete(task_handle_);
        task_handle_ = nullptr;
    }
    
    touch_pad_fsm_stop();
}

void SwipeGestureManager::SetSwipeThreshold(float threshold_pixels) {
    swipe_threshold_pixels_ = threshold_pixels;
}

void SwipeGestureManager::SetSwipeTimeoutMs(uint32_t timeout_ms) {
    swipe_timeout_ms_ = timeout_ms;
}

void SwipeGestureManager::SetMinTouchPoints(uint8_t min_points) {
    min_touch_points_ = min_points;
}

void SwipeGestureManager::TouchTask(void* arg) {
    SwipeGestureManager* manager = static_cast<SwipeGestureManager*>(arg);
    manager->ProcessTouch();
}

void SwipeGestureManager::ProcessTouch() {
    while (running_) {
        uint32_t current_time = esp_timer_get_time() / 1000; // 转换为毫秒
        
        // 检查所有触摸通道
        bool any_touched = false;
        float total_x = 0;
        float total_y = 0;
        int touched_count = 0;
        
        for (int i = 0; i < channel_count_; i++) {
            uint32_t touch_value;
            if (ReadTouchChannel(i, touch_value)) {
                // 简化的坐标映射：使用通道索引和触摸值估算位置
                if (touch_value < channels_[i].threshold) {
                    any_touched = true;
                    touched_count++;
                    
                    // 模拟X坐标（基于通道索引）
                    float x = i * 100.0f; // 每个通道间隔100像素
                    
                    // 模拟Y坐标（基于触摸强度）
                    float y = (channels_[i].threshold - touch_value) * 2.0f;
                    
                    total_x += x;
                    total_y += y;
                }
            }
        }
        
        if (any_touched) {
            if (!touch_active_) {
                // 触摸开始
                touch_active_ = true;
                touch_start_time_ = current_time;
                ClearTouchTrail();
                touch_state_.touch_count = 1;
            } else {
                touch_state_.touch_count++;
            }
            
            // 计算平均位置
            if (touched_count > 0) {
                float avg_x = total_x / touched_count;
                float avg_y = total_y / touched_count;
                
                // 添加到轨迹
                AddTouchEvent(static_cast<uint32_t>(avg_x), 
                             static_cast<uint32_t>(avg_y), 
                             touched_count * 100);
            }
            
            touch_state_.last_timestamp = current_time;
        } else {
            if (touch_active_) {
                // 触摸结束，检测手势
                touch_active_ = false;
                DetectGesture();
            }
        }
        
        // 检查超时
        if (touch_active_ && (current_time - touch_start_time_) > swipe_timeout_ms_) {
            touch_active_ = false;
            ClearTouchTrail();
        }
        
        vTaskDelay(pdMS_TO_TICKS(16)); // 60Hz采样率
    }
}

void SwipeGestureManager::AddTouchEvent(uint32_t x, uint32_t y, uint32_t pressure) {
    touch_point_t point = {
        .x = x,
        .y = y,
        .pressure = pressure,
        .timestamp = esp_timer_get_time() / 1000
    };
    
    touch_trail_.push_back(point);
    
    // 限制轨迹长度
    while (touch_trail_.size() > MAX_TOUCH_POINTS) {
        touch_trail_.erase(touch_trail_.begin());
    }
}

void SwipeGestureManager::DetectGesture() {
    if (touch_trail_.size() < min_touch_points_) {
        ClearTouchTrail();
        return;
    }
    
    // 计算滑动方向和速度
    swipe_direction_t direction = CalculateSwipeDirection();
    if (direction != SWIPE_DIRECTION_NONE) {
        float velocity = CalculateSwipeVelocity();
        uint32_t duration = touch_trail_.back().timestamp - touch_trail_.front().timestamp;
        
        HandleSwipeGesture(direction, velocity, duration);
    }
    
    ClearTouchTrail();
}

swipe_direction_t SwipeGestureManager::CalculateSwipeDirection() {
    if (touch_trail_.size() < 2) {
        return SWIPE_DIRECTION_NONE;
    }
    
    const touch_point_t& start = touch_trail_.front();
    const touch_point_t& end = touch_trail_.back();
    
    float dx = static_cast<float>(end.x) - static_cast<float>(start.x);
    float dy = static_cast<float>(end.y) - static_cast<float>(start.y);
    
    // 检查移动距离是否足够
    float distance = std::sqrt(dx * dx + dy * dy);
    if (distance < swipe_threshold_pixels_) {
        return SWIPE_DIRECTION_NONE;
    }
    
    // 确定主要方向
    if (std::abs(dx) > std::abs(dy)) {
        // 水平滑动
        return dx > 0 ? SWIPE_DIRECTION_RIGHT : SWIPE_DIRECTION_LEFT;
    } else {
        // 垂直滑动
        return dy > 0 ? SWIPE_DIRECTION_DOWN : SWIPE_DIRECTION_UP;
    }
}

float SwipeGestureManager::CalculateSwipeVelocity() {
    if (touch_trail_.size() < 2) {
        return 0.0f;
    }
    
    const touch_point_t& start = touch_trail_.front();
    const touch_point_t& end = touch_trail_.back();
    
    float dx = static_cast<float>(end.x) - static_cast<float>(start.x);
    float dy = static_cast<float>(end.y) - static_cast<float>(start.y);
    float distance = std::sqrt(dx * dx + dy * dy);
    
    uint32_t time_diff = end.timestamp - start.timestamp;
    if (time_diff == 0) {
        return 0.0f;
    }
    
    return distance / static_cast<float>(time_diff) * 1000.0f; // pixels per second
}

void SwipeGestureManager::HandleSwipeGesture(swipe_direction_t direction, float velocity, uint32_t duration) {
    ESP_LOGI(TAG, "Swipe detected: direction=%d, velocity=%.2f, duration=%dms", 
             direction, velocity, duration);
    
    if (swipe_callback_) {
        swipe_gesture_event_t event = {
            .direction = direction,
            .velocity = velocity,
            .duration_ms = duration,
            .timestamp = esp_timer_get_time() / 1000
        };
        
        swipe_callback_(event);
    }
}

void SwipeGestureManager::ClearTouchTrail() {
    touch_trail_.clear();
}

bool SwipeGestureManager::InitializeTouchSensor() {
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
    
    return true;
}

bool SwipeGestureManager::ReadTouchChannel(int channel_index, uint32_t& value) {
    if (channel_index < 0 || channel_index >= channel_count_) {
        return false;
    }
    
    esp_err_t ret = touch_pad_read_raw_data(channels_[channel_index].channel, &value);
    if (ret != ESP_OK) {
        return false;
    }
    
    return true;
}