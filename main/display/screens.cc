#include "screens.h"
#include "screen_manager.h"
#include "time_manager.h"
#include <esp_log.h>
#include <cstring>
#include <ctime>
#include <cstdio>
#include <lvgl.h>

static const char* TAG = "Screens";

int ScreenFactory::screen_counter_ = 0;

// MainScreen 实现
MainScreen::MainScreen(const char* name) {
    name_ = name;
}

void MainScreen::Create() {
    ESP_LOGI(TAG, "Creating MainScreen");
    
    // 创建根容器
    root_ = lv_obj_create(lv_scr_act());
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
    if (root_) {
        lv_obj_clear_flag(root_, LV_OBJ_FLAG_HIDDEN);
    }
}

void MainScreen::Hide() {
    if (root_) {
        lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
    }
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
    root_ = lv_obj_create(lv_scr_act());
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
    if (root_) {
        lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
    }
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
    root_ = lv_obj_create(lv_scr_act());
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
    if (root_) {
        lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
    }
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
    
    // 创建根容器
    root_ = lv_obj_create(lv_scr_act());
    lv_obj_set_size(root_, LV_PCT(100), LV_PCT(100));
    lv_obj_center(root_);
    lv_obj_set_style_bg_color(root_, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_border_width(root_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(root_, 0, LV_PART_MAIN);
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_SCROLLABLE);
    
    // 使用独立的插件创建日期和天气显示
    // 日期小部件放在屏幕中央偏下位置
    date_widget_.Create(root_);
    date_widget_.SetPosition(0, 0);  // 居中显示
    
    // 天气小部件放在屏幕左上角
    weather_widget_.Create(root_);
    weather_widget_.SetPosition(20, 20);
    
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
    
    // 销毁插件
    date_widget_.Destroy();
    weather_widget_.Destroy();
    
    if (root_) {
        lv_obj_del(root_);
        root_ = nullptr;
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
        
        // 立即获取一次天气数据
        WeatherManager::GetInstance().FetchWeather();
        
        // 立即更新插件数据
        date_widget_.UpdateTime();
        weather_widget_.UpdateWeather();
        
        // 启动时间更新定时器（每秒更新）
        time_timer_ = lv_timer_create([](lv_timer_t* timer) {
            void* user_data = lv_timer_get_user_data(timer);
            WeatherClockScreen* screen = static_cast<WeatherClockScreen*>(user_data);
            screen->date_widget_.UpdateTime();
        }, 1000, this);
        
        // 启动天气更新定时器（每30分钟更新）
        weather_timer_ = lv_timer_create([](lv_timer_t* timer) {
            void* user_data = lv_timer_get_user_data(timer);
            WeatherClockScreen* screen = static_cast<WeatherClockScreen*>(user_data);
            screen->weather_widget_.UpdateWeather();
            
            // 每30分钟重新获取一次天气数据
            WeatherManager::GetInstance().FetchWeather();
        }, 1800000, this);
    }
}

void WeatherClockScreen::Hide() {
    if (root_) {
        lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
        
        // 停止定时器
        if (time_timer_) {
            lv_timer_del(time_timer_);
            time_timer_ = nullptr;
        }
        
        if (weather_timer_) {
            lv_timer_del(weather_timer_);
            weather_timer_ = nullptr;
        }
    }
}

void WeatherClockScreen::HandleEvent(screen_event_t event) {
    ESP_LOGD(TAG, "WeatherClockScreen handling event: %d", event);
}



// ScreenFactory 实现
void ScreenFactory::CreateAllScreens() {
    ESP_LOGI(TAG, "Creating all screens");
    
    auto& manager = ScreenManager::GetInstance();
    
    // 创建主屏幕
    manager.AddScreen(std::make_unique<MainScreen>());
    
    // 创建天气时钟屏幕
    manager.AddScreen(std::make_unique<WeatherClockScreen>());
    
    // 创建几个空屏幕作为演示
    manager.AddScreen(std::make_unique<EmptyScreen>("屏幕 3"));
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