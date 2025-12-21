#include "location_manager.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include <cstring>
#include <cstdio>

static const char* TAG = "LocationManager";

LocationManager& LocationManager::GetInstance() {
    static LocationManager instance;
    return instance;
}

esp_err_t LocationManager::Init() {
    ESP_LOGI(TAG, "Initializing Location Manager");
    
    // 初始化当前位置为上海（默认）
    SetDefaultLocation();
    
    initialized_ = true;
    ESP_LOGI(TAG, "Location Manager initialized successfully");
    return ESP_OK;
}

esp_err_t LocationManager::GetCurrentLocation(location_info_t* location) {
    if (!location) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!initialized_) {
        ESP_LOGE(TAG, "Location manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 暂时返回默认位置（上海）
    // 在实际项目中，这里可以从GPS模块、WiFi定位或IP定位获取位置
    memcpy(location, &current_location_, sizeof(location_info_t));
    
    ESP_LOGI(TAG, "Current location - Lat: %.6f, Lon: %.6f, City: %s (%s)", 
             location->latitude, location->longitude, location->city_name, location->city_code);
    
    return ESP_OK;
}

esp_err_t LocationManager::GetCityCodeFromCoords(float lat, float lon, char* city_code, size_t city_code_len) {
    if (!city_code || city_code_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Getting city code from coordinates: %.6f, %.6f", lat, lon);
    
    // 使用和风天气的逆地理编码API
    char url[512];
    snprintf(url, sizeof(url), 
             "https://p35hut5dmy.re.qweatherapi.com/v7/city/lookup?location=%.6f,%.6f&key=ea17d923db1c496fb5f6919756c483cd",
             lat, lon);
    
    ESP_LOGI(TAG, "Geocoding API URL: %s", url);
    
    // 配置HTTP客户端
    esp_http_client_config_t config = {};
    config.url = url;
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = 5000;
    config.disable_auto_redirect = true;  // 禁用自动重定向
    // 使用HTTPS，配置跳过SSL验证
    config.skip_cert_common_name_check = true;
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client for geocoding");
        return ESP_FAIL;
    }
    
    // 执行请求
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }
    
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "Geocoding HTTP status: %d", status_code);
    
    if (status_code == 200) {
        // 读取响应数据
        char* response_data = (char*)malloc(1024);
        if (response_data) {
            int read_len = esp_http_client_read(client, response_data, 1023);
            if (read_len > 0) {
                response_data[read_len] = '\0';
                
                // 解析JSON响应
                cJSON* json = cJSON_Parse(response_data);
                if (json) {
                    cJSON* location_array = cJSON_GetObjectItem(json, "location");
                    if (cJSON_IsArray(location_array) && cJSON_GetArraySize(location_array) > 0) {
                        cJSON* first_location = cJSON_GetArrayItem(location_array, 0);
                        cJSON* city_code_json = cJSON_GetObjectItem(first_location, "id");
                        
                        if (cJSON_IsString(city_code_json)) {
                            strncpy(city_code, city_code_json->valuestring, city_code_len - 1);
                            city_code[city_code_len - 1] = '\0';
                            ESP_LOGI(TAG, "Found city code: %s", city_code);
                        }
                    }
                    cJSON_Delete(json);
                }
            }
            free(response_data);
        }
    }
    
    esp_http_client_cleanup(client);
    
    // 如果没有获取到城市代码，使用默认的上海代码
    if (strlen(city_code) == 0) {
        strcpy(city_code, "101020100");
        ESP_LOGW(TAG, "Using default city code (Shanghai): %s", city_code);
    }
    
    return ESP_OK;
}

void LocationManager::SetDefaultLocation() {
    // 设置上海作为默认位置
    current_location_.latitude = 31.2304;
    current_location_.longitude = 121.4737;
    strcpy(current_location_.city_code, "101020100");
    strcpy(current_location_.city_name, "上海");
    current_location_.is_valid = true;
    
    ESP_LOGI(TAG, "Default location set to Shanghai");
}

bool LocationManager::IsLocationAvailable() {
    return initialized_ && current_location_.is_valid;
}

void LocationManager::SetLocation(float lat, float lon, const char* city_code, const char* city_name) {
    current_location_.latitude = lat;
    current_location_.longitude = lon;
    
    if (city_code) {
        strncpy(current_location_.city_code, city_code, sizeof(current_location_.city_code) - 1);
        current_location_.city_code[sizeof(current_location_.city_code) - 1] = '\0';
    }
    
    if (city_name) {
        strncpy(current_location_.city_name, city_name, sizeof(current_location_.city_name) - 1);
        current_location_.city_name[sizeof(current_location_.city_name) - 1] = '\0';
    }
    
    current_location_.is_valid = true;
    
    ESP_LOGI(TAG, "Location updated - Lat: %.6f, Lon: %.6f, City: %s (%s)", 
             lat, lon, city_name ? city_name : "Unknown", city_code ? city_code : "Unknown");
}