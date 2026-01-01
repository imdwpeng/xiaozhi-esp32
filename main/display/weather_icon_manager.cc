#include "weather_icon_manager.h"
#include "mmap_generate_weather.h"
#include "lvgl_image.h"
#include <esp_log.h>
#include <string.h>
#include <esp_heap_caps.h>
#include <memory>

static const char* TAG = "WeatherIconManager";

// 使用静态变量保持 LvglRawImage 对象的生命周期
// 注意：LvglAllocatedImage会在析构时释放data，但我们的data来自mmap，不能释放
// 所以使用LvglRawImage，它不会释放data指针
static std::unique_ptr<LvglRawImage> g_weather_image;

WeatherIconManager& WeatherIconManager::GetInstance() {
    static WeatherIconManager instance;
    return instance;
}

esp_err_t WeatherIconManager::Init() {
    if (initialized_) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing weather icon manager");
    
    const mmap_assets_config_t config_weather = {
        .partition_label = "weather",
        .max_files = MMAP_WEATHER_FILES,
        .checksum = MMAP_WEATHER_CHECKSUM,
        .flags = {
            .mmap_enable = true,
            .app_bin_check = true,
        },
    };
    
    esp_err_t ret = mmap_assets_new(&config_weather, &asset_weather_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create mmap assets for weather icons: %s", esp_err_to_name(ret));
        return ret;
    }
    
    int stored_files = mmap_assets_get_stored_files(asset_weather_);
    ESP_LOGI(TAG, "Weather icons stored files: %d", stored_files);
    
    initialized_ = true;
    return ESP_OK;
}

int WeatherIconManager::FindIconIndex(const char* icon_code) {
    if (!icon_code || !initialized_) {
        return -1;
    }

    // Search for PNG format
    char icon_name[20];
    snprintf(icon_name, sizeof(icon_name), "%s.png", icon_code);

    int total_files = mmap_assets_get_stored_files(asset_weather_);
    for (int i = 0; i < total_files; i++) {
        const char* stored_name = mmap_assets_get_name(asset_weather_, i);
        if (stored_name && strcmp(stored_name, icon_name) == 0) {
            ESP_LOGI(TAG, "Found icon: %s at index %d", icon_name, i);
            return i;
        }
    }

    // 如果找不到对应的图标，使用默认的未知图标 (999.png)
    for (int i = 0; i < total_files; i++) {
        const char* stored_name = mmap_assets_get_name(asset_weather_, i);
        if (stored_name && strcmp(stored_name, "999.png") == 0) {
            ESP_LOGW(TAG, "Icon not found, using default: 999.png");
            return i;
        }
    }

    ESP_LOGW(TAG, "Icon not found: %s", icon_name);
    return -1;
}

esp_err_t WeatherIconManager::UpdateWeatherIcon(lv_obj_t* icon_obj, const char* icon_code) {
    if (!icon_obj || !icon_code || !initialized_) {
        ESP_LOGE(TAG, "Invalid parameters or manager not initialized");
        return ESP_ERR_INVALID_ARG;
    }

    int icon_index = FindIconIndex(icon_code);
    if (icon_index < 0) {
        ESP_LOGE(TAG, "Failed to find icon index for code: %s", icon_code);
        return ESP_ERR_NOT_FOUND;
    }

    // Get image data from mmap_assets
    int data_size = mmap_assets_get_size(asset_weather_, icon_index);
    const uint8_t* data = mmap_assets_get_mem(asset_weather_, icon_index);

    if (!data || data_size == 0) {
        ESP_LOGE(TAG, "Invalid image data for icon: %s", icon_code);
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_LOGI(TAG, "Setting weather icon with code: %s, index: %d, size: %d, first bytes: %02x %02x %02x %02x",
              icon_code, icon_index, data_size, data[0], data[1], data[2], data[3]);

    // Check if data starts with PNG signature (89 50 4E 47 = .PNG)
    if (data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G') {
        ESP_LOGI(TAG, "PNG file detected");
    } else {
        ESP_LOGW(TAG, "Unknown file format, first bytes: %02x %02x %02x %02x", data[0], data[1], data[2], data[3]);
        return ESP_ERR_INVALID_SIZE;
    }

    // 使用 LvglRawImage 处理 PNG 图像，它不会释放 data 指针
    // LvglRawImage 设置 LV_COLOR_FORMAT_RAW_ALPHA，让解码器自动识别 PNG 格式
    g_weather_image = std::make_unique<LvglRawImage>((void*)data, data_size);
    ESP_LOGI(TAG, "Created LvglRawImage successfully: w=%d, h=%d, cf=%d",
              g_weather_image->image_dsc()->header.w,
              g_weather_image->image_dsc()->header.h,
              g_weather_image->image_dsc()->header.cf);

    ESP_LOGI(TAG, "Setting PNG image source...");
    lv_image_set_src(icon_obj, g_weather_image->image_dsc());
    ESP_LOGI(TAG, "Image source set successfully");

    // 刷新显示
    lv_obj_invalidate(icon_obj);
    lv_obj_t* screen = lv_obj_get_screen(icon_obj);
    if (screen) {
        lv_obj_invalidate(screen);
    }

    return ESP_OK;
}
