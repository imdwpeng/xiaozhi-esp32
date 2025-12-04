#ifndef SWIPE_GESTURE_MANAGER_H
#define SWIPE_GESTURE_MANAGER_H

#include <driver/touch_pad.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <functional>
#include <esp_timer.h>
#include <vector>
#include <array>

// 滑动手势方向
typedef enum {
    SWIPE_DIRECTION_LEFT = 0,
    SWIPE_DIRECTION_RIGHT,
    SWIPE_DIRECTION_UP,
    SWIPE_DIRECTION_DOWN,
    SWIPE_DIRECTION_NONE
} swipe_direction_t;

// 滑动手势事件
typedef struct {
    swipe_direction_t direction;
    float velocity;          // 滑动速度
    uint32_t duration_ms;    // 滑动持续时间
    uint32_t timestamp;      // 时间戳
} swipe_gesture_event_t;

// 触摸点数据
typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t pressure;
    uint32_t timestamp;
} touch_point_t;

// 滑动手势回调函数类型
using SwipeGestureCallback = std::function<void(const swipe_gesture_event_t& event)>;

// 触摸通道配置
typedef struct {
    touch_pad_t channel;
    float threshold;
    const char* name;
} touch_channel_config_t;

class SwipeGestureManager {
public:
    static SwipeGestureManager& GetInstance();
    
    // 初始化和反初始化
    bool Initialize();
    void Deinitialize();
    
    // 配置触摸通道（用于多点触摸）
    bool AddTouchChannel(touch_pad_t channel, float threshold, const char* name);
    
    // 设置回调函数
    void SetSwipeCallback(SwipeGestureCallback callback);
    
    // 启动和停止
    bool Start();
    void Stop();
    
    // 配置参数
    void SetSwipeThreshold(float threshold_pixels);
    void SetSwipeTimeoutMs(uint32_t timeout_ms);
    void SetMinTouchPoints(uint8_t min_points);
    
    // 获取状态
    bool IsRunning() const { return running_; }
    
private:
    SwipeGestureManager() = default;
    ~SwipeGestureManager() = default;
    SwipeGestureManager(const SwipeGestureManager&) = delete;
    SwipeGestureManager& operator=(const SwipeGestureManager&) = delete;
    
    // 触摸通道管理
    static const int MAX_TOUCH_CHANNELS = 4;
    std::array<touch_channel_config_t, MAX_TOUCH_CHANNELS> channels_;
    int channel_count_ = 0;
    
    // 手势检测相关
    static const int MAX_TOUCH_POINTS = 10;
    std::vector<touch_point_t> touch_trail_;
    uint32_t touch_start_time_ = 0;
    bool touch_active_ = false;
    
    // 配置参数
    float swipe_threshold_pixels_ = 50.0f;
    uint32_t swipe_timeout_ms_ = 500;
    uint8_t min_touch_points_ = 3;
    
    // 任务和队列
    TaskHandle_t task_handle_ = nullptr;
    QueueHandle_t event_queue_ = nullptr;
    
    // 回调函数
    SwipeGestureCallback swipe_callback_;
    
    // 状态
    bool initialized_ = false;
    bool running_ = false;
    
    // 内部函数
    static void TouchTask(void* arg);
    void ProcessTouch();
    void DetectGesture();
    void ProcessSwipeDirection();
    void ClearTouchTrail();
    
    // 手势识别算法
    swipe_direction_t CalculateSwipeDirection();
    float CalculateSwipeVelocity();
    
    // 滤波和平滑
    static const int FILTER_SIZE = 5;
    std::array<float, FILTER_SIZE> x_filter_;
    std::array<float, FILTER_SIZE> y_filter_;
    int filter_index_ = 0;
    bool filter_filled_ = false;
    
    // 触摸状态跟踪
    struct {
        bool touched;
        uint32_t last_timestamp;
        float last_x;
        float last_y;
        uint32_t touch_count;
    } touch_state_;
    
    // 事件处理
    void HandleSwipeGesture(swipe_direction_t direction, float velocity, uint32_t duration);
    void AddTouchEvent(uint32_t x, uint32_t y, uint32_t pressure);
    
    // ESP-IDF v5.4 触摸传感器API封装
    bool InitializeTouchSensor();
    bool ReadTouchChannel(int channel_index, uint32_t& value);
};

#endif // SWIPE_GESTURE_MANAGER_H