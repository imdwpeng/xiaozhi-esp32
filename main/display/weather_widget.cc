#include "weather_widget.h"
#include "esp_log.h"

static const char* TAG = "WeatherWidget";

WeatherWidget::WeatherWidget() {
    // 初始化默认天气数据
    strcpy(current_temp_, "--°");
    strcpy(current_weather_, "未知");
}

WeatherWidget::~WeatherWidget() {
    Destroy();
}

void WeatherWidget::Create(lv_obj_t* parent) {
    ESP_LOGI(TAG, "Creating weather widget");
    
    // 创建容器
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, 200, 80);
    lv_obj_set_style_bg_opa(container_, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(container_, 10, LV_PART_MAIN);
    
    // 创建温度标签
    temp_label_ = lv_label_create(container_);
    lv_label_set_text(temp_label_, current_temp_);
    lv_obj_set_style_text_font(temp_label_, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(temp_label_, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(temp_label_, LV_ALIGN_TOP_LEFT, 0, 0);
    
    // 创建天气描述标签
    weather_label_ = lv_label_create(container_);
    lv_label_set_text(weather_label_, current_weather_);
    lv_obj_set_style_text_font(weather_label_, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(weather_label_, lv_color_hex(0xcccccc), LV_PART_MAIN);
    lv_obj_align(weather_label_, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    
    ESP_LOGI(TAG, "Weather widget created successfully");
}

void WeatherWidget::Destroy() {
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
        temp_label_ = nullptr;
        weather_label_ = nullptr;
        weather_icon_ = nullptr;
    }
}

void WeatherWidget::UpdateWeather() {
    // 使用天气管理器获取真实天气数据
    weather_info_t weather_info;
    esp_err_t ret = WeatherManager::GetInstance().GetWeatherInfo(&weather_info);
    
    if (ret == ESP_OK) {
        // 成功获取到天气数据
        strcpy(current_temp_, weather_info.temp);
        strcpy(current_weather_, weather_info.weather);
        
        ESP_LOGI(TAG, "Weather updated from API: %s %s", current_temp_, current_weather_);
    } else {
        // 获取天气数据失败，使用默认数据
        strcpy(current_temp_, "--°");
        strcpy(current_weather_, "未知");
        
        ESP_LOGW(TAG, "Failed to get weather data, using default");
        
        // 如果天气管理器未初始化，尝试初始化并获取一次天气数据
        if (WeatherManager::GetInstance().GetState() == WEATHER_MANAGER_STATE_NOT_INITIALIZED) {
            ESP_LOGI(TAG, "Initializing weather manager");
            WeatherManager::GetInstance().Init();
            WeatherManager::GetInstance().FetchWeather();
        }
    }
    
    UpdateWeatherDisplay();
}

void WeatherWidget::SetPosition(int x, int y) {
    if (container_) {
        lv_obj_set_pos(container_, x, y);
    }
}

void WeatherWidget::UpdateWeatherDisplay() {
    if (!container_) return;
    
    lv_label_set_text(temp_label_, current_temp_);
    lv_label_set_text(weather_label_, current_weather_);
}

const char* WeatherWidget::GetWeatherIcon(const char* icon_code) {
    // 根据天气图标代码返回对应的图标符号
    // 这里可以根据需要扩展为更丰富的图标映射
    if (strcmp(icon_code, "100") == 0) return "☀️";  // 晴
    if (strcmp(icon_code, "101") == 0) return "⛅";  // 多云
    if (strcmp(icon_code, "102") == 0) return "☁️";  // 阴
    if (strcmp(icon_code, "103") == 0) return "🌤️";  // 晴间多云
    if (strcmp(icon_code, "104") == 0) return "☁️";  // 阴
    
    // 雨雪天气
    if (strstr(icon_code, "3") != nullptr) return "🌧️";  // 雨
    if (strstr(icon_code, "4") != nullptr) return "❄️";  // 雪
    
    return "🌡️";  // 默认图标
}