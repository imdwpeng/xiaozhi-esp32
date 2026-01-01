#pragma once

#include <string>
#include <functional>
#include <esp_err.h>
#include <esp_http_client.h>

#ifdef __cplusplus
extern "C" {
#endif

// 天气信息结构体
typedef struct {
    char temp[8];        // 温度，如 "15°"
    char weather[16];    // 天气描述，如 "多云"
    char icon_code[8];   // 天气图标代码，如 "104"
    char humidity[8];    // 湿度
    char wind_speed[8];  // 风速
} weather_info_t;

// 天气数据更新回调函数类型
typedef std::function<void()> weather_update_callback_t;


// 天气管理器状态
typedef enum {
    WEATHER_MANAGER_STATE_NOT_INITIALIZED = 0,
    WEATHER_MANAGER_STATE_INITIALIZED,
    WEATHER_MANAGER_STATE_FETCHING,
    WEATHER_MANAGER_STATE_READY,
    WEATHER_MANAGER_STATE_ERROR
} weather_manager_state_t;

#ifdef __cplusplus
}

class WeatherManager {
public:
    static WeatherManager& GetInstance();
    
    // 初始化天气管理器
    esp_err_t Init();
    
    // 获取天气数据
    esp_err_t FetchWeather();
    
    // 获取当前天气信息
    esp_err_t GetWeatherInfo(weather_info_t* info);
    
    // 获取状态
    weather_manager_state_t GetState();
    
    // 检查是否已获取天气数据
    bool IsWeatherAvailable();
    
    // 设置天气数据更新回调函数
    void SetUpdateCallback(weather_update_callback_t callback);
    
private:
    WeatherManager() = default;
    ~WeatherManager() = default;
    WeatherManager(const WeatherManager&) = delete;
    WeatherManager& operator=(const WeatherManager&) = delete;
    
    weather_manager_state_t state_;
    weather_info_t current_weather_;
    weather_update_callback_t update_callback_ = nullptr;
    
    // HTTP事件处理函数
    static esp_err_t HttpEventHandler(esp_http_client_event_t* evt);
    
    // 解析天气JSON数据
    esp_err_t ParseWeatherData(const char* json_data);
    
    // 静态数据指针，用于HTTP回调
    static WeatherManager* instance_;
    static char* http_response_data_;
    static size_t http_response_len_;
};

#endif