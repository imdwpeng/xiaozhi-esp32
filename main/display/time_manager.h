#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <esp_err.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// 时间管理器状态
typedef enum {
    TIME_MANAGER_STATE_NOT_INITIALIZED = 0,
    TIME_MANAGER_STATE_INITIALIZING,
    TIME_MANAGER_STATE_SYNCING,
    TIME_MANAGER_STATE_SYNCED,
    TIME_MANAGER_STATE_ERROR
} time_manager_state_t;

/**
 * @brief 初始化时间管理器
 * 
 * @return esp_err_t 初始化结果
 */
esp_err_t time_manager_init(void);

/**
 * @brief 反初始化时间管理器
 * 
 * @return esp_err_t 反初始化结果
 */
esp_err_t time_manager_deinit(void);

/**
 * @brief 启动时间同步
 * 
 * @return esp_err_t 启动结果
 */
esp_err_t time_manager_start_sync(void);

/**
 * @brief 停止时间同步
 * 
 * @return esp_err_t 停止结果
 */
esp_err_t time_manager_stop_sync(void);

/**
 * @brief 获取当前时间
 * 
 * @param time_struct 时间结构体指针
 * @return esp_err_t 获取结果
 */
esp_err_t time_manager_get_time(struct tm* time_struct);

/**
 * @brief 获取时间戳
 * 
 * @param timestamp 时间戳指针
 * @return esp_err_t 获取结果
 */
esp_err_t time_manager_get_timestamp(time_t* timestamp);

/**
 * @brief 检查时间是否已同步
 * 
 * @return true 时间已同步
 * @return false 时间未同步
 */
bool time_manager_is_synced(void);

/**
 * @brief 获取时间管理器状态
 * 
 * @return time_manager_state_t 当前状态
 */
time_manager_state_t time_manager_get_state(void);

/**
 * @brief 设置时区
 * 
 * @param timezone 时区字符串，如 "CST-8"
 * @return esp_err_t 设置结果
 */
esp_err_t time_manager_set_timezone(const char* timezone);

#ifdef __cplusplus
}
#endif

#endif // TIME_MANAGER_H