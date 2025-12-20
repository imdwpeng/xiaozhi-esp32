#ifndef SIMPLE_TOUCH_MANAGER_H
#define SIMPLE_TOUCH_MANAGER_H

#include <driver/touch_pad.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <functional>

// 触摸按键事件类型
typedef enum {
    SIMPLE_TOUCH_PRESS = 0,    // 按下
    SIMPLE_TOUCH_RELEASE,      // 释放
} simple_touch_event_t;

// 触摸按键回调函数类型
using SimpleTouchCallback = std::function<void(int button_id, simple_touch_event_t event)>;

class SimpleTouchManager {
public:
    static SimpleTouchManager& GetInstance();
    
    // 初始化
    bool Initialize();
    
    // 添加触摸按键
    bool AddButton(touch_pad_t pad_num, uint32_t threshold, const char* name);
    
    // 设置回调函数
    void SetCallback(SimpleTouchCallback callback);
    
    // 设置游戏模式回调
    void SetGameModeCallback(SimpleTouchCallback callback);
    
    // 切换回调模式
    void SetMode(int mode); // 0=normal, 1=game, 2=pause
    
    // 启动检测
    bool Start();
    void Stop();
    
    // 诊断函数
    void DiagnosticScanAllTouchPads();
    
private:
    static const int MAX_BUTTONS = 3;
    struct ButtonInfo {
        touch_pad_t pad;
        uint32_t threshold;
        uint32_t baseline;
        const char* name;
        bool last_state;
        uint32_t debounce_count;
    };
    
    ButtonInfo buttons_[MAX_BUTTONS];
    int button_count_;
    bool initialized_;
    volatile bool running_;
    SemaphoreHandle_t running_mutex_;
    TaskHandle_t task_handle_;
    SimpleTouchCallback callback_;
    SimpleTouchCallback game_callback_;
    int current_mode_;
    
    static void TouchTask(void* arg);
    void ProcessTouch();
};

#endif // SIMPLE_TOUCH_MANAGER_H