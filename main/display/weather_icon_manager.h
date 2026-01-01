#ifndef WEATHER_ICON_MANAGER_H
#define WEATHER_ICON_MANAGER_H

#include <esp_err.h>
#include <esp_mmap_assets.h>
#include <lvgl.h>

class WeatherIconManager {
public:
    static WeatherIconManager& GetInstance();
    
    esp_err_t Init();
    esp_err_t UpdateWeatherIcon(lv_obj_t* icon_obj, const char* icon_code);
    
private:
    WeatherIconManager() = default;
    ~WeatherIconManager() = default;
    
    mmap_assets_handle_t asset_weather_;
    bool initialized_ = false;
    
    int FindIconIndex(const char* icon_code);
};

#endif // WEATHER_ICON_MANAGER_H