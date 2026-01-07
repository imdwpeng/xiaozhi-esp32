#include "screens.h"
#include "screen_manager.h"
#include "time_manager.h"
#include "flight_game_widget.h"
#include "location_manager.h"
#include "weather_icon_manager.h"
#include "lvgl_display/lvgl_theme.h"
#include "settings.h"
#include <esp_log.h>
#include <cstring>
#include <ctime>
#include <cstdio>
#include <lvgl.h>

// Flight game screen implementation

static const char* TAG = "Screens";

int ScreenFactory::screen_counter_ = 0;

// MainScreen 实现
MainScreen::MainScreen(const char* name) {
    name_ = name;
}

void MainScreen::Create() {
    ESP_LOGI(TAG, "Creating MainScreen");
    
    // 创建根容器，不使用 lv_scr_act() 作为父对象
    root_ = lv_obj_create(NULL);
    lv_obj_set_size(root_, LV_PCT(100), LV_PCT(100));
    lv_obj_center(root_);
    lv_obj_set_style_bg_color(root_, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
    lv_obj_set_style_border_width(root_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(root_, 20, LV_PART_MAIN);
    
    // 创建标题标签
    title_label_ = lv_label_create(root_);
    lv_label_set_text(title_label_, "小智 ESP32");
    lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(title_label_, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, 20);
    
    // 创建信息标签
    info_label_ = lv_label_create(root_);
    lv_label_set_text(info_label_, "ESP-SparkBot 触摸屏切换演示");
    lv_obj_set_style_text_font(info_label_, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(info_label_, lv_color_hex(0xcccccc), LV_PART_MAIN);
    lv_obj_align(info_label_, LV_ALIGN_CENTER, 0, -20);
    
    // 创建提示标签
    hint_label_ = lv_label_create(root_);
    lv_label_set_text(hint_label_, "触摸左右两侧切换屏幕");
    lv_obj_set_style_text_font(hint_label_, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(hint_label_, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_align(hint_label_, LV_ALIGN_BOTTOM_MID, 0, -20);
    
    // 添加手势支持
    lv_obj_add_event_cb(root_, ScreenManager::LvglEventHandler, LV_EVENT_ALL, nullptr);
}

void MainScreen::Destroy() {
    if (root_) {
        lv_obj_del(root_);
        root_ = nullptr;
    }
}

void MainScreen::Show() {
    // 屏幕显示由ScreenManager通过lv_scr_load处理
    // 这里只需要确保对象未被隐藏即可
    if (root_) {
        lv_obj_clear_flag(root_, LV_OBJ_FLAG_HIDDEN);
    }
}

void MainScreen::Hide() {
    // 屏幕将被替换，无需隐藏操作
    // 避免任何可能阻塞的LVGL操作
}

void MainScreen::HandleEvent(screen_event_t event) {
    ESP_LOGD(TAG, "MainScreen handling event: %d", event);
}

// EmptyScreen 实现
EmptyScreen::EmptyScreen(const char* name) {
    name_ = name;
    screen_number_ = ScreenFactory::GetNextScreenNumber();
}

void EmptyScreen::Create() {
    ESP_LOGI(TAG, "Creating EmptyScreen: %s", name_);
    
    // 创建根容器
    root_ = lv_obj_create(NULL);
    lv_obj_set_size(root_, LV_PCT(100), LV_PCT(100));
    lv_obj_center(root_);
    lv_obj_set_style_bg_color(root_, lv_color_hex(0x0a0a0a), LV_PART_MAIN);
    lv_obj_set_style_border_width(root_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(root_, 20, LV_PART_MAIN);
    
    // 创建标题标签
    title_label_ = lv_label_create(root_);
    lv_label_set_text(title_label_, name_);
    lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(title_label_, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, 30);
    
    // 创建屏幕编号标签
    screen_number_label_ = lv_label_create(root_);
    static char number_text[32];
    snprintf(number_text, sizeof(number_text), "屏幕编号: %d", screen_number_);
    lv_label_set_text(screen_number_label_, number_text);
    lv_obj_set_style_text_font(screen_number_label_, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(screen_number_label_, lv_color_hex(0xaaaaaa), LV_PART_MAIN);
    lv_obj_align(screen_number_label_, LV_ALIGN_CENTER, 0, 0);
    
    // 添加手势支持
    lv_obj_add_event_cb(root_, ScreenManager::LvglEventHandler, LV_EVENT_ALL, nullptr);
}

void EmptyScreen::Destroy() {
    if (root_) {
        lv_obj_del(root_);
        root_ = nullptr;
    }
}

void EmptyScreen::Show() {
    if (root_) {
        lv_obj_clear_flag(root_, LV_OBJ_FLAG_HIDDEN);
    }
}

void EmptyScreen::Hide() {
    // 屏幕将被替换，无需隐藏操作
    // 避免任何可能阻塞的LVGL操作
}

void EmptyScreen::HandleEvent(screen_event_t event) {
    ESP_LOGD(TAG, "EmptyScreen %s handling event: %d", name_, event);
}

// SettingsScreen 实现
SettingsScreen::SettingsScreen(const char* name) {
    name_ = name;
}

void SettingsScreen::Create() {
    ESP_LOGI(TAG, "Creating SettingsScreen");
    
    // 创建根容器
    root_ = lv_obj_create(NULL);
    lv_obj_set_size(root_, LV_PCT(100), LV_PCT(100));
    lv_obj_center(root_);
    lv_obj_set_style_bg_color(root_, lv_color_hex(0x2a2a2a), LV_PART_MAIN);
    lv_obj_set_style_border_width(root_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(root_, 20, LV_PART_MAIN);
    
    // 创建标题标签
    title_label_ = lv_label_create(root_);
    lv_label_set_text(title_label_, "设置");
    lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(title_label_, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, 20);
    
    // 创建返回提示标签
    back_label_ = lv_label_create(root_);
    lv_label_set_text(back_label_, "触摸左右两侧返回");
    lv_obj_set_style_text_font(back_label_, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(back_label_, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_align(back_label_, LV_ALIGN_BOTTOM_MID, 0, -20);
    
    // 添加手势支持
    lv_obj_add_event_cb(root_, ScreenManager::LvglEventHandler, LV_EVENT_ALL, nullptr);
}

void SettingsScreen::Destroy() {
    if (root_) {
        lv_obj_del(root_);
        root_ = nullptr;
    }
}

void SettingsScreen::Show() {
    if (root_) {
        lv_obj_clear_flag(root_, LV_OBJ_FLAG_HIDDEN);
    }
}

void SettingsScreen::Hide() {
    // 屏幕将被替换，无需隐藏操作
    // 避免任何可能阻塞的LVGL操作
}

void SettingsScreen::HandleEvent(screen_event_t event) {
    ESP_LOGD(TAG, "SettingsScreen handling event: %d", event);
}

// WeatherClockScreen 实现
WeatherClockScreen::WeatherClockScreen(const char* name) {
    name_ = name;
    time_timer_ = nullptr;
    weather_timer_ = nullptr;
}

void WeatherClockScreen::Create() {
    ESP_LOGI(TAG, "Creating WeatherClockScreen");

    // 使用内置字体（支持中文）- 使用 BUILTIN_TEXT_FONT
    const lv_font_t* text_font = &BUILTIN_TEXT_FONT;
    ESP_LOGI(TAG, "Using BUILTIN_TEXT_FONT: %p", text_font);

    // 创建根容器 - 参考esp_sparkbot的设计
    root_ = lv_obj_create(NULL);
    lv_obj_set_size(root_, 240, 240);
    lv_obj_set_align(root_, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(root_, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_border_width(root_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(root_, 0, LV_PART_MAIN);
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_SCROLLABLE);

    // 创建主面板 - 参考esp_sparkbot的Panel1设计
    main_panel_ = lv_obj_create(root_);
    lv_obj_set_width(main_panel_, 240);
    lv_obj_set_height(main_panel_, 240);
    lv_obj_set_align(main_panel_, LV_ALIGN_CENTER);
    lv_obj_clear_flag(main_panel_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(main_panel_, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(main_panel_, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(main_panel_, 255, LV_PART_MAIN);
    lv_obj_set_style_border_width(main_panel_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(main_panel_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_right(main_panel_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_top(main_panel_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(main_panel_, 0, LV_PART_MAIN);
    
    // 创建时间显示 - 小时（左）- 参考esp_sparkbot的布局
    hour_label_ = lv_label_create(main_panel_);
    lv_obj_set_width(hour_label_, LV_SIZE_CONTENT);
    lv_obj_set_height(hour_label_, LV_SIZE_CONTENT);
    lv_obj_set_x(hour_label_, -60);
    lv_obj_set_y(hour_label_, 0);
    lv_obj_set_align(hour_label_, LV_ALIGN_BOTTOM_MID);
    lv_label_set_text(hour_label_, "23");
    lv_obj_set_style_text_color(hour_label_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_opa(hour_label_, 255, LV_PART_MAIN);
    lv_obj_set_style_text_font(hour_label_, &lv_font_montserrat_48, LV_PART_MAIN);
    
    // 创建时间显示 - 分钟（右）- 参考esp_sparkbot的金色设计
    min_label_ = lv_label_create(main_panel_);
    lv_obj_set_width(min_label_, LV_SIZE_CONTENT);
    lv_obj_set_height(min_label_, LV_SIZE_CONTENT);
    lv_obj_set_x(min_label_, 60);
    lv_obj_set_y(min_label_, 0);
    lv_obj_set_align(min_label_, LV_ALIGN_BOTTOM_MID);
    lv_label_set_text(min_label_, "59");
    lv_obj_set_style_text_color(min_label_, lv_color_hex(0xF1BA3B), LV_PART_MAIN);
    lv_obj_set_style_text_opa(min_label_, 255, LV_PART_MAIN);
    lv_obj_set_style_text_font(min_label_, &lv_font_montserrat_48, LV_PART_MAIN);
    
    // 创建冒号 - 参考esp_sparkbot的闪烁设计
    colon_label_ = lv_label_create(main_panel_);
    lv_obj_set_width(colon_label_, LV_SIZE_CONTENT);
    lv_obj_set_height(colon_label_, LV_SIZE_CONTENT);
    lv_obj_set_x(colon_label_, 0);
    lv_obj_set_y(colon_label_, -8);
    lv_obj_set_align(colon_label_, LV_ALIGN_BOTTOM_MID);
    lv_label_set_text(colon_label_, ":");
    lv_obj_set_style_text_color(colon_label_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_opa(colon_label_, 255, LV_PART_MAIN);
    lv_obj_set_style_text_font(colon_label_, &lv_font_montserrat_48, LV_PART_MAIN);
    
    // 创建日期显示（居中偏左）- 参考esp_sparkbot的布局
    date_label_ = lv_label_create(main_panel_);
    lv_obj_set_width(date_label_, LV_SIZE_CONTENT);
    lv_obj_set_height(date_label_, LV_SIZE_CONTENT);
    lv_obj_set_align(date_label_, LV_ALIGN_CENTER);
    lv_obj_set_x(date_label_, -20);
    lv_obj_set_y(date_label_, 15);
    lv_label_set_text(date_label_, "02/20");
    lv_obj_set_style_text_color(date_label_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_opa(date_label_, 255, LV_PART_MAIN);
    lv_obj_set_style_text_font(date_label_, &lv_font_montserrat_18, LV_PART_MAIN);  // 数字用montserrat

    // 创建星期显示（日期右侧）- 参考factory_demo_v1的方式
    weekday_label_ = lv_label_create(main_panel_);
    lv_obj_set_width(weekday_label_, LV_SIZE_CONTENT);
    lv_obj_set_height(weekday_label_, LV_SIZE_CONTENT);
    lv_obj_set_align(weekday_label_, LV_ALIGN_CENTER);  // 使用 CENTER
    lv_obj_set_x(weekday_label_, 85);  // x=85 对应右侧位置
    lv_obj_set_y(weekday_label_, 15);  // y=15 和 date_label_ 同一行
    lv_label_set_text(weekday_label_, "周五");
    lv_obj_set_style_text_color(weekday_label_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_opa(weekday_label_, 255, LV_PART_MAIN);
    lv_obj_set_style_text_font(weekday_label_, &font_puhui_16_4, LV_PART_MAIN);  // 使用支持周等字符的字体
    lv_obj_clear_flag(weekday_label_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_invalidate(weekday_label_);  // 强制重绘
    ESP_LOGI(TAG, "Created weekday_label_ at %p", (void*)weekday_label_);

    // === 重新设计布局：三栏式布局 ===
    // 左栏：天气图标和描述
    // 中栏：温度和时间（主信息）
    // 右栏：位置和空气质量（辅助信息）

    // 1. 左栏：天气图标和描述（左侧）
    weather_icon_ = lv_image_create(main_panel_);
    lv_obj_set_width(weather_icon_, 80);  // 缩小图标尺寸
    lv_obj_set_height(weather_icon_, 80);
    lv_obj_set_align(weather_icon_, LV_ALIGN_LEFT_MID);
    lv_obj_set_x(weather_icon_, 10);  // 左侧留出间距
    lv_obj_set_y(weather_icon_, -10); // 稍微向上偏移

    // 天气描述标签（图标下方）
    real_weather_label_ = lv_label_create(main_panel_);
    lv_obj_set_width(real_weather_label_, LV_SIZE_CONTENT);
    lv_obj_set_height(real_weather_label_, LV_SIZE_CONTENT);
    lv_obj_set_align(real_weather_label_, LV_ALIGN_LEFT_MID);
    lv_obj_set_x(real_weather_label_, 10);
    lv_obj_set_y(real_weather_label_, 45);  // 图标下方
    lv_obj_set_style_text_color(real_weather_label_, lv_color_hex(0xF1BA3B), LV_PART_MAIN);
    lv_obj_set_style_text_opa(real_weather_label_, 255, LV_PART_MAIN);
    lv_obj_set_style_text_font(real_weather_label_, &font_puhui_16_4, LV_PART_MAIN);
    lv_obj_set_style_text_align(real_weather_label_, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_clear_flag(real_weather_label_, LV_OBJ_FLAG_HIDDEN);

    // 2. 中栏：温度和时间（居中）
    // 当前温度（大字体，居中偏上）
    temp_label_ = lv_label_create(main_panel_);
    lv_obj_set_width(temp_label_, LV_SIZE_CONTENT);
    lv_obj_set_height(temp_label_, LV_SIZE_CONTENT);
    lv_obj_set_align(temp_label_, LV_ALIGN_TOP_MID);
    lv_obj_set_x(temp_label_, 0);
    lv_obj_set_y(temp_label_, 20);  // 向下移动，避免与顶部元素重叠
    lv_label_set_text(temp_label_, "15℃");
    lv_obj_set_style_text_color(temp_label_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_opa(temp_label_, 255, LV_PART_MAIN);
    lv_obj_set_style_text_font(temp_label_, &font_puhui_16_4, LV_PART_MAIN);
    lv_obj_clear_flag(temp_label_, LV_OBJ_FLAG_HIDDEN);

    // 最高最低温度（当前温度下方）
    temp_max_label_ = lv_label_create(main_panel_);
    lv_obj_set_width(temp_max_label_, LV_SIZE_CONTENT);
    lv_obj_set_height(temp_max_label_, LV_SIZE_CONTENT);
    lv_obj_set_align(temp_max_label_, LV_ALIGN_TOP_MID);
    lv_obj_set_x(temp_max_label_, -20);  // 最高温度稍微左移
    lv_obj_set_y(temp_max_label_, 60);   // 当前温度下方
    lv_label_set_text(temp_max_label_, "↑--℃");
    lv_obj_set_style_text_color(temp_max_label_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_opa(temp_max_label_, 255, LV_PART_MAIN);
    lv_obj_set_style_text_font(temp_max_label_, &font_puhui_16_4, LV_PART_MAIN);
    lv_obj_set_style_text_align(temp_max_label_, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_clear_flag(temp_max_label_, LV_OBJ_FLAG_HIDDEN);

    temp_min_label_ = lv_label_create(main_panel_);
    lv_obj_set_width(temp_min_label_, LV_SIZE_CONTENT);
    lv_obj_set_height(temp_min_label_, LV_SIZE_CONTENT);
    lv_obj_set_align(temp_min_label_, LV_ALIGN_TOP_MID);
    lv_obj_set_x(temp_min_label_, 20);   // 最低温度稍微右移
    lv_obj_set_y(temp_min_label_, 60);   // 与最高温度同一行
    lv_label_set_text(temp_min_label_, "↓--℃");
    lv_obj_set_style_text_color(temp_min_label_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_opa(temp_min_label_, 255, LV_PART_MAIN);
    lv_obj_set_style_text_font(temp_min_label_, &font_puhui_16_4, LV_PART_MAIN);
    lv_obj_set_style_text_align(temp_min_label_, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_clear_flag(temp_min_label_, LV_OBJ_FLAG_HIDDEN);

    // 3. 右栏：位置和空气质量（右侧）
    // 位置信息（右上角）
    location_label_ = lv_label_create(main_panel_);
    lv_obj_set_width(location_label_, 120);  // 限制宽度
    lv_obj_set_height(location_label_, LV_SIZE_CONTENT);
    lv_obj_set_align(location_label_, LV_ALIGN_TOP_RIGHT);
    lv_obj_set_x(location_label_, -5);
    lv_obj_set_y(location_label_, 5);
    lv_obj_set_style_text_color(location_label_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_opa(location_label_, 255, LV_PART_MAIN);
    lv_obj_set_style_text_font(location_label_, &font_puhui_16_4, LV_PART_MAIN);
    lv_obj_set_style_text_align(location_label_, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_clear_flag(location_label_, LV_OBJ_FLAG_HIDDEN);

    // 空气质量背景容器（位置下方）
    aqi_container_ = lv_obj_create(main_panel_);
    lv_obj_set_width(aqi_container_, 60);
    lv_obj_set_height(aqi_container_, 25);
    lv_obj_set_align(aqi_container_, LV_ALIGN_TOP_RIGHT);
    lv_obj_set_x(aqi_container_, -5);
    lv_obj_set_y(aqi_container_, 25);
    
    // 设置默认背景颜色（优 - 绿色）
    lv_obj_set_style_bg_color(aqi_container_, lv_color_hex(0x00E400), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(aqi_container_, 255, LV_PART_MAIN);
    lv_obj_set_style_radius(aqi_container_, 5, LV_PART_MAIN);
    lv_obj_set_style_border_width(aqi_container_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(aqi_container_, 3, LV_PART_MAIN);
    lv_obj_clear_flag(aqi_container_, LV_OBJ_FLAG_HIDDEN);

    // 空气质量标签 - 在容器内居中显示
    aqi_label_ = lv_label_create(aqi_container_);
    lv_obj_set_width(aqi_label_, LV_SIZE_CONTENT);
    lv_obj_set_height(aqi_label_, LV_SIZE_CONTENT);
    lv_obj_set_align(aqi_label_, LV_ALIGN_CENTER);
    lv_obj_set_x(aqi_label_, 0);
    lv_obj_set_y(aqi_label_, 0);
    lv_label_set_text(aqi_label_, "优");
    lv_obj_set_style_text_color(aqi_label_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_opa(aqi_label_, 255, LV_PART_MAIN);
    lv_obj_set_style_text_font(aqi_label_, &font_puhui_16_4, LV_PART_MAIN);
    lv_obj_set_style_text_align(aqi_label_, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_clear_flag(aqi_label_, LV_OBJ_FLAG_HIDDEN);

    // 添加手势支持
    lv_obj_add_event_cb(root_, ScreenManager::LvglEventHandler, LV_EVENT_ALL, nullptr);
}



void WeatherClockScreen::Destroy() {
    // 停止定时器
    if (time_timer_) {
        lv_timer_del(time_timer_);
        time_timer_ = nullptr;
    }
    
    if (weather_timer_) {
        lv_timer_del(weather_timer_);
        weather_timer_ = nullptr;
    }
    
    if (root_) {
        lv_obj_del(root_);
        root_ = nullptr;
        main_panel_ = nullptr;
        hour_label_ = nullptr;
        min_label_ = nullptr;
        colon_label_ = nullptr;
        date_label_ = nullptr;
        weekday_label_ = nullptr;
        temp_label_ = nullptr;
        real_weather_label_ = nullptr;
        weather_icon_ = nullptr;
        location_label_ = nullptr;
        aqi_label_ = nullptr;
        temp_max_label_ = nullptr;
        temp_min_label_ = nullptr;
    }
}

void WeatherClockScreen::Show() {
    if (root_) {
        lv_obj_clear_flag(root_, LV_OBJ_FLAG_HIDDEN);
        
        // 初始化时间管理器
        if (time_manager_get_state() == TIME_MANAGER_STATE_NOT_INITIALIZED) {
            time_manager_init();
        }
        
        // 启动时间同步
        if (!time_manager_is_synced()) {
            time_manager_start_sync();
        }
        
        // 初始化天气管理器
        if (WeatherManager::GetInstance().GetState() == WEATHER_MANAGER_STATE_NOT_INITIALIZED) {
            WeatherManager::GetInstance().Init();
        }

        // 初始化位置管理器
        LocationManager::GetInstance().Init();

        // 初始化天气图标管理器
        esp_err_t ret = WeatherIconManager::GetInstance().Init();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize weather icon manager: %s", esp_err_to_name(ret));
        }

        // 设置初始天气图标
        if (weather_icon_) {
            UpdateWeatherIconImage("104");  // 使用"阴"天气的图标
        }

        // 立即更新时间显示
        UpdateTimeDisplay();


        // 立即更新天气显示（使用默认数据）
        UpdateWeatherDisplay();

        // 注册天气数据更新回调，当天气数据获取成功时立即更新UI
        WeatherManager::GetInstance().SetUpdateCallback([this]() {
            UpdateWeatherDisplay();
        });

        // 延迟获取位置和天气数据（等待屏幕显示后再获取，避免阻塞UI渲染）
        // 参考 factory_demo_v1：先获取位置，再获取天气
        lv_timer_create([](lv_timer_t* timer) {
            // 1. 先获取IP位置
            LocationManager::GetInstance().FetchLocationByIP();

            // 2. 再获取天气数据
            WeatherManager::GetInstance().FetchWeather();

            lv_timer_del(timer);
        }, 1000, nullptr);

        // 启动时间更新定时器（每秒更新）
        time_timer_ = lv_timer_create([](lv_timer_t* timer) {
            void* user_data = lv_timer_get_user_data(timer);
            WeatherClockScreen* screen = static_cast<WeatherClockScreen*>(user_data);
            screen->UpdateTimeDisplay();
        }, 1000, this);
        
        // 启动天气更新定时器（每30秒更新一次UI）
        weather_timer_ = lv_timer_create([](lv_timer_t* timer) {
            void* user_data = lv_timer_get_user_data(timer);
            WeatherClockScreen* screen = static_cast<WeatherClockScreen*>(user_data);
            screen->UpdateWeatherDisplay();
        }, 30000, this);

        // 启动天气数据获取定时器（每30分钟获取一次）
        // 参考 factory_demo_v1：先获取位置，再获取天气
        lv_timer_create([](lv_timer_t* timer) {
            // 1. 先获取IP位置
            LocationManager::GetInstance().FetchLocationByIP();

            // 2. 再获取天气数据
            WeatherManager::GetInstance().FetchWeather();
        }, 1800000, this);
    }
}

void WeatherClockScreen::Hide() {
    // 屏幕将被替换，无需隐藏操作
    // 避免任何可能阻塞的LVGL操作
    
    // 停止定时器（保留必要的清理）
    if (time_timer_) {
        lv_timer_del(time_timer_);
        time_timer_ = nullptr;
    }
    
    if (weather_timer_) {
        lv_timer_del(weather_timer_);
        weather_timer_ = nullptr;
    }
    
    // 清除天气更新回调
    WeatherManager::GetInstance().SetUpdateCallback(nullptr);
}

void WeatherClockScreen::HandleEvent(screen_event_t event) {
    ESP_LOGD(TAG, "WeatherClockScreen handling event: %d", event);
}

void WeatherClockScreen::UpdateTimeDisplay() {
    if (!root_) return;
    
    // 使用时间管理器获取时间
    struct tm timeinfo;
    esp_err_t ret = time_manager_get_time(&timeinfo);
    
    if (ret == ESP_OK) {
        // 格式化时间
        char hour_str[8];
        char min_str[8];
        char date_str[16];
        
        snprintf(hour_str, sizeof(hour_str), "%02d", timeinfo.tm_hour);
        snprintf(min_str, sizeof(min_str), "%02d", timeinfo.tm_min);
        strftime(date_str, sizeof(date_str), "%m/%d", &timeinfo);
        const char* weekday_name = GetWeekdayName(timeinfo.tm_wday);

        // 更新UI
        lv_label_set_text(hour_label_, hour_str);
        lv_label_set_text(min_label_, min_str);
        lv_label_set_text(date_label_, date_str);
        lv_label_set_text(weekday_label_, weekday_name);

        // 确保UI元素可见
        lv_obj_clear_flag(hour_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(min_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(date_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(weekday_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(colon_label_, LV_OBJ_FLAG_HIDDEN);
    }
}

// 根据背景颜色亮度计算文字颜色（浅色背景用深色文字，深色背景用浅色文字）
lv_color_t GetContrastTextColor(lv_color_t bg_color) {
    // 计算背景颜色的亮度（使用相对亮度公式）
    lv_color_hsv_t hsv = lv_color_to_hsv(bg_color);
    
    // 使用HSV中的亮度值（value）作为亮度指标
    float brightness = hsv.v * 255.0f; // hsv.v 范围是0-1，转换为0-255
    
    // 如果背景亮度大于128（浅色背景），使用黑色文字，否则使用白色文字
    return brightness > 128 ? lv_color_hex(0x000000) : lv_color_hex(0xFFFFFF);
}

// 根据空气质量等级设置背景颜色和文字颜色
void SetAQIBackgroundColor(lv_obj_t* container, lv_obj_t* label, const char* aqi_level) {
    if (!container || !label || !aqi_level) {
        return;
    }
    
    // 和风天气中国空气质量标准颜色（AQI CN）
    lv_color_t bg_color;
    
    // 根据空气质量等级设置不同颜色
    if (strstr(aqi_level, "优") != NULL) {
        bg_color = lv_color_hex(0x00E400);  // 优 - 绿色
    } else if (strstr(aqi_level, "良") != NULL) {
        bg_color = lv_color_hex(0xFFFF00);  // 良 - 黄色
    } else if (strstr(aqi_level, "轻度污染") != NULL) {
        bg_color = lv_color_hex(0xFF7E00);  // 轻度污染 - 橙色
    } else if (strstr(aqi_level, "中度污染") != NULL) {
        bg_color = lv_color_hex(0xFF0000);  // 中度污染 - 红色
    } else if (strstr(aqi_level, "重度污染") != NULL) {
        bg_color = lv_color_hex(0x99004C);  // 重度污染 - 紫色
    } else if (strstr(aqi_level, "严重污染") != NULL) {
        bg_color = lv_color_hex(0x7E0023);  // 严重污染 - 深红色
    } else {
        bg_color = lv_color_hex(0x00E400);  // 默认 - 绿色
    }
    
    // 设置背景颜色
    lv_obj_set_style_bg_color(container, bg_color, LV_PART_MAIN);
    
    // 根据背景颜色亮度设置文字颜色
    lv_color_t text_color = GetContrastTextColor(bg_color);
    lv_obj_set_style_text_color(label, text_color, LV_PART_MAIN);
}

void WeatherClockScreen::UpdateWeatherDisplay() {
    if (!root_) {
        return;
    }

    // 使用天气管理器获取天气数据
    weather_info_t weather_info;
    esp_err_t ret = WeatherManager::GetInstance().GetWeatherInfo(&weather_info);

    ESP_LOGI(TAG, "UpdateWeatherDisplay: ret=%d, temp=%s, temp_max=%s, temp_min=%s",
             ret, weather_info.temp, weather_info.temp_max, weather_info.temp_min);

    if (ret == ESP_OK) {
        // 更新温度（确保显示℃单位）
        if (temp_label_) {
            char temp_text[16];
            
            // 安全处理温度字符串（避免多字节字符问题）
            // 检查字符串中是否包含°符号（UTF-8编码为0xC2 0xB0）
            char* degree_pos = strstr(weather_info.temp, "°");
            if (degree_pos) {
                // 计算数字部分的长度
                size_t num_len = degree_pos - weather_info.temp;
                if (num_len < sizeof(temp_text) - 3) {  // 保留空间给C单位
                    // 复制数字部分
                    strncpy(temp_text, weather_info.temp, num_len);
                    temp_text[num_len] = '\0';
                    // 添加C单位（使用ASCII字符确保兼容性）
                    strcat(temp_text, "°C");
                    ESP_LOGI(TAG, "Current temperature processed: %s -> %s", weather_info.temp, temp_text);
                } else {
                    // 如果数字部分太长，直接使用温度值
                    snprintf(temp_text, sizeof(temp_text), "%s", weather_info.temp);
                    ESP_LOGI(TAG, "Current temperature too long: %s", weather_info.temp);
                }
            } else {
                // 如果温度数据中没有单位，直接添加C单位
                snprintf(temp_text, sizeof(temp_text), "%s°C", weather_info.temp);
                ESP_LOGI(TAG, "Current temperature without unit: %s -> %s", weather_info.temp, temp_text);
            }
            
            lv_label_set_text(temp_label_, temp_text);
            ESP_LOGI(TAG, "Temperature label set to: %s", temp_text);
        }

        // 更新天气描述
        if (real_weather_label_) {
            lv_label_set_text(real_weather_label_, weather_info.weather);
        }

        // 更新城市名称
        if (location_label_) {
            lv_label_set_text(location_label_, weather_info.city);
        }

        // 更新空气质量（包含背景颜色和文字颜色适配）
        if (aqi_label_ && aqi_container_) {
            lv_label_set_text(aqi_label_, weather_info.aqi_level);
            SetAQIBackgroundColor(aqi_container_, aqi_label_, weather_info.aqi_level);
        }

        // 更新最高温度
        if (temp_max_label_) {
            char temp_max_text[16];
            
            // 安全处理最高温度字符串
            char* degree_pos = strstr(weather_info.temp_max, "°");
            if (degree_pos) {
                // 计算数字部分的长度
                size_t num_len = degree_pos - weather_info.temp_max;
                if (num_len < sizeof(temp_max_text) - 3) {  // 保留空间给↑和°C
                    // 复制数字部分
                    strncpy(temp_max_text, "↑", sizeof(temp_max_text) - 1);
                    strncat(temp_max_text, weather_info.temp_max, num_len);
                    strcat(temp_max_text, "°C");
                } else {
                    // 如果数字部分太长，直接使用温度值
                    snprintf(temp_max_text, sizeof(temp_max_text), "↑%s°C", weather_info.temp_max);
                }
            } else {
                // 如果没有°符号，直接添加°C单位
                snprintf(temp_max_text, sizeof(temp_max_text), "↑%s°C", weather_info.temp_max);
            }
            lv_label_set_text(temp_max_label_, temp_max_text);
        }

        // 更新最低温度
        if (temp_min_label_) {
            char temp_min_text[16];
            
            // 安全处理最低温度字符串
            char* degree_pos = strstr(weather_info.temp_min, "°");
            if (degree_pos) {
                // 计算数字部分的长度
                size_t num_len = degree_pos - weather_info.temp_min;
                if (num_len < sizeof(temp_min_text) - 3) {  // 保留空间给↓和℃
                    // 复制数字部分
                    strncpy(temp_min_text, "↓", sizeof(temp_min_text) - 1);
                    strncat(temp_min_text, weather_info.temp_min, num_len);
                    strcat(temp_min_text, "℃");
                } else {
                    // 如果数字部分太长，直接使用温度值
                    snprintf(temp_min_text, sizeof(temp_min_text), "↓%s℃", weather_info.temp_min);
                }
            } else {
                // 如果没有°符号，直接添加℃单位
                snprintf(temp_min_text, sizeof(temp_min_text), "↓%s℃", weather_info.temp_min);
            }
            lv_label_set_text(temp_min_label_, temp_min_text);
        }

        // 更新天气图标
        if (weather_icon_) {
            UpdateWeatherIconImage(weather_info.icon_code);
        }

    } else {
        // 显示默认值
        if (real_weather_label_) {
            lv_label_set_text(real_weather_label_, "--");
        }
        if (aqi_label_ && aqi_container_) {
            lv_label_set_text(aqi_label_, "--");
            // 设置默认背景颜色（优 - 绿色）和适配的文字颜色
            lv_obj_set_style_bg_color(aqi_container_, lv_color_hex(0x00E400), LV_PART_MAIN);
            lv_obj_set_style_text_color(aqi_label_, lv_color_hex(0xFFFFFF), LV_PART_MAIN); // 绿色背景用白色文字
        }
        if (temp_max_label_) {
            lv_label_set_text(temp_max_label_, "↑--℃");
        }
        if (temp_min_label_) {
            lv_label_set_text(temp_min_label_, "↓--℃");
        }
    }
}

const char* WeatherClockScreen::GetWeekdayName(int weekday) {
    // 返回中文名称
    switch (weekday) {
        case 0: return "周日";
        case 1: return "周一";
        case 2: return "周二";
        case 3: return "周三";
        case 4: return "周四";
        case 5: return "周五";
        case 6: return "周六";
        default: return "未知";
    }
}

const char* WeatherClockScreen::GetWeatherIcon(const char* icon_code) {
    // 根据和风天气图标代码返回对应的ASCII字符图标
    if (!icon_code) return "*";
    
    // 主要天气状况
    if (strcmp(icon_code, "100") == 0 || strcmp(icon_code, "150") == 0) return "☼";  // 晴
    if (strcmp(icon_code, "101") == 0 || strcmp(icon_code, "151") == 0) return "☁";  // 多云
    if (strcmp(icon_code, "102") == 0 || strcmp(icon_code, "152") == 0) return "⛅";  // 少云
    if (strcmp(icon_code, "103") == 0 || strcmp(icon_code, "153") == 0) return "⛅";  // 晴间多云
    if (strcmp(icon_code, "104") == 0) return "☁";  // 阴
    
    // 雨雪天气
    if (strstr(icon_code, "3") != nullptr) return "☔";  // 雨相关
    if (strstr(icon_code, "4") != nullptr) return "❄";  // 雪相关
    if (strcmp(icon_code, "399") == 0) return "☔";  // 雨
    if (strcmp(icon_code, "499") == 0) return "❄";  // 雪
    
    // 雾霾沙尘
    if (strstr(icon_code, "5") != nullptr) return "≡";  // 雾霾沙尘
    
    // 极端天气
    if (strcmp(icon_code, "900") == 0) return "☀";  // 热
    if (strcmp(icon_code, "901") == 0) return "❄";  // 冷
    
    // 未知天气
    if (strcmp(icon_code, "999") == 0) return "*";  // 未知
    
    return "*";  // 默认图标
}

// 天气图标生成函数已被移除，改为使用预生成的QOI图片

void WeatherClockScreen::UpdateWeatherIconImage(const char* icon_code) {
    if (!weather_icon_ || !icon_code) return;
    
    // 使用WeatherIconManager加载预生成的QOI图片
    esp_err_t ret = WeatherIconManager::GetInstance().UpdateWeatherIcon(weather_icon_, icon_code);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update weather icon for code: %s", icon_code);
    }
}



// FlightGameScreen 实现
FlightGameScreen::FlightGameScreen(const char* name) {
    name_ = name;
    game_widget_ = nullptr;
}

void FlightGameScreen::Create() {
    // 创建根容器
    root_ = lv_obj_create(NULL);
    lv_obj_set_size(root_, LV_PCT(100), LV_PCT(100));
    lv_obj_center(root_);
    lv_obj_set_style_bg_color(root_, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_border_width(root_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(root_, 0, LV_PART_MAIN);

    // 创建游戏小部件
    game_widget_ = std::make_unique<FlightGameWidget>();
    game_widget_->Create(root_);
    game_widget_->SetPosition(0, 0);
    game_widget_->SetSize(240, 320);

    // 设置游戏按键回调
    game_widget_->SetButtonCallback([this](int button_id, bool pressed) {
        
        // Touch mode switching handled by Touch Element Library
        // No need for separate SimpleTouchManager
    });
}

void FlightGameScreen::Destroy() {
    if (game_widget_) {
        game_widget_.reset();
    }
    
    if (root_) {
        lv_obj_del(root_);
        root_ = nullptr;
    }
}

void FlightGameScreen::Show() {
    if (root_) {
        lv_obj_clear_flag(root_, LV_OBJ_FLAG_HIDDEN);
        
        // Touch handling is now managed by Touch Element Library
        // No need for SimpleTouchManager setup
    }
}

void FlightGameScreen::Hide() {
    // 屏幕将被替换，无需隐藏操作
    // 避免任何可能阻塞的LVGL操作
    
    // Touch mode handled by Touch Element Library
    // No need to reset SimpleTouchManager
}

void FlightGameScreen::HandleEvent(screen_event_t event) {
    // 处理屏幕切换事件
    // 游戏中的按键处理由FlightGameWidget直接管理
}

// ScreenFactory 实现
void ScreenFactory::CreateAllScreens() {
    auto& manager = ScreenManager::GetInstance();

    // 创建主屏幕
    manager.AddScreen(std::make_unique<MainScreen>());

    // 创建天气时钟屏幕
    manager.AddScreen(std::make_unique<WeatherClockScreen>());

    // 创建飞行游戏屏幕
    manager.AddScreen(std::make_unique<FlightGameScreen>());

    // 创建几个空屏幕作为演示
    manager.AddScreen(std::make_unique<EmptyScreen>("屏幕 4"));

    // 创建设置屏幕
    manager.AddScreen(std::make_unique<SettingsScreen>());
}

void ScreenFactory::InitializeScreens() {
    // 重置计数器
    screen_counter_ = 0;

    // 创建所有屏幕
    CreateAllScreens();
    
    // 设置屏幕切换事件回调
    auto& manager = ScreenManager::GetInstance();
    manager.SetEventCallback([](screen_event_t event, const char* screen_name) {
        const char* event_name = "Unknown";
        switch (event) {
            case SCREEN_EVENT_NEXT:
                event_name = "Next";
                break;
            case SCREEN_EVENT_PREVIOUS:
                event_name = "Previous";
                break;
            case SCREEN_EVENT_GOTO:
                event_name = "Goto";
                break;
        }
        ESP_LOGI(TAG, "Screen event: %s -> %s", event_name, screen_name);
    });
    
    ESP_LOGI(TAG, "Screens initialized");
}