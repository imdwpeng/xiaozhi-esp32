#include "date_widget.h"
#include "esp_log.h"

static const char* TAG = "DateWidget";

DateWidget::DateWidget() {
    // 初始化默认时间数据
    current_hour_ = 0;
    current_min_ = 0;
    strcpy(current_date_, "--/--");
    strcpy(current_weekday_, "未知");
}

DateWidget::~DateWidget() {
    Destroy();
}

void DateWidget::Create(lv_obj_t* parent) {
    ESP_LOGI(TAG, "Creating date widget");
    
    // 创建容器
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, 200, 120);
    lv_obj_set_style_bg_opa(container_, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(container_, 10, LV_PART_MAIN);
    
    // 创建小时标签
    hour_label_ = lv_label_create(container_);
    lv_label_set_text(hour_label_, "00");
    lv_obj_set_style_text_font(hour_label_, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(hour_label_, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(hour_label_, LV_ALIGN_TOP_LEFT, 0, 0);
    
    // 创建冒号标签
    colon_label_ = lv_label_create(container_);
    lv_label_set_text(colon_label_, ":");
    lv_obj_set_style_text_font(colon_label_, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(colon_label_, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align_to(colon_label_, hour_label_, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    
    // 创建分钟标签
    min_label_ = lv_label_create(container_);
    lv_label_set_text(min_label_, "00");
    lv_obj_set_style_text_font(min_label_, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(min_label_, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align_to(min_label_, colon_label_, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    
    // 创建日期标签
    date_label_ = lv_label_create(container_);
    lv_label_set_text(date_label_, current_date_);
    lv_obj_set_style_text_font(date_label_, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(date_label_, lv_color_hex(0xcccccc), LV_PART_MAIN);
    lv_obj_align(date_label_, LV_ALIGN_BOTTOM_LEFT, 0, -20);
    
    // 创建星期标签
    weekday_label_ = lv_label_create(container_);
    lv_label_set_text(weekday_label_, current_weekday_);
    lv_obj_set_style_text_font(weekday_label_, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(weekday_label_, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_align(weekday_label_, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    
    ESP_LOGI(TAG, "Date widget created successfully");
}

void DateWidget::Destroy() {
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
        hour_label_ = nullptr;
        min_label_ = nullptr;
        colon_label_ = nullptr;
        date_label_ = nullptr;
        weekday_label_ = nullptr;
    }
}

void DateWidget::UpdateTime() {
    // 使用时间管理器获取时间
    struct tm timeinfo;
    esp_err_t ret = time_manager_get_time(&timeinfo);
    
    if (ret == ESP_OK) {
        current_hour_ = timeinfo.tm_hour;
        current_min_ = timeinfo.tm_min;
        
        // 格式化日期
        strftime(current_date_, sizeof(current_date_), "%m/%d", &timeinfo);
        
        // 获取星期
        strcpy(current_weekday_, GetWeekdayName(timeinfo.tm_wday));
        
        UpdateTimeDisplay();
    } else {
        ESP_LOGE(TAG, "Failed to get time from time manager");
    }
}

void DateWidget::SetPosition(int x, int y) {
    if (container_) {
        lv_obj_set_pos(container_, x, y);
    }
}

void DateWidget::UpdateTimeDisplay() {
    if (!container_) return;
    
    // 更新时间显示
    char hour_str[8];
    char min_str[8];
    snprintf(hour_str, sizeof(hour_str), "%02d", current_hour_);
    snprintf(min_str, sizeof(min_str), "%02d", current_min_);
    
    lv_label_set_text(hour_label_, hour_str);
    lv_label_set_text(min_label_, min_str);
    lv_label_set_text(date_label_, current_date_);
    lv_label_set_text(weekday_label_, current_weekday_);
}

const char* DateWidget::GetWeekdayName(int weekday) {
    switch (weekday) {
        case 0: return "星期日";
        case 1: return "星期一";
        case 2: return "星期二";
        case 3: return "星期三";
        case 4: return "星期四";
        case 5: return "星期五";
        case 6: return "星期六";
        default: return "未知";
    }
}