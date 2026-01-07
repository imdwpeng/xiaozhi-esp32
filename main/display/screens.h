#ifndef SCREENS_H
#define SCREENS_H

#include "screen_manager.h"
#include "time_manager.h"
#include "weather_manager.h"
#include "weather_widget.h"
#include "date_widget.h"
#include "flight_game_widget.h"
#include <lvgl.h>
#include <string>
#include <memory>

// 字体声明
LV_FONT_DECLARE(lv_font_montserrat_14);
LV_FONT_DECLARE(lv_font_montserrat_16);
LV_FONT_DECLARE(lv_font_montserrat_18);
LV_FONT_DECLARE(lv_font_montserrat_20);
LV_FONT_DECLARE(lv_font_montserrat_24);
LV_FONT_DECLARE(lv_font_montserrat_32);
LV_FONT_DECLARE(lv_font_montserrat_48);

// 使用支持中文的内置字体
LV_FONT_DECLARE(BUILTIN_TEXT_FONT);
LV_FONT_DECLARE(font_noto_basic_20_4);  // 更完整的中文字体
LV_FONT_DECLARE(font_puhui_basic_20_4);  // 更大的中文字体，可能包含城市名称
LV_FONT_DECLARE(font_puhui_16_4);  // 完整的中文字体，包含杭、州等字符（大小适中：5.3MB）
LV_FONT_DECLARE(font_puhui_20_4);  // 完整的中文字体，包含杭、州等字符（较大：7.5MB）

// 主屏幕
class MainScreen : public Screen {
public:
    MainScreen(const char* name = "Main");
    virtual ~MainScreen() = default;
    
    void Create() override;
    void Destroy() override;
    void Show() override;
    void Hide() override;
    void HandleEvent(screen_event_t event) override;
    
private:
    lv_obj_t* title_label_;
    lv_obj_t* info_label_;
    lv_obj_t* hint_label_;
};

// 空屏幕（用于演示）
class EmptyScreen : public Screen {
public:
    EmptyScreen(const char* name = "Empty");
    virtual ~EmptyScreen() = default;
    
    void Create() override;
    void Destroy() override;
    void Show() override;
    void Hide() override;
    void HandleEvent(screen_event_t event) override;
    
private:
    lv_obj_t* title_label_;
    lv_obj_t* screen_number_label_;
    int screen_number_;
};

// 设置屏幕
class SettingsScreen : public Screen {
public:
    SettingsScreen(const char* name = "Settings");
    virtual ~SettingsScreen() = default;
    
    void Create() override;
    void Destroy() override;
    void Show() override;
    void Hide() override;
    void HandleEvent(screen_event_t event) override;
    
private:
    lv_obj_t* title_label_;
    lv_obj_t* back_label_;
};

// 天气时钟屏幕 - 使用独立的插件
class WeatherClockScreen : public Screen {
public:
    WeatherClockScreen(const char* name = "WeatherClock");
    virtual ~WeatherClockScreen() = default;
    
    void Create() override;
    void Destroy() override;
    void Show() override;
    void Hide() override;
    void HandleEvent(screen_event_t event) override;
    
private:
    // UI元素
    lv_obj_t* main_panel_;  // 主面板容器
    lv_obj_t* hour_label_;
    lv_obj_t* min_label_;
    lv_obj_t* colon_label_;
    lv_obj_t* date_label_;
    lv_obj_t* weekday_label_;
    lv_obj_t* temp_label_;
    lv_obj_t* real_weather_label_;  // 真实天气数据标签
    lv_obj_t* weather_icon_;  // 天气图标
    lv_obj_t* location_label_;
    lv_obj_t* aqi_container_;  // 空气质量背景容器
    lv_obj_t* aqi_label_;  // 空气质量标签
    lv_obj_t* temp_max_label_;  // 最高温度标签
    lv_obj_t* temp_min_label_;  // 最低温度标签
    
    // 定时器
    lv_timer_t* time_timer_;
    lv_timer_t* weather_timer_;
    
    // 更新方法
    void UpdateTimeDisplay();
    void UpdateWeatherDisplay();
    void UpdateWeatherIconImage(const char* icon_code);
    const char* GetWeekdayName(int weekday);
    const char* GetWeatherIcon(const char* icon_code);
};

// 飞行游戏屏幕 - 使用独立的游戏插件
class FlightGameScreen : public Screen {
public:
    FlightGameScreen(const char* name = "FlightGame");
    virtual ~FlightGameScreen() = default;
    
    void Create() override;
    void Destroy() override;
    void Show() override;
    void Hide() override;
    void HandleEvent(screen_event_t event) override;
    
private:
    std::unique_ptr<FlightGameWidget> game_widget_;
};

// 屏幕工厂
class ScreenFactory {
public:
    static void CreateAllScreens();
    static void InitializeScreens();
    static int GetNextScreenNumber() { return ++screen_counter_; }
    
private:
    static int screen_counter_;
};

#endif // SCREENS_H