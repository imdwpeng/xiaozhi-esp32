#pragma once

#include <string>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// 位置信息结构体
typedef struct {
    float latitude;        // 纬度
    float longitude;       // 经度
    char city_code[32];    // 城市代码
    char city_name[64];    // 城市名称
    bool is_valid;         // 位置是否有效
} location_info_t;

// 位置管理器类
class LocationManager {
public:
    static LocationManager& GetInstance();
    
    // 初始化位置管理器
    esp_err_t Init();
    
    // 获取当前位置信息
    esp_err_t GetCurrentLocation(location_info_t* location);
    
    // 根据经纬度获取城市代码
    esp_err_t GetCityCodeFromCoords(float lat, float lon, char* city_code, size_t city_code_len);
    
    // 设置默认位置（上海）
    void SetDefaultLocation();
    
    // 检查位置是否可用
    bool IsLocationAvailable();
    
    // 手动设置位置
    void SetLocation(float lat, float lon, const char* city_code, const char* city_name);

private:
    LocationManager() = default;
    ~LocationManager() = default;
    LocationManager(const LocationManager&) = delete;
    LocationManager& operator=(const LocationManager&) = delete;
    
    location_info_t current_location_;
    bool initialized_;
};

#ifdef __cplusplus
}
#endif