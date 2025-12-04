#ifndef TOUCH_BUTTON_MANAGER_H
#define TOUCH_BUTTON_MANAGER_H

#include <driver/touch_pad.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <functional>
#include <esp_timer.h>

// 触摸按键事件类型
typedef enum {
    TOUCH_BUTTON_EVT_PRESS = 0,      // 按下
    TOUCH_BUTTON_EVT_RELEASE,         // 释放
    TOUCH_BUTTON_EVT_LONG_PRESS,      // 长按
    TOUCH_BUTTON_EVT_REPEAT,          // 重复按下
} touch_button_event_t;

// 触摸按键配置
typedef struct {
    touch_pad_t touch_num;    // 触摸通道号
    float threshold;           // 触摸阈值
    const char* name;         // 按键名称
} touch_button_config_t;

// 触摸按键回调函数类型
using TouchButtonCallback = std::function<void(int button_id, touch_button_event_t event)>;

class TouchButtonManager {
public:
    static TouchButtonManager& GetInstance();
    
    // 初始化和反初始化
    bool Initialize();
    void Deinitialize();
    
    // 添加触摸按键
    bool AddButton(touch_pad_t touch_num, float threshold, const char* name);
    
    // 设置回调函数
    void SetCallback(TouchButtonCallback callback);
    
    // 启动和停止
    bool Start();
    void Stop();
    
    // 获取按键状态
    bool IsButtonPressed(int button_id);
    const char* GetButtonName(int button_id);
    
private:
    TouchButtonManager() = default;
    ~TouchButtonManager() = default;
    TouchButtonManager(const TouchButtonManager&) = delete;
    TouchButtonManager& operator=(const TouchButtonManager&) = delete;
    
    // 触摸按键配置 (基于 ESP-SparkBot 硬件限制)
    static const int MAX_TOUCH_BUTTONS = 3;
    touch_button_config_t buttons_[MAX_TOUCH_BUTTONS];
    int button_count_ = 0;
    
    // 任务和队列
    TaskHandle_t task_handle_ = nullptr;
    QueueHandle_t event_queue_ = nullptr;
    
    // 回调函数
    TouchButtonCallback callback_;
    
    // 状态
    bool initialized_ = false;
    bool running_ = false;
    
    // 内部函数
    static void TouchTask(void* arg);
    void ProcessTouch();
    void HandleButtonEvent(int button_id, touch_button_event_t event);
    
    // 滤波相关
    static const int FILTER_COUNT = 5;
    uint32_t filter_values_[MAX_TOUCH_BUTTONS][FILTER_COUNT];
    int filter_index_[MAX_TOUCH_BUTTONS] = {0};
    
    // 防抖相关
    static const int DEBOUNCE_COUNT = 3;
    int debounce_counter_[MAX_TOUCH_BUTTONS] = {0};
    bool last_state_[MAX_TOUCH_BUTTONS] = {false};
    
    // 长按检测
    esp_timer_handle_t long_press_timer_[MAX_TOUCH_BUTTONS] = {nullptr};
    static const int LONG_PRESS_TIME_MS = 1000;
    
    // 定时器回调
    static void LongPressTimerCallback(void* arg);
};

#endif // TOUCH_BUTTON_MANAGER_H