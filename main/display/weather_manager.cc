#include "weather_manager.h"
#include "location_manager.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_netif.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "settings.h"
#include <cstring>
#include <cstdlib>
#include <string>

// 添加zlib支持用于gzip解压缩
#include "zlib.h"

// 静态成员初始化
WeatherManager* WeatherManager::instance_ = nullptr;
char* WeatherManager::http_response_data_ = nullptr;
size_t WeatherManager::http_response_len_ = 0;

static const char* TAG = "WeatherManager";

// 和风天气API配置 - 使用指定的API服务器和HTTPS
#define DEFAULT_WEATHER_API_KEY "ea17d923db1c496fb5f6919756c483cd"  // 新的API密钥
#define DEFAULT_WEATHER_SERVER "p35hut5dmy.re.qweatherapi.com"     // 指定的API服务器
#define WEATHER_URL_FORMAT "https://%s/v7/weather/now?location=%s&key=%s&gzip=n&lang=zh"

// 工厂示例配置
#define USER_AGENT "esp32_S3_86_box"
#define MAX_HTTP_RECV_BUFFER 1024

// NVS配置键名
#define CONFIG_KEY_WEATHER_API_KEY "weather_api_key"
#define CONFIG_KEY_WEATHER_SERVER "weather_server"
#define CONFIG_KEY_WEATHER_CITY "weather_city"

// gzip解压缩函数（参考factory_demo实现）
static int network_gzip_decompress(void *in_buf, size_t in_size, void *out_buf, size_t *out_size, size_t out_buf_size)
{
    int err = 0;
    z_stream d_stream = {0}; /* decompression stream */
    d_stream.zalloc = NULL;
    d_stream.zfree = NULL;
    d_stream.opaque = NULL;
    d_stream.next_in  = (Bytef*)in_buf;
    d_stream.avail_in = 0;
    d_stream.next_out = (Bytef*)out_buf;

    if ((err = inflateInit2(&d_stream, 47)) != Z_OK) {
        return err;
    }
    while (d_stream.total_out < out_buf_size - 1 && d_stream.total_in < in_size) {
        d_stream.avail_in = d_stream.avail_out = 1;
        if ((err = inflate(&d_stream, Z_NO_FLUSH)) == Z_STREAM_END) {
            break;
        }
        if (err != Z_OK) {
            return err;
        }
    }

    if ((err = inflateEnd(&d_stream)) != Z_OK) {
        return err;
    }

    *out_size = d_stream.total_out;
    ((char *)out_buf)[*out_size] = '\0';

    return Z_OK;
}

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

// 从位置管理器获取实时位置和天气API配置
static void GetWeatherConfig(std::string& api_key, std::string& server, std::string& city) {
    // 使用新的API配置
    api_key = DEFAULT_WEATHER_API_KEY;
    server = DEFAULT_WEATHER_SERVER;
    
    // 从位置管理器获取当前城市代码
    LocationManager& location_mgr = LocationManager::GetInstance();
    location_info_t location;
    
    if (location_mgr.GetCurrentLocation(&location) == ESP_OK && location.is_valid) {
        city = location.city_code;
        ESP_LOGI(TAG, "Using current location - City: %s (%s), Lat: %.6f, Lon: %.6f", 
                 location.city_name, location.city_code, location.latitude, location.longitude);
    } else {
        city = "101020100"; // 默认上海
        ESP_LOGW(TAG, "Using default location (Shanghai): %s", city.c_str());
    }
    
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
    
    // 检查网络连接
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif == NULL) {
        ESP_LOGW(TAG, "WiFi netif not available, skipping weather fetch");
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_netif_ip_info_t ip_info;
    esp_err_t ret = esp_netif_get_ip_info(netif, &ip_info);
    if (ret != ESP_OK || ip_info.ip.addr == 0) {
        ESP_LOGW(TAG, "WiFi not connected, skipping weather fetch");
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
    
    // 配置HTTP客户端（复刻工厂示例 + 完全跳过SSL验证）
    esp_http_client_config_t config = {};
    config.url = url;
    config.method = HTTP_METHOD_GET;
    config.event_handler = HttpEventHandler;
    config.buffer_size = MAX_HTTP_RECV_BUFFER;
    config.timeout_ms = 5000;  // 工厂示例使用5秒
    config.disable_auto_redirect = true;  // 禁用自动重定向
    // 使用HTTPS，配置跳过SSL验证
    config.skip_cert_common_name_check = true;
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        state_ = WEATHER_MANAGER_STATE_ERROR;
        return ESP_FAIL;
    }
    
    // 设置HTTP头（完全复刻工厂示例）
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Host", DEFAULT_WEATHER_SERVER);
    esp_http_client_set_header(client, "User-Agent", "esp32_S3_86_box");
    esp_http_client_set_header(client, "Accept-Encoding", "identity");
    esp_http_client_set_header(client, "Cache-Control", "no-cache");
    esp_http_client_set_header(client, "Accept", "*/*");
    // 明确要求不压缩
    esp_http_client_set_header(client, "Accept-Encoding", "identity");
    
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
            
            // 强制更新UI显示（通过日志触发UI检查）
            ESP_LOGI(TAG, "Weather data updated - temp: %s, weather: %s, icon: %s", 
                     current_weather_.temp, current_weather_.weather, current_weather_.icon_code);
            
            // 触发UI更新回调
            if (update_callback_) {
                ESP_LOGI(TAG, "Triggering weather update callback");
                update_callback_();
            }
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
        // 如果数据已经获取但状态不对，仍然返回数据
        if (state_ == WEATHER_MANAGER_STATE_ERROR || state_ == WEATHER_MANAGER_STATE_FETCHING || state_ == WEATHER_MANAGER_STATE_INITIALIZED) {
            // 返回当前存储的数据
            memcpy(info, &current_weather_, sizeof(weather_info_t));
            return ESP_OK;
        }
        // 对于未初始化状态，返回默认数据
        strcpy(info->temp, "--°");
        strcpy(info->weather, "未知");
        strcpy(info->icon_code, "999");
        strcpy(info->humidity, "--%");
        strcpy(info->wind_speed, "-- km/h");
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

void WeatherManager::SetUpdateCallback(weather_update_callback_t callback) {
    update_callback_ = callback;
    ESP_LOGI(TAG, "Weather update callback set");
}

esp_err_t WeatherManager::HttpEventHandler(esp_http_client_event_t* evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
            break;
            
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
            
        case HTTP_EVENT_ON_HEADER:
            if (evt->data && evt->user_data) {
                ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER: Key=%.*s, Value=%.*s", 
                         evt->data_len, (char*)evt->data, 
                         evt->data_len, (char*)evt->user_data);
            } else {
                ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER: data_len=%d", evt->data_len);
            }
            break;
            
        case HTTP_EVENT_ON_DATA:
            // 使用PSRAM分配内存（参考工厂示例）
            http_response_data_ = (char*)heap_caps_realloc(http_response_data_, 
                                                           http_response_len_ + evt->data_len + 1, 
                                                           MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (http_response_data_) {
                memcpy(http_response_data_ + http_response_len_, evt->data, evt->data_len);
                http_response_len_ += evt->data_len;
                http_response_data_[http_response_len_] = '\0';
            }
            break;
            
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH, total length: %d", http_response_len_);
            
            // 检查是否是gzip压缩数据（gzip文件头为0x1f8b）
            if (http_response_data_ && http_response_len_ >= 2 && 
                (unsigned char)http_response_data_[0] == 0x1f && 
                (unsigned char)http_response_data_[1] == 0x8b) {
                ESP_LOGI(TAG, "Detected gzip compressed data, decompressing...");
                
                size_t decode_maxlen = http_response_len_ * 2;
                char *decode_out = (char*)heap_caps_malloc(decode_maxlen, MALLOC_CAP_SPIRAM);
                if (decode_out) {
                    size_t out_size = 0;
                    int decompress_result = network_gzip_decompress(http_response_data_, http_response_len_, 
                                                                   decode_out, &out_size, decode_maxlen);
                    
                    if (decompress_result == Z_OK && out_size > 0) {
                        ESP_LOGI(TAG, "Gzip decompression successful, decompressed size: %d", out_size);
                        
                        // 替换原始数据为解压缩后的数据
                        free(http_response_data_);
                        http_response_data_ = decode_out;
                        http_response_len_ = out_size;
                    } else {
                        ESP_LOGE(TAG, "Gzip decompression failed: %d", decompress_result);
                        heap_caps_free(decode_out);
                    }
                } else {
                    ESP_LOGE(TAG, "Failed to allocate memory for decompression");
                }
            } else {
                ESP_LOGI(TAG, "Data is not compressed, using as-is");
            }
            
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
    ESP_LOGI(TAG, "JSON data length: %d", strlen(json_data));
    
    // 打印前100个字符用于调试
    char debug_buf[101];
    strncpy(debug_buf, json_data, 100);
    debug_buf[100] = '\0';
    ESP_LOGI(TAG, "First 100 chars: %s", debug_buf);
    
    // 检查是否是有效的JSON开头
    if (strlen(json_data) == 0) {
        ESP_LOGE(TAG, "Empty JSON data");
        return ESP_FAIL;
    }
    
    // 检查是否是gzip压缩数据（gzip文件头为0x1f8b）
    if (strlen(json_data) >= 2 && (unsigned char)json_data[0] == 0x1f && (unsigned char)json_data[1] == 0x8b) {
        ESP_LOGE(TAG, "Data appears to be gzip compressed despite gzip=n parameter");
        return ESP_FAIL;
    }
    
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