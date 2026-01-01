#include "screens.h"
#include "screen_manager.h"
#include "time_manager.h"
#include "flight_game_widget.h"
#include "location_manager.h"
#include "weather_icon_manager.h"
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
    
    // 创建日期显示（居中）- 参考esp_sparkbot的布局
    date_label_ = lv_label_create(main_panel_);
    lv_obj_set_width(date_label_, LV_SIZE_CONTENT);
    lv_obj_set_height(date_label_, LV_SIZE_CONTENT);
    lv_obj_set_x(date_label_, 25);
    lv_obj_set_y(date_label_, 15);
    lv_obj_set_align(date_label_, LV_ALIGN_CENTER);
    lv_label_set_text(date_label_, "02/20");
    lv_obj_set_style_text_color(date_label_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_opa(date_label_, 255, LV_PART_MAIN);
    lv_obj_set_style_text_font(date_label_, &lv_font_montserrat_18, LV_PART_MAIN);
    
    // 创建星期显示（日期右侧）- 参考esp_sparkbot的布局
    weekday_label_ = lv_label_create(main_panel_);
    lv_obj_set_width(weekday_label_, LV_SIZE_CONTENT);
    lv_obj_set_height(weekday_label_, LV_SIZE_CONTENT);
    lv_obj_set_x(weekday_label_, 85);
    lv_obj_set_y(weekday_label_, 15);
    lv_obj_set_align(weekday_label_, LV_ALIGN_CENTER);
    lv_label_set_text(weekday_label_, "周日");
    lv_obj_set_style_text_color(weekday_label_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_opa(weekday_label_, 255, LV_PART_MAIN);
    lv_obj_set_style_text_font(weekday_label_, &lv_font_montserrat_20, LV_PART_MAIN);
    
    // 创建温度显示（顶部）- 参考esp_sparkbot的大字体设计
    temp_label_ = lv_label_create(main_panel_);
    lv_obj_set_width(temp_label_, LV_SIZE_CONTENT);
    lv_obj_set_height(temp_label_, LV_SIZE_CONTENT);
    lv_obj_set_x(temp_label_, 60);
    lv_obj_set_y(temp_label_, 30);
    lv_obj_set_align(temp_label_, LV_ALIGN_TOP_MID);
    lv_label_set_text(temp_label_, "15°");
    lv_obj_set_style_text_color(temp_label_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_opa(temp_label_, 255, LV_PART_MAIN);
    lv_obj_set_style_text_font(temp_label_, &lv_font_montserrat_32, LV_PART_MAIN);
    
    // 创建天气描述（左侧）- 参考esp_sparkbot的金色设计
    weather_label_ = lv_label_create(main_panel_);
    lv_obj_set_width(weather_label_, LV_SIZE_CONTENT);
    lv_obj_set_height(weather_label_, LV_SIZE_CONTENT);
    lv_obj_set_x(weather_label_, -60);
    lv_obj_set_y(weather_label_, 15);
    lv_obj_set_align(weather_label_, LV_ALIGN_CENTER);
    lv_label_set_text(weather_label_, "多云");
    lv_obj_set_style_text_color(weather_label_, lv_color_hex(0xF1BA3B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(weather_label_, 255, LV_PART_MAIN);
    lv_obj_set_style_text_font(weather_label_, &lv_font_montserrat_20, LV_PART_MAIN);
    
    // 创建天气图标（左侧）- 使用 lv_image_create (LVGL 9.x API)
    weather_icon_ = lv_image_create(main_panel_);
    lv_obj_set_width(weather_icon_, 110);
    lv_obj_set_height(weather_icon_, 110);
    lv_obj_set_x(weather_icon_, -60);
    lv_obj_set_y(weather_icon_, -54);
    lv_obj_set_align(weather_icon_, LV_ALIGN_CENTER);
    
    // 创建位置信息（顶部居中）- 参考esp_sparkbot的布局
    location_label_ = lv_label_create(main_panel_);
    lv_obj_set_width(location_label_, LV_SIZE_CONTENT);
    lv_obj_set_height(location_label_, LV_SIZE_CONTENT);
    lv_obj_set_align(location_label_, LV_ALIGN_TOP_MID);
    lv_obj_set_y(location_label_, 10);
    lv_label_set_text(location_label_, "杭州");
    lv_obj_set_style_text_color(location_label_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_opa(location_label_, 255, LV_PART_MAIN);
    lv_obj_set_style_text_font(location_label_, &lv_font_montserrat_20, LV_PART_MAIN);
    
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
        weather_label_ = nullptr;
        weather_icon_ = nullptr;
        location_label_ = nullptr;
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
        
        // 初始化天气图标管理器
        esp_err_t ret = WeatherIconManager::GetInstance().Init();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize weather icon manager: %s", esp_err_to_name(ret));
        }
        
        // 立即更新时间显示
        UpdateTimeDisplay();
        
        // 立即更新天气显示（使用默认数据）
        UpdateWeatherDisplay();
        
        // 注册天气数据更新回调，当天气数据获取成功时立即更新UI
        WeatherManager::GetInstance().SetUpdateCallback([this]() {
            ESP_LOGI(TAG, "Weather data update callback triggered");
            UpdateWeatherDisplay();
        });
        
        // 延迟获取天气数据（等待屏幕显示后再获取）
        lv_timer_create([](lv_timer_t* timer) {
            WeatherManager::GetInstance().FetchWeather();
            lv_timer_del(timer);
        }, 2000, nullptr);
        
        // 启动时间更新定时器（每秒更新）
        time_timer_ = lv_timer_create([](lv_timer_t* timer) {
            void* user_data = lv_timer_get_user_data(timer);
            WeatherClockScreen* screen = static_cast<WeatherClockScreen*>(user_data);
            screen->UpdateTimeDisplay();
        }, 1000, this);
        
        // 启动天气更新定时器（每30秒更新一次UI，每30分钟更新数据）
        ESP_LOGI(TAG, "Creating weather timer with 30s interval");
        weather_timer_ = lv_timer_create([](lv_timer_t* timer) {
            ESP_LOGI(TAG, "Weather timer callback triggered");
            void* user_data = lv_timer_get_user_data(timer);
            WeatherClockScreen* screen = static_cast<WeatherClockScreen*>(user_data);
            screen->UpdateWeatherDisplay();
        }, 30000, this);  // 每30秒更新一次UI显示
        
        // 启动天气数据获取定时器（每30分钟获取一次）
        lv_timer_create([](lv_timer_t* timer) {
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
        char weekday_str[16];
        
        snprintf(hour_str, sizeof(hour_str), "%02d", timeinfo.tm_hour);
        snprintf(min_str, sizeof(min_str), "%02d", timeinfo.tm_min);
        strftime(date_str, sizeof(date_str), "%m/%d", &timeinfo);
        const char* weekday_name = GetWeekdayName(timeinfo.tm_wday);
        
        ESP_LOGI(TAG, "Updating time display: %s:%s %s %s", hour_str, min_str, date_str, weekday_name);
        
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
    } else {
        ESP_LOGW(TAG, "Failed to get time from time manager: %d", ret);
    }
}

void WeatherClockScreen::UpdateWeatherDisplay() {
    ESP_LOGI(TAG, "UpdateWeatherDisplay called");
    if (!root_) {
        ESP_LOGW(TAG, "root_ is null, skipping update");
        return;
    }

    // 使用天气管理器获取天气数据
    weather_info_t weather_info;
    esp_err_t ret = WeatherManager::GetInstance().GetWeatherInfo(&weather_info);

    ESP_LOGI(TAG, "GetWeatherInfo returned: %d, temp: %s, weather: %s, icon: %s",
              ret, weather_info.temp, weather_info.weather, weather_info.icon_code);

    if (ret == ESP_OK) {
        // 更新温度
        if (temp_label_) {
            ESP_LOGI(TAG, "Updating temp_label to: %s", weather_info.temp);
            lv_label_set_text(temp_label_, weather_info.temp);
        } else {
            ESP_LOGW(TAG, "temp_label_ is null!");
        }

        // 更新天气描述
        if (weather_label_) {
            ESP_LOGI(TAG, "Updating weather_label to: %s", weather_info.weather);
            lv_label_set_text(weather_label_, weather_info.weather);
        } else {
            ESP_LOGW(TAG, "weather_label_ is null!");
        }

        // 更新位置信息（使用当前位置）
        location_info_t location_info;
        if (LocationManager::GetInstance().GetCurrentLocation(&location_info) == ESP_OK) {
            ESP_LOGI(TAG, "Setting location label to: %s", location_info.city_name);
            if (location_label_) {
                lv_label_set_text(location_label_, location_info.city_name);
            } else {
                ESP_LOGW(TAG, "location_label_ is null!");
            }
        } else {
            ESP_LOGW(TAG, "Failed to get current location");
        }

        // 更新天气图标图像
        ESP_LOGI(TAG, "Setting weather icon with code: %s, weather_icon_ ptr: %p", weather_info.icon_code, weather_icon_);

        if (weather_icon_) {
            ESP_LOGI(TAG, "Calling UpdateWeatherIconImage with code: %s", weather_info.icon_code);
            UpdateWeatherIconImage(weather_info.icon_code);
        } else {
            ESP_LOGW(TAG, "weather_icon_ is null!");
        }
        
    } else {
        // 使用默认数据
        lv_label_set_text(temp_label_, "--°");
        lv_label_set_text(weather_label_, "未知");
        lv_label_set_text(location_label_, "杭州");
        
        // 设置默认天气图标
        if (weather_icon_) {
            UpdateWeatherIconImage("999");
        }
    }
}

const char* WeatherClockScreen::GetWeekdayName(int weekday) {
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
    ESP_LOGI(TAG, "Creating FlightGameScreen");
    
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
        // 根据游戏状态和按键模式进行相应的处理
        ESP_LOGI(TAG, "Game button callback: button_id=%d, pressed=%d", button_id, pressed);
        
        // Touch mode switching handled by Touch Element Library
        // No need for separate SimpleTouchManager
    });
    
    ESP_LOGI(TAG, "FlightGameScreen created successfully");
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
    ESP_LOGD(TAG, "FlightGameScreen handling event: %d", event);
    
    // 处理屏幕切换事件
    // 游戏中的按键处理由FlightGameWidget直接管理
}

// ScreenFactory 实现
void ScreenFactory::CreateAllScreens() {
    ESP_LOGI(TAG, "Creating all screens");
    
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
    
    ESP_LOGI(TAG, "Created %zu screens", manager.GetScreenCount());
}

void ScreenFactory::InitializeScreens() {
    ESP_LOGI(TAG, "Initializing screens");
    
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