#include "location_manager.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "cJSON.h"
#include <cstring>
#include <cstdio>

static const char* TAG = "LocationManager";

// 参考 factory_demo_v1，使用全局变量存储经纬度字符串
// 格式： "经度,纬度"，例如："121.47,31.23"
char west_south[20] = "";

// 存储城市名称
char city_name[32] = "北京";

// 高德地图IP定位API配置（参考 factory_demo_v1）
#define AMAP_SERVER "restapi.amap.com"
#define AMAP_PORT "443"
#define AMAP_URL "https://restapi.amap.com/v3/ip?key=ff71ce3e2aacca9ab7382572f61573e7"

LocationManager& LocationManager::GetInstance() {
    static LocationManager instance;
    return instance;
}

// HTTP 事件处理器
esp_err_t LocationManager::HttpEventHandler(esp_http_client_event_t* evt) {
    LocationManager* instance = &GetInstance();

    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
            break;

        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;

        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER: data_len=%d", evt->data_len);
            break;

        case HTTP_EVENT_ON_DATA:
            // 使用PSRAM分配内存
            instance->http_response_data_ = (char*)heap_caps_realloc(
                instance->http_response_data_,
                instance->http_response_len_ + evt->data_len + 1,
                MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (instance->http_response_data_) {
                memcpy(instance->http_response_data_ + instance->http_response_len_,
                       evt->data, evt->data_len);
                instance->http_response_len_ += evt->data_len;
                instance->http_response_data_[instance->http_response_len_] = '\0';
            }
            break;

        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH, total length: %d", instance->http_response_len_);
            break;

        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;

        default:
            break;
    }

    return ESP_OK;
}

// 解析高德API返回的位置数据（参考 factory_demo_v1 的 parse_location_data 函数）
static esp_err_t parse_location_data(const char *buffer) {
    ESP_LOGI(TAG, "Parsing location data from Amap API");

    // 2. 解析JSON（esp_http_client已经处理了HTTP头，buffer就是JSON响应体）
    cJSON *json = cJSON_Parse(buffer);
    if (NULL == json) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }

    // 3. 检查响应状态
    cJSON *status = cJSON_GetObjectItem(json, "status");
    if (status && cJSON_IsString(status) && strcmp(status->valuestring, "1") == 0) {
        ESP_LOGI(TAG, "Amap API response status: OK");

        // 4. 获取城市名称
        cJSON *city = cJSON_GetObjectItem(json, "city");
        if (city && cJSON_IsString(city)) {
            strncpy(city_name, city->valuestring, sizeof(city_name) - 1);
            city_name[sizeof(city_name) - 1] = '\0';
            ESP_LOGI(TAG, "City parsed: %s", city_name);
        }

        // 5. 获取rectangle字段（格式："longitude1,latitude1;longitude2,latitude2"）
        cJSON *rectangle = cJSON_GetObjectItem(json, "rectangle");
        if (rectangle && cJSON_IsString(rectangle)) {
            char *rectangle_str = strdup(rectangle->valuestring);
            ESP_LOGI(TAG, "Rectangle: %s", rectangle_str);

            // 6. 分割字符串，取第一个点
            char *semicolon_pos = strchr(rectangle_str, ';');
            if (semicolon_pos != NULL) {
                *semicolon_pos = '\0';
            }

            // 7. 解析经纬度
            double latitude, longitude;
            if (sscanf(rectangle_str, "%lf,%lf", &longitude, &latitude) == 2) {
                // 8. 格式化为"经度,纬度"字符串（保留2位小数）
                snprintf(west_south, sizeof(west_south), "%.2f,%.2f", longitude, latitude);
                ESP_LOGI(TAG, "Location parsed: %s (Lon: %.2f, Lat: %.2f)", west_south, longitude, latitude);

                free(rectangle_str);
                cJSON_Delete(json);
                return ESP_OK;
            } else {
                ESP_LOGE(TAG, "Failed to parse coordinates from rectangle");
                free(rectangle_str);
            }
        } else {
            ESP_LOGE(TAG, "No 'rectangle' field in Amap response");
        }
    } else {
        ESP_LOGE(TAG, "Amap API response status failed");
    }

    cJSON_Delete(json);
    return ESP_FAIL;
}

esp_err_t LocationManager::Init() {
    ESP_LOGI(TAG, "Initializing Location Manager");

    // 初始化HTTP响应缓冲区
    http_response_data_ = nullptr;
    http_response_len_ = 0;

    initialized_ = true;

    ESP_LOGI(TAG, "Location Manager initialized successfully");
    return ESP_OK;
}

// 使用esp_http_client进行HTTPS请求获取IP位置（参考 factory_demo_v1 的 app_location_request 函数）
esp_err_t LocationManager::FetchLocationByIP() {
    ESP_LOGI(TAG, "Fetching location by IP...");

    // 清空之前的响应数据
    if (http_response_data_) {
        free(http_response_data_);
        http_response_data_ = nullptr;
        http_response_len_ = 0;
    }

    // 配置HTTP客户端
    esp_http_client_config_t config = {};
    config.url = AMAP_URL;
    config.method = HTTP_METHOD_GET;
    config.event_handler = HttpEventHandler;
    config.timeout_ms = 5000;
    config.skip_cert_common_name_check = true;
    config.crt_bundle_attach = esp_crt_bundle_attach;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }

    // 执行HTTP请求
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP status code: %d", status_code);

    esp_http_client_cleanup(client);

    // 解析位置数据
    if (status_code == 200 && http_response_data_) {
        if (parse_location_data(http_response_data_) == ESP_OK) {
            ESP_LOGI(TAG, "Location successfully fetched: %s", west_south);
        } else {
            ESP_LOGE(TAG, "Failed to parse location data");
        }
    } else {
        ESP_LOGE(TAG, "Failed to get location data, status: %d", status_code);
    }

    // 清理响应数据
    if (http_response_data_) {
        free(http_response_data_);
        http_response_data_ = nullptr;
        http_response_len_ = 0;
    }

    return ESP_OK;
}

bool LocationManager::IsLocationAvailable() {
    return initialized_ && strlen(west_south) > 0;
}
