#pragma once

#include <string>
#include "esp_err.h"
#include "esp_http_client.h"

#ifdef __cplusplus
extern "C" {
#endif

// 参考 factory_demo_v1 的实现，使用全局变量存储经纬度字符串
// 格式： "经度,纬度"，例如："121.47,31.23"
extern char west_south[20];

// 存储城市名称
extern char city_name[32];

// 位置管理器类
class LocationManager {
public:
    static LocationManager& GetInstance();

    // 初始化位置管理器
    esp_err_t Init();

    // 通过IP获取位置（使用高德地图API，esp_http_client实现）
    esp_err_t FetchLocationByIP();

    // 检查位置是否可用
    bool IsLocationAvailable();

    // HTTP 事件处理器（静态成员函数）
    static esp_err_t HttpEventHandler(esp_http_client_event_t* evt);

private:
    LocationManager() = default;
    ~LocationManager() = default;
    LocationManager(const LocationManager&) = delete;
    LocationManager& operator=(const LocationManager&) = delete;

    bool initialized_;
    char* http_response_data_;  // HTTP 响应数据缓冲区
    int http_response_len_;      // HTTP 响应数据长度
};

#ifdef __cplusplus
}
#endif