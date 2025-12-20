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
    
    ESP_LOGI(TAG, "Initializing simple touch manager with legacy API (ESP-IDF 5.x compatibility)");
    
    // 创建互斥锁
    running_mutex_ = xSemaphoreCreateMutex();
    if (running_mutex_ == NULL) {
        ESP_LOGE(TAG, "Failed to create running mutex");
        return false;
    }
    
    // 初始化触摸外设
    esp_err_t ret = touch_pad_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize touch pad: %s", esp_err_to_name(ret));
        vSemaphoreDelete(running_mutex_);
        return false;
    }
    
    ESP_LOGI(TAG, "Touch pad initialized successfully");
    
    // 设置触摸电压配置
    ret = touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set touch voltage config: %s (continuing anyway)", esp_err_to_name(ret));
    }
    
    // 禁用睡眠模式下的触摸通道，确保它们在运行时可用
    touch_pad_sleep_channel_enable(TOUCH_PAD_NUM1, false);
    touch_pad_sleep_channel_enable(TOUCH_PAD_NUM2, false);
    touch_pad_sleep_channel_enable(TOUCH_PAD_NUM3, false);
    touch_pad_sleep_channel_enable(TOUCH_PAD_NUM8, false);
    touch_pad_sleep_channel_enable(TOUCH_PAD_NUM9, false);
    touch_pad_sleep_channel_enable(TOUCH_PAD_NUM10, false);
    
    // 启动触摸传感器FSM
    ret = touch_pad_fsm_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start touch pad FSM: %s", esp_err_to_name(ret));
        vSemaphoreDelete(running_mutex_);
        return false;
    }
    
    ESP_LOGI(TAG, "Touch pad FSM started successfully");
    
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
    
    ESP_LOGI(TAG, "=== DIAGNOSTIC: Testing touch pad %d (%s) ===", pad_num, name ? name : "Unknown");
    
    // 首先测试原始读取
    for (int test = 0; test < 5; test++) {
        uint32_t raw_value;
        esp_err_t ret = touch_pad_read_raw_data(pad_num, &raw_value);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Test %d: Raw read pad %d = %lu", test, pad_num, raw_value);
        } else {
            ESP_LOGE(TAG, "Test %d: Failed to read raw data from pad %d: %s", test, pad_num, esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    // 配置触摸通道
    esp_err_t ret = touch_pad_config(pad_num);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to config touch pad %d: %s", pad_num, esp_err_to_name(ret));
        return false;
    }
    
    ESP_LOGI(TAG, "Touch pad %d configured successfully", pad_num);
    
    // 等待一段时间让传感器稳定
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // 再次测试配置后的读取
    for (int test = 0; test < 5; test++) {
        uint32_t raw_value;
        ret = touch_pad_read_raw_data(pad_num, &raw_value);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Post-config Test %d: Raw read pad %d = %lu", test, pad_num, raw_value);
        } else {
            ESP_LOGE(TAG, "Post-config Test %d: Failed to read from pad %d: %s", test, pad_num, esp_err_to_name(ret));
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    // 读取基准值（未触摸时的值）
    uint32_t baseline = 0;
    int valid_readings = 0;
    for (int i = 0; i < 20; i++) {
        uint32_t value;
        ret = touch_pad_read_raw_data(pad_num, &value);
        if (ret == ESP_OK) {
            baseline += value;
            valid_readings++;
            if (i < 5) {  // 显示前5次读取
                ESP_LOGI(TAG, "Baseline reading %d: pad %d = %lu", i, pad_num, value);
            }
        } else {
            ESP_LOGE(TAG, "Failed baseline reading %d from pad %d: %s", i, pad_num, esp_err_to_name(ret));
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    
    if (valid_readings > 0) {
        baseline /= valid_readings;
    } else {
        ESP_LOGE(TAG, "No valid readings from touch pad %d, cannot use this pad", pad_num);
        return false;
    }
    
    ESP_LOGI(TAG, "Final baseline for pad %d: %lu (from %d readings)", pad_num, baseline, valid_readings);
    
    // 设置阈值（基准值 + 阈值，因为触摸时值会变大）
    uint32_t touch_threshold = baseline + threshold;
    ret = touch_pad_set_thresh(pad_num, touch_threshold);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set threshold for touch pad %d: %s", pad_num, esp_err_to_name(ret));
        return false;
    }
    
    // 配置滤波器 - 基于ESP-SparkBot硬件优化，模拟factory_demo_v1效果
    touch_filter_config_t filter_config = {
        .mode = TOUCH_PAD_FILTER_IIR_16,
        .debounce_cnt = 0,  // 无防抖，最大响应速度
        .noise_thr = 0,     // 最小噪声阈值，最高敏感度
        .jitter_step = 1,   // 最小抖动步长
        .smh_lvl = TOUCH_PAD_SMOOTH_IIR_2,  // 使用可用的最小平滑
    };
    
    ret = touch_pad_filter_set_config(&filter_config);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to config filter: %s (continuing without filter)", esp_err_to_name(ret));
        // 不返回false，继续执行
    } else {
        // 启用滤波器
        ret = touch_pad_filter_enable();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to enable filter: %s (continuing without filter)", esp_err_to_name(ret));
        } else {
            ESP_LOGI(TAG, "Touch filter configured and enabled successfully");
        }
    }
    
    buttons_[button_count_] = {
        .pad = pad_num,
        .threshold = touch_threshold,
        .baseline = baseline,
        .name = name ? name : "Unknown",
        .last_state = false,
        .debounce_count = 0
    };
    
    ESP_LOGI(TAG, "Added touch button %d: %s (pad %d, baseline=%lu, threshold=%lu)", 
             button_count_, buttons_[button_count_].name, pad_num, baseline, touch_threshold);
    
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

void SimpleTouchManager::DiagnosticScanAllTouchPads() {
    ESP_LOGI(TAG, "=== ESP-IDF 5.x TOUCH PAD DIAGNOSTIC ===");
    ESP_LOGI(TAG, "Factory demo v1 works, so hardware is OK - testing ESP-IDF compatibility");
    
    // 重点测试ESP-SparkBot实际使用的pad，以及可能的其他连接
    touch_pad_t priority_pads[] = {
        TOUCH_PAD_NUM1, TOUCH_PAD_NUM2, TOUCH_PAD_NUM3,  // 原始配置
        TOUCH_PAD_NUM8, TOUCH_PAD_NUM9, TOUCH_PAD_NUM10, // 可能的替代配置
        TOUCH_PAD_NUM4, TOUCH_PAD_NUM5, TOUCH_PAD_NUM6,  // 其他常见GPIO
        TOUCH_PAD_NUM7
    };
    
    ESP_LOGI(TAG, "Testing priority pads first...");
    
    for (int i = 0; i < 9; i++) {
        touch_pad_t pad = priority_pads[i];
        
        ESP_LOGI(TAG, "--- Testing TOUCH_PAD_NUM%d ---", i);
        
        // 尝试配置这个pad
        esp_err_t ret = touch_pad_config(pad);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "TOUCH_PAD_NUM%d: Failed to config: %s", i, esp_err_to_name(ret));
            continue;
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
        
        // 尝试读取几次
        uint32_t sum = 0;
        int valid_reads = 0;
        for (int j = 0; j < 5; j++) {
            uint32_t value;
            ret = touch_pad_read_raw_data(pad, &value);
            if (ret == ESP_OK) {
                sum += value;
                valid_reads++;
                ESP_LOGI(TAG, "TOUCH_PAD_NUM%d: Read %d = %lu", i, j, value);
            } else {
                ESP_LOGE(TAG, "TOUCH_PAD_NUM%d: Read %d failed: %s", i, j, esp_err_to_name(ret));
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        
        if (valid_reads > 0) {
            uint32_t avg = sum / valid_reads;
            ESP_LOGI(TAG, "TOUCH_PAD_NUM%d: AVERAGE = %lu (%d/%d valid reads)", i, avg, valid_reads, 5);
            
            // 检查是否有变化（用手触摸测试）- 降低检测阈值增加灵敏度
            ESP_LOGI(TAG, "TOUCH_PAD_NUM%d: Please touch this pad now for testing...", i);
            int change_count = 0;
            for (int j = 0; j < 10; j++) {
                uint32_t value;
                ret = touch_pad_read_raw_data(pad, &value);
                if (ret == ESP_OK) {
                    int32_t diff = (int32_t)value - (int32_t)avg;
                    if (abs(diff) > 50) {  // 降低阈值到50，更敏感
                        ESP_LOGI(TAG, "TOUCH_PAD_NUM%d: CHANGE DETECTED! value=%lu, diff=%ld (count=%d)", i, value, diff, change_count + 1);
                        change_count++;
                    }
                }
                vTaskDelay(pdMS_TO_TICKS(200));
            }
            
            if (change_count > 0) {
                ESP_LOGI(TAG, "TOUCH_PAD_NUM%d: *** ACTIVE PAD DETECTED! *** %d changes detected", i, change_count);
            } else {
                ESP_LOGW(TAG, "TOUCH_PAD_NUM%d: NO CHANGES DETECTED - pad may not be connected", i);
            }
        } else {
            ESP_LOGW(TAG, "TOUCH_PAD_NUM%d: NO VALID READINGS - pad not available", i);
        }
        
        ESP_LOGI(TAG, "--- End testing TOUCH_PAD_NUM%d ---", i);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    ESP_LOGI(TAG, "=== END DIAGNOSTIC SCAN ===");
}

bool SimpleTouchManager::Start() {
    if (!initialized_) {
        return false;
    }
    
    if (xSemaphoreTake(running_mutex_, portMAX_DELAY) == pdTRUE) {
        if (running_) {
            xSemaphoreGive(running_mutex_);
            return false;
        }
        
        ESP_LOGI(TAG, "Starting touch detection task");
        
        BaseType_t ret = xTaskCreate(TouchTask, "simple_touch", 4096, this, 5, &task_handle_);
        if (ret != pdPASS) {
            ESP_LOGE(TAG, "Failed to create touch task");
            xSemaphoreGive(running_mutex_);
            return false;
        }
        
        running_ = true;
        xSemaphoreGive(running_mutex_);
        return true;
    }
    
    return false;
}

void SimpleTouchManager::Stop() {
    if (xSemaphoreTake(running_mutex_, portMAX_DELAY) == pdTRUE) {
        if (!running_) {
            xSemaphoreGive(running_mutex_);
            return;
        }
        
        ESP_LOGI(TAG, "Stopping touch detection task");
        running_ = false;
        xSemaphoreGive(running_mutex_);
        
        if (task_handle_) {
            vTaskDelete(task_handle_);
            task_handle_ = nullptr;
        }
    }
}

void SimpleTouchManager::TouchTask(void* arg) {
    SimpleTouchManager* manager = static_cast<SimpleTouchManager*>(arg);
    if (manager && manager->initialized_) {
        manager->ProcessTouch();
    } else {
        ESP_LOGE(TAG, "TouchTask called with invalid manager or uninitialized");
        vTaskDelete(NULL);
    }
}

void SimpleTouchManager::ProcessTouch() {
    bool should_continue = true;
    
    while (should_continue) {
        // 检查是否应该继续运行
        if (xSemaphoreTake(running_mutex_, portMAX_DELAY) == pdTRUE) {
            should_continue = running_;
            xSemaphoreGive(running_mutex_);
        } else {
            should_continue = false;
        }
        
        if (!should_continue) {
            break;
        }
        
        for (int i = 0; i < button_count_; i++) {
            uint32_t touch_value;
            esp_err_t ret = touch_pad_read_raw_data(buttons_[i].pad, &touch_value);
            if (ret != ESP_OK) {
                continue;
            }
            
            // 动态基准线校准 - 适应环境变化（模拟factory_demo_v1的自适应算法）
            static int baseline_update_counter[3] = {0};
            baseline_update_counter[i]++;
            if (baseline_update_counter[i] >= 100) {  // 每5秒更新一次基准线
                // 只有在长期未触摸时才更新基准线
                int32_t current_diff = (int32_t)touch_value - (int32_t)buttons_[i].baseline;
                if (abs(current_diff) < 20) {  // 稳定状态，更新基准线
                    buttons_[i].baseline = (buttons_[i].baseline * 7 + touch_value) / 8;  // 平滑更新
                    ESP_LOGI(TAG, "Touch %d (%s): baseline updated to %lu", 
                             i, buttons_[i].name, buttons_[i].baseline);
                }
                baseline_update_counter[i] = 0;
            }
            
            // 调试输出：打印触摸值（每2秒一次，减少日志）
            static int debug_counter = 0;
            debug_counter++;
            if (debug_counter % 100 == 0) { // 20Hz * 100 = 2秒
                int32_t diff = (int32_t)touch_value - (int32_t)buttons_[i].baseline;
                ESP_LOGI(TAG, "Touch %d (%s): value=%lu, baseline=%lu, diff=%ld", 
                         i, buttons_[i].name, touch_value, buttons_[i].baseline, diff);
            }
            
            // 计算当前差值
            int32_t diff = (int32_t)touch_value - (int32_t)buttons_[i].baseline;
            
            // 实时输出显著变化
            if (abs(diff) > 25) {  // 适中的阈值
                ESP_LOGI(TAG, "Touch %d (%s): CHANGE! value=%lu, baseline=%lu, diff=%ld", 
                         i, buttons_[i].name, touch_value, buttons_[i].baseline, diff);
            }
            
            // ESP-IDF 5.x优化检测算法 - 基于factory_demo_v1工作原理
            bool current_state = false;
            
            // 方法1: 主要检测 - 检测增大（触摸时电容增加）
            if (diff > 100) {  // 进一步降低阈值
                current_state = true;
            }
            // 方法2: 备用检测 - 检测减小（某些硬件设计）
            else if (diff < -100) {
                current_state = true;
            }
            // 方法3: 微小但稳定的变化检测
            else if (abs(diff) > 30) {  // 降低到30，更敏感
                // 对于微小变化，需要连续检测
                static int small_change_count[3] = {0};
                if (abs(diff) > 30) {
                    small_change_count[i]++;
                    if (small_change_count[i] >= 3) {  // 连续3次小变化即可
                        current_state = true;
                        small_change_count[i] = 0;
                    }
                } else {
                    small_change_count[i] = 0;
                }
            }
            
            // 优化的防抖处理 - 更快响应，模拟factory_demo_v1的即时响应
            if (current_state != buttons_[i].last_state) {
                if (current_state) {
                    buttons_[i].debounce_count++;
                    if (buttons_[i].debounce_count >= 1) {  // 立即响应，无延迟
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
                    if (buttons_[i].debounce_count >= 1) {  // 立即响应，无延迟
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