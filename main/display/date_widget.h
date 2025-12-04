#ifndef DATE_WIDGET_H
#define DATE_WIDGET_H

#include "time_manager.h"
#include <lvgl.h>
#include <cstring>
#include <ctime>

// 声明字体
LV_FONT_DECLARE(lv_font_montserrat_24);
LV_FONT_DECLARE(lv_font_montserrat_16);
LV_FONT_DECLARE(lv_font_montserrat_14);

class DateWidget {
public:
    DateWidget();
    ~DateWidget();
    
    // 创建日期小部件
    void Create(lv_obj_t* parent);
    
    // 销毁日期小部件
    void Destroy();
    
    // 更新时间
    void UpdateTime();
    
    // 设置位置
    void SetPosition(int x, int y);
    
    // 获取小部件根对象
    lv_obj_t* GetRoot() { return container_; }
    
    // 获取时间信息
    int GetHour() { return current_hour_; }
    int GetMinute() { return current_min_; }
    const char* GetDate() { return current_date_; }
    const char* GetWeekday() { return current_weekday_; }
    
private:
    lv_obj_t* container_ = nullptr;
    lv_obj_t* hour_label_ = nullptr;
    lv_obj_t* min_label_ = nullptr;
    lv_obj_t* colon_label_ = nullptr;
    lv_obj_t* date_label_ = nullptr;
    lv_obj_t* weekday_label_ = nullptr;
    
    int current_hour_ = 0;
    int current_min_ = 0;
    char current_date_[16];
    char current_weekday_[16];
    
    // 更新时间显示
    void UpdateTimeDisplay();
    
    // 获取星期名称
    const char* GetWeekdayName(int weekday);
};

#endif // DATE_WIDGET_H