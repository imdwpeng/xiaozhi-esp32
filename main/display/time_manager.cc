#include "time_manager.h"
#include <esp_log.h>
#include <esp_sntp.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

static const char* TAG = "TimeManager";

// SNTP服务器列表
static const char* sntp_servers[] = {
    "cn.pool.ntp.org",
    "ntp1.aliyun.com", 
    "ntp.ntsc.ac.cn"
};

// 时区设置
static char current_timezone[32] = "CST-8";

// 时间管理器状态
static time_manager_state_t current_state = TIME_MANAGER_STATE_NOT_INITIALIZED;
static bool time_synced = false;

// 时间同步回调函数
static void time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI(TAG, "Time synchronized successfully");
    time_synced = true;
    current_state = TIME_MANAGER_STATE_SYNCED;
    
    // 打印同步后的时间
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time is: %s", strftime_buf);
}

// SNTP初始化函数
static void initialize_sntp(void) {
    ESP_LOGI(TAG, "Initializing SNTP");
    
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, sntp_servers[0]);
    esp_sntp_setservername(1, sntp_servers[1]);
    esp_sntp_setservername(2, sntp_servers[2]);
    
    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
    
    esp_sntp_init();
    current_state = TIME_MANAGER_STATE_SYNCING;
    ESP_LOGI(TAG, "SNTP initialized, waiting for time sync...");
}

esp_err_t time_manager_init(void) {
    if (current_state != TIME_MANAGER_STATE_NOT_INITIALIZED) {
        ESP_LOGW(TAG, "Time manager already initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Initializing time manager");
    
    // 设置时区
    setenv("TZ", current_timezone, 1);
    tzset();
    
    current_state = TIME_MANAGER_STATE_INITIALIZING;
    
    // 初始化SNTP
    initialize_sntp();
    
    ESP_LOGI(TAG, "Time manager initialized");
    return ESP_OK;
}

esp_err_t time_manager_deinit(void) {
    if (current_state == TIME_MANAGER_STATE_NOT_INITIALIZED) {
        ESP_LOGW(TAG, "Time manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Deinitializing time manager");
    
    // 停止SNTP
    esp_sntp_stop();
    
    // 重置状态
    time_synced = false;
    current_state = TIME_MANAGER_STATE_NOT_INITIALIZED;
    
    ESP_LOGI(TAG, "Time manager deinitialized");
    return ESP_OK;
}

esp_err_t time_manager_start_sync(void) {
    if (current_state == TIME_MANAGER_STATE_NOT_INITIALIZED) {
        ESP_LOGE(TAG, "Time manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Starting time synchronization");
    
    // 重新初始化SNTP
    esp_sntp_stop();
    initialize_sntp();
    
    return ESP_OK;
}

esp_err_t time_manager_stop_sync(void) {
    if (current_state == TIME_MANAGER_STATE_NOT_INITIALIZED) {
        ESP_LOGE(TAG, "Time manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Stopping time synchronization");
    
    esp_sntp_stop();
    current_state = TIME_MANAGER_STATE_INITIALIZING;
    time_synced = false;
    
    return ESP_OK;
}

esp_err_t time_manager_get_time(struct tm* time_struct) {
    if (!time_struct) {
        return ESP_ERR_INVALID_ARG;
    }
    
    time_t now;
    time(&now);
    localtime_r(&now, time_struct);
    
    return ESP_OK;
}

esp_err_t time_manager_get_timestamp(time_t* timestamp) {
    if (!timestamp) {
        return ESP_ERR_INVALID_ARG;
    }
    
    time(timestamp);
    return ESP_OK;
}

bool time_manager_is_synced(void) {
    return time_synced;
}

time_manager_state_t time_manager_get_state(void) {
    return current_state;
}

esp_err_t time_manager_set_timezone(const char* timezone) {
    if (!timezone) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (strlen(timezone) >= sizeof(current_timezone)) {
        return ESP_ERR_INVALID_ARG;
    }
    
    strcpy(current_timezone, timezone);
    setenv("TZ", current_timezone, 1);
    tzset();
    
    ESP_LOGI(TAG, "Timezone set to: %s", current_timezone);
    return ESP_OK;
}