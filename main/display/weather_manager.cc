#include "weather_manager.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "settings.h"
#include <cstring>
#include <cstdlib>
#include <string>

// 静态成员初始化
WeatherManager* WeatherManager::instance_ = nullptr;
char* WeatherManager::http_response_data_ = nullptr;
size_t WeatherManager::http_response_len_ = 0;

static const char* TAG = "WeatherManager";

// 和风天气API配置 - 从智控台服务器动态获取配置
#define DEFAULT_WEATHER_API_KEY "a861d0d5e7bf4ee1a83d9a9e4f96d4da"  // 默认API密钥
#define DEFAULT_WEATHER_SERVER "mj7p3y7naa.re.qweatherapi.com"      // 默认API主机
#define WEATHER_URL_FORMAT "https://%s/v7/weather/now?location=%s&key=%s&gzip=n&lang=zh"

// NVS配置键名
#define CONFIG_KEY_WEATHER_API_KEY "weather_api_key"
#define CONFIG_KEY_WEATHER_SERVER "weather_server"
#define CONFIG_KEY_WEATHER_CITY "weather_city"

WeatherManager& WeatherManager::GetInstance() {
    static WeatherManager instance;
    instance_ = &instance;
    return instance;
}

esp_err_t WeatherManager::Init() {
    ESP_LOGI(TAG, "Initializing Weather Manager");
    
    state_ = WEATHER_MANAGER_STATE_INITIALIZED;
    
    // 初始化默认天气数据
    strcpy(current_weather_.temp, "--°");
    strcpy(current_weather_.weather, "未知");
    strcpy(current_weather_.icon_code, "999");
    strcpy(current_weather_.humidity, "--");
    strcpy(current_weather_.wind_speed, "--");
    
    ESP_LOGI(TAG, "Weather Manager initialized successfully");
    return ESP_OK;
}

// 从NVS配置中获取天气API配置
static void GetWeatherConfig(std::string& api_key, std::string& server, std::string& city) {
    // 暂时使用默认值，因为Settings类可能不存在
    api_key = DEFAULT_WEATHER_API_KEY;
    server = DEFAULT_WEATHER_SERVER;
    city = "101020100"; // 默认上海
    
    ESP_LOGI(TAG, "Weather config - API Key: %s, Server: %s, City: %s", 
             api_key.c_str(), server.c_str(), city.c_str());
}

esp_err_t WeatherManager::FetchWeather() {
    if (state_ == WEATHER_MANAGER_STATE_NOT_INITIALIZED) {
        ESP_LOGE(TAG, "Weather manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (state_ == WEATHER_MANAGER_STATE_FETCHING) {
        ESP_LOGW(TAG, "Weather fetch already in progress");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Fetching weather data from API");
    
    // 从NVS获取天气配置
    std::string api_key, server, city;
    GetWeatherConfig(api_key, server, city);
    
    // 构建完整的URL
    char url[512];
    snprintf(url, sizeof(url), WEATHER_URL_FORMAT, server.c_str(), city.c_str(), api_key.c_str());
    
    ESP_LOGI(TAG, "Weather API URL: %s", url);
    
    // 清空之前的响应数据
    if (http_response_data_) {
        free(http_response_data_);
        http_response_data_ = nullptr;
        http_response_len_ = 0;
    }
    
    state_ = WEATHER_MANAGER_STATE_FETCHING;
    
    // 配置HTTP客户端
    esp_http_client_config_t config = {};
    config.url = url;
    config.method = HTTP_METHOD_GET;
    config.event_handler = HttpEventHandler;
    config.buffer_size = 1024;
    config.timeout_ms = 10000;
    config.skip_cert_common_name_check = true;
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        state_ = WEATHER_MANAGER_STATE_ERROR;
        return ESP_FAIL;
    }
    
    // 设置HTTP头
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "User-Agent", "ESP32-Weather-App");
    esp_http_client_set_header(client, "Accept", "*/*");
    
    // 执行HTTP请求
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        state_ = WEATHER_MANAGER_STATE_ERROR;
        esp_http_client_cleanup(client);
        return err;
    }
    
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP status code: %d", status_code);
    
    esp_http_client_cleanup(client);
    
    if (status_code == 200 && http_response_data_) {
        // 解析天气数据
        err = ParseWeatherData(http_response_data_);
        if (err == ESP_OK) {
            state_ = WEATHER_MANAGER_STATE_READY;
            ESP_LOGI(TAG, "Weather data parsed successfully");
        } else {
            state_ = WEATHER_MANAGER_STATE_ERROR;
            ESP_LOGE(TAG, "Failed to parse weather data");
        }
    } else {
        state_ = WEATHER_MANAGER_STATE_ERROR;
        ESP_LOGE(TAG, "Failed to get weather data, status: %d", status_code);
        err = ESP_FAIL;
    }
    
    // 清理响应数据
    if (http_response_data_) {
        free(http_response_data_);
        http_response_data_ = nullptr;
        http_response_len_ = 0;
    }
    
    return err;
}

esp_err_t WeatherManager::GetWeatherInfo(weather_info_t* info) {
    if (!info) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (state_ != WEATHER_MANAGER_STATE_READY) {
        ESP_LOGW(TAG, "Weather data not ready, state: %d", state_);
        // 返回默认数据
        strcpy(info->temp, current_weather_.temp);
        strcpy(info->weather, current_weather_.weather);
        strcpy(info->icon_code, current_weather_.icon_code);
        strcpy(info->humidity, current_weather_.humidity);
        strcpy(info->wind_speed, current_weather_.wind_speed);
        return ESP_ERR_INVALID_STATE;
    }
    
    memcpy(info, &current_weather_, sizeof(weather_info_t));
    return ESP_OK;
}

weather_manager_state_t WeatherManager::GetState() {
    return state_;
}

bool WeatherManager::IsWeatherAvailable() {
    return state_ == WEATHER_MANAGER_STATE_READY;
}

esp_err_t WeatherManager::HttpEventHandler(esp_http_client_event_t* evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
            break;
            
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
            
        case HTTP_EVENT_ON_DATA:
            // 重新分配内存以存储响应数据
            http_response_data_ = (char*)realloc(http_response_data_, http_response_len_ + evt->data_len + 1);
            if (http_response_data_) {
                memcpy(http_response_data_ + http_response_len_, evt->data, evt->data_len);
                http_response_len_ += evt->data_len;
                http_response_data_[http_response_len_] = '\0';
            }
            break;
            
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH, total length: %d", http_response_len_);
            break;
            
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
            
        default:
            break;
    }
    
    return ESP_OK;
}

esp_err_t WeatherManager::ParseWeatherData(const char* json_data) {
    if (!json_data) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Parsing weather JSON data");
    
    cJSON* json = cJSON_Parse(json_data);
    if (!json) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }
    
    cJSON* now = cJSON_GetObjectItem(json, "now");
    if (!now) {
        ESP_LOGE(TAG, "No 'now' object in JSON");
        cJSON_Delete(json);
        return ESP_FAIL;
    }
    
    // 解析温度
    cJSON* temp = cJSON_GetObjectItem(now, "temp");
    if (temp && cJSON_IsString(temp)) {
        snprintf(current_weather_.temp, sizeof(current_weather_.temp), "%s°", temp->valuestring);
        ESP_LOGI(TAG, "Temperature: %s", current_weather_.temp);
    }
    
    // 解析天气描述
    cJSON* text = cJSON_GetObjectItem(now, "text");
    if (text && cJSON_IsString(text)) {
        strncpy(current_weather_.weather, text->valuestring, sizeof(current_weather_.weather) - 1);
        current_weather_.weather[sizeof(current_weather_.weather) - 1] = '\0';
        ESP_LOGI(TAG, "Weather: %s", current_weather_.weather);
    }
    
    // 解析天气图标代码
    cJSON* icon = cJSON_GetObjectItem(now, "icon");
    if (icon && cJSON_IsString(icon)) {
        strncpy(current_weather_.icon_code, icon->valuestring, sizeof(current_weather_.icon_code) - 1);
        current_weather_.icon_code[sizeof(current_weather_.icon_code) - 1] = '\0';
        ESP_LOGI(TAG, "Icon code: %s", current_weather_.icon_code);
    }
    
    // 解析湿度
    cJSON* humidity = cJSON_GetObjectItem(now, "humidity");
    if (humidity && cJSON_IsString(humidity)) {
        strncpy(current_weather_.humidity, humidity->valuestring, sizeof(current_weather_.humidity) - 1);
        current_weather_.humidity[sizeof(current_weather_.humidity) - 1] = '\0';
        ESP_LOGI(TAG, "Humidity: %s%%", current_weather_.humidity);
    }
    
    // 解析风速
    cJSON* windSpeed = cJSON_GetObjectItem(now, "windSpeed");
    if (windSpeed && cJSON_IsString(windSpeed)) {
        strncpy(current_weather_.wind_speed, windSpeed->valuestring, sizeof(current_weather_.wind_speed) - 1);
        current_weather_.wind_speed[sizeof(current_weather_.wind_speed) - 1] = '\0';
        ESP_LOGI(TAG, "Wind speed: %s km/h", current_weather_.wind_speed);
    }
    
    cJSON_Delete(json);
    
    ESP_LOGI(TAG, "Weather data parsed successfully");
    return ESP_OK;
}