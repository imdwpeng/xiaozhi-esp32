#ifndef WEATHER_WIDGET_H
#define WEATHER_WIDGET_H

#include "weather_manager.h"
#include <lvgl.h>
#include <cstring>

// 声明字体
LV_FONT_DECLARE(lv_font_montserrat_24);
LV_FONT_DECLARE(lv_font_montserrat_14);

class WeatherWidget {
public:
    WeatherWidget();
    ~WeatherWidget();
    
    // 创建天气小部件
    void Create(lv_obj_t* parent);
    
    // 销毁天气小部件
    void Destroy();
    
    // 更新天气数据
    void UpdateWeather();
    
    // 设置位置
    void SetPosition(int x, int y);
    
    // 获取小部件根对象
    lv_obj_t* GetRoot() { return container_; }
    
    // 获取天气信息
    const char* GetTemperature() { return current_temp_; }
    const char* GetWeather() { return current_weather_; }
    
private:
    lv_obj_t* container_ = nullptr;
    lv_obj_t* temp_label_ = nullptr;
    lv_obj_t* weather_label_ = nullptr;
    lv_obj_t* weather_icon_ = nullptr;
    
    char current_temp_[8];
    char current_weather_[16];
    
    // 更新天气显示
    void UpdateWeatherDisplay();
    
    // 根据天气代码获取图标
    const char* GetWeatherIcon(const char* icon_code);
};

#endif // WEATHER_WIDGET_H