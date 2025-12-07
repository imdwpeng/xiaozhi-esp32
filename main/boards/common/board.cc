#include "board.h"
#include "system_info.h"
#include "settings.h"
#include "display/display.h"
#include "display/oled_display.h"
#include "assets/lang_config.h"
#include "esp32_music.h"

#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_chip_info.h>
#include <esp_random.h>

#define TAG "Board"

Board::Board() {
    music_ = nullptr;  // 先初始化为空指针
    
    Settings settings("board", true);
    uuid_ = settings.GetString("uuid");
    if (uuid_.empty()) {
        uuid_ = GenerateUuid();
        settings.SetString("uuid", uuid_);
    }
    ESP_LOGI(TAG, "UUID=%s SKU=%s", uuid_.c_str(), BOARD_NAME);
    
    // 初始化音乐播放器
    music_ = new Esp32Music();
    ESP_LOGI(TAG, "Music player initialized for all boards");
}

Board::~Board() {
    if (music_) {
        delete music_;
        music_ = nullptr;
        ESP_LOGI(TAG, "Music player destroyed");
    }
}

std::string Board::GenerateUuid() {
    // UUID v4 需要 16 字节的随机数据
    uint8_t uuid[16];
    
    // 使用 ESP32 的硬件随机数生成器
    esp_fill_random(uuid, sizeof(uuid));
    
    // 设置版本 (版本 4) 和变体位
    uuid[6] = (uuid[6] & 0x0F) | 0x40;    // 版本 4
    uuid[8] = (uuid[8] & 0x3F) | 0x80;    // 变体 1
    
    // 将字节转换为标准的 UUID 字符串格式
    char uuid_str[37];
    snprintf(uuid_str, sizeof(uuid_str),
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        uuid[0], uuid[1], uuid[2], uuid[3],
        uuid[4], uuid[5], uuid[6], uuid[7],
        uuid[8], uuid[9], uuid[10], uuid[11],
        uuid[12], uuid[13], uuid[14], uuid[15]);
    
    return std::string(uuid_str);
}

bool Board::GetBatteryLevel(int &level, bool& charging, bool& discharging) {
    return false;
}

bool Board::GetTemperature(float& esp32temp){
    return false;
}

Display* Board::GetDisplay() {
    static NoDisplay display;
    return &display;
}

Camera* Board::GetCamera() {
    return nullptr;
}

Music* Board::GetMusic() {
    return music_;
}

std::string Board::GetSystemInfoJson() {
    std::string json = "{";
    
    // Basic info
    json += "\"version\":2,";
    json += "\"language\":\"" + std::string(Lang::CODE) + "\",";
    json += "\"flash_size\":" + std::to_string(SystemInfo::GetFlashSize()) + ",";
    json += "\"minimum_free_heap_size\":" + std::to_string(SystemInfo::GetMinimumFreeHeapSize()) + ",";
    json += "\"mac_address\":\"" + SystemInfo::GetMacAddress() + "\",";
    json += "\"uuid\":\"" + uuid_ + "\",";
    json += "\"chip_model_name\":\"" + SystemInfo::GetChipModelName() + "\",";

    // Chip info
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    json += "\"chip_info\":{";
    json += "\"model\":" + std::to_string(chip_info.model) + ",";
    json += "\"cores\":" + std::to_string(chip_info.cores) + ",";
    json += "\"revision\":" + std::to_string(chip_info.revision) + ",";
    json += "\"features\":" + std::to_string(chip_info.features);
    json += "},";

    // Application info
    auto app_desc = esp_app_get_description();
    json += "\"application\":{";
    json += "\"name\":\"" + std::string(app_desc->project_name) + "\",";
    json += "\"version\":\"" + std::string(app_desc->version) + "\",";
    json += "\"compile_time\":\"" + std::string(app_desc->date) + "T" + std::string(app_desc->time) + "Z\",";
    json += "\"idf_version\":\"" + std::string(app_desc->idf_ver) + "\",";
    char sha256_str[65];
    for (int i = 0; i < 32; i++) {
        snprintf(sha256_str + i * 2, sizeof(sha256_str) - i * 2, "%02x", app_desc->app_elf_sha256[i]);
    }
    json += "\"elf_sha256\":\"" + std::string(sha256_str) + "\"";
    json += "},";

    // Partition table
    json += "\"partition_table\":[";
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
    bool first_partition = true;
    while (it) {
        const esp_partition_t *partition = esp_partition_get(it);
        if (!first_partition) {
            json += ",";
        }
        json += "{";
        json += "\"label\":\"" + std::string(partition->label) + "\",";
        json += "\"type\":" + std::to_string(partition->type) + ",";
        json += "\"subtype\":" + std::to_string(partition->subtype) + ",";
        json += "\"address\":" + std::to_string(partition->address) + ",";
        json += "\"size\":" + std::to_string(partition->size);
        json += "}";
        first_partition = false;
        it = esp_partition_next(it);
    }
    json += "],";

    // OTA info
    json += "\"ota\":{";
    auto ota_partition = esp_ota_get_running_partition();
    json += "\"label\":\"" + std::string(ota_partition->label) + "\"";
    json += "},";

    // Display info
    auto display = GetDisplay();
    if (display) {
        json += "\"display\":{";
        if (dynamic_cast<OledDisplay*>(display)) {
            json += "\"monochrome\":true,";
        } else {
            json += "\"monochrome\":false,";
        }
        json += "\"width\":" + std::to_string(display->width()) + ",";
        json += "\"height\":" + std::to_string(display->height());
        json += "},";
    }

    // Board info
    json += "\"board\":" + GetBoardJson();

    json += "}";
    return json;
}

Led* Board::GetLed() {
    static NoLed led;
    return &led;
}
