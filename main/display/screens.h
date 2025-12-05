#ifndef SCREENS_H
#define SCREENS_H

#include "screen_manager.h"
#include "time_manager.h"
#include "weather_manager.h"
#include "weather_widget.h"
#include "date_widget.h"
#include "flight_game_widget.h"
#include <lvgl.h>

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
    // 使用独立的插件
    DateWidget date_widget_;
    WeatherWidget weather_widget_;
    
    // 定时器
    lv_timer_t* time_timer_;
    lv_timer_t* weather_timer_;
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