#include "screen_manager.h"
#include <esp_log.h>
#include <algorithm>
#include <cstring>

static const char* TAG = "ScreenManager";

// 静态回调函数用于 lv_async_call
static void destroy_screen_async_cb(void* user_data) {
    Screen* screen = static_cast<Screen*>(user_data);
    if (screen) {
        screen->Destroy();
    }
}

// 异步显示屏幕回调
static void show_screen_async_cb(void* user_data) {
    Screen* screen = static_cast<Screen*>(user_data);
    if (screen) {
        if (!screen->GetRoot()) {
            screen->Create();
        }
        screen->Show();
        
        // 异步加载屏幕
        lv_obj_t* root = screen->GetRoot();
        if (root) {
            lv_obj_clear_flag(root, LV_OBJ_FLAG_HIDDEN);
            lv_scr_load(root);
        }
    }
}



// 组合异步回调：隐藏当前屏幕并显示新屏幕
typedef struct {
    Screen* old_screen;
    Screen* new_screen;
} screen_switch_data_t;

static void screen_switch_complete_cb(void* user_data) {
    screen_switch_data_t* data = static_cast<screen_switch_data_t*>(user_data);
    if (data) {
        // 显示新屏幕
        if (data->new_screen) {
            if (!data->new_screen->GetRoot()) {
                data->new_screen->Create();
            }
            data->new_screen->Show();
            
            lv_obj_t* root = data->new_screen->GetRoot();
            if (root) {
                lv_obj_clear_flag(root, LV_OBJ_FLAG_HIDDEN);
                lv_scr_load(root);
            }
        }
        
        // 释放内存
        free(data);
    }
}

ScreenManager& ScreenManager::GetInstance() {
    static ScreenManager instance;
    return instance;
}

void ScreenManager::AddScreen(std::unique_ptr<Screen> screen) {
    if (!screen) {
        ESP_LOGE(TAG, "Cannot add null screen");
        return;
    }
    
    ESP_LOGI(TAG, "Adding screen: %s", screen->GetName());
    screens_.push_back(std::move(screen));
    
    // 如果是第一个屏幕，自动切换到它
    if (screens_.size() == 1) {
        current_screen_index_ = 0;
        ShowCurrentScreen();
    }
}

void ScreenManager::RemoveScreen(const char* name) {
    auto it = std::find_if(screens_.begin(), screens_.end(),
        [name](const std::unique_ptr<Screen>& screen) {
            return strcmp(screen->GetName(), name) == 0;
        });
    
    if (it != screens_.end()) {
        int removed_index = std::distance(screens_.begin(), it);
        
        // 如果要删除的是当前屏幕，先隐藏它
        if (removed_index == current_screen_index_) {
            HideCurrentScreen();
        }
        
        // 删除屏幕 - 调度到 LVGL 任务中执行
        Screen* screen_to_destroy = it->get();
        lv_async_call(destroy_screen_async_cb, screen_to_destroy);
        screens_.erase(it);
        
        // 调整当前屏幕索引
        if (current_screen_index_ >= static_cast<int>(screens_.size())) {
            current_screen_index_ = screens_.size() - 1;
        } else if (current_screen_index_ > removed_index) {
            current_screen_index_--;
        }
        
        // 如果还有屏幕，显示当前屏幕
        if (current_screen_index_ >= 0 && !screens_.empty()) {
            ShowCurrentScreen();
        } else {
            current_screen_index_ = -1;
        }
        
        ESP_LOGI(TAG, "Removed screen: %s", name);
    }
}

void ScreenManager::SwitchToScreen(const char* name) {
    auto it = std::find_if(screens_.begin(), screens_.end(),
        [name](const std::unique_ptr<Screen>& screen) {
            return strcmp(screen->GetName(), name) == 0;
        });
    
    if (it != screens_.end()) {
        int new_index = std::distance(screens_.begin(), it);
        
        if (new_index != current_screen_index_) {
            HideCurrentScreen();
            current_screen_index_ = new_index;
            ShowCurrentScreen();
            
            ESP_LOGI(TAG, "Switched to screen: %s", name);
            
            // 触发回调
            if (event_callback_) {
                event_callback_(SCREEN_EVENT_GOTO, name);
            }
        }
    } else {
        ESP_LOGW(TAG, "Screen not found: %s", name);
    }
}

void ScreenManager::SwitchToNextScreen() {
    if (screens_.empty()) {
        return;
    }
    
    int next_index = current_screen_index_ + 1;
    if (next_index >= static_cast<int>(screens_.size())) {
        next_index = 0; // 循环到第一个屏幕
    }
    
    Screen* old_screen = (current_screen_index_ >= 0) ? screens_[current_screen_index_].get() : nullptr;
    Screen* new_screen = screens_[next_index].get();
    
    current_screen_index_ = next_index;
    
    // 使用组合异步操作避免阻塞
    if (old_screen) {
        // 分配切换数据
        screen_switch_data_t* switch_data = static_cast<screen_switch_data_t*>(malloc(sizeof(screen_switch_data_t)));
        if (switch_data) {
            switch_data->old_screen = old_screen;
            switch_data->new_screen = new_screen;
            
            // 先隐藏旧屏幕
            old_screen->Hide();
            
            // 然后异步完成切换
            lv_async_call(screen_switch_complete_cb, switch_data);
        }
    } else {
        // 如果没有旧屏幕，直接显示新屏幕
        ShowCurrentScreen();
    }
    
    ESP_LOGI(TAG, "Switched to next screen: %s", screens_[current_screen_index_]->GetName());
    
    // 触发回调
    if (event_callback_) {
        event_callback_(SCREEN_EVENT_NEXT, screens_[current_screen_index_]->GetName());
    }
}

void ScreenManager::SwitchToPreviousScreen() {
    if (screens_.empty()) {
        return;
    }
    
    int prev_index = current_screen_index_ - 1;
    if (prev_index < 0) {
        prev_index = screens_.size() - 1; // 循环到最后一个屏幕
    }
    
    Screen* old_screen = (current_screen_index_ >= 0) ? screens_[current_screen_index_].get() : nullptr;
    Screen* new_screen = screens_[prev_index].get();
    
    current_screen_index_ = prev_index;
    
    // 使用组合异步操作避免阻塞
    if (old_screen) {
        // 分配切换数据
        screen_switch_data_t* switch_data = static_cast<screen_switch_data_t*>(malloc(sizeof(screen_switch_data_t)));
        if (switch_data) {
            switch_data->old_screen = old_screen;
            switch_data->new_screen = new_screen;
            
            // 先隐藏旧屏幕
            old_screen->Hide();
            
            // 然后异步完成切换
            lv_async_call(screen_switch_complete_cb, switch_data);
        }
    } else {
        // 如果没有旧屏幕，直接显示新屏幕
        ShowCurrentScreen();
    }
    
    ESP_LOGI(TAG, "Switched to previous screen: %s", screens_[current_screen_index_]->GetName());
    
    // 触发回调
    if (event_callback_) {
        event_callback_(SCREEN_EVENT_PREVIOUS, screens_[current_screen_index_]->GetName());
    }
}

void ScreenManager::HandleEvent(screen_event_t event) {
    switch (event) {
        case SCREEN_EVENT_NEXT:
            SwitchToNextScreen();
            break;
        case SCREEN_EVENT_PREVIOUS:
            SwitchToPreviousScreen();
            break;
        default:
            if (current_screen_index_ >= 0 && current_screen_index_ < static_cast<int>(screens_.size())) {
                screens_[current_screen_index_]->HandleEvent(event);
            }
            break;
    }
}

void ScreenManager::SetEventCallback(std::function<void(screen_event_t, const char*)> callback) {
    event_callback_ = callback;
}

Screen* ScreenManager::GetCurrentScreen() {
    if (current_screen_index_ >= 0 && current_screen_index_ < static_cast<int>(screens_.size())) {
        return screens_[current_screen_index_].get();
    }
    return nullptr;
}

const char* ScreenManager::GetCurrentScreenName() {
    Screen* screen = GetCurrentScreen();
    return screen ? screen->GetName() : nullptr;
}

size_t ScreenManager::GetScreenCount() {
    return screens_.size();
}

void ScreenManager::LvglEventHandler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        
        switch (dir) {
            case LV_DIR_LEFT:
                GetInstance().HandleEvent(SCREEN_EVENT_NEXT);
                break;
            case LV_DIR_RIGHT:
                GetInstance().HandleEvent(SCREEN_EVENT_PREVIOUS);
                break;
            default:
                break;
        }
    }
}

void ScreenManager::ShowCurrentScreen() {
    if (current_screen_index_ >= 0 && current_screen_index_ < static_cast<int>(screens_.size())) {
        Screen* screen_ptr = screens_[current_screen_index_].get();
        
        ESP_LOGI(TAG, "Showing screen: %s", screen_ptr->GetName());
        
        // 使用异步调用避免阻塞，防止看门狗超时
        lv_async_call(show_screen_async_cb, screen_ptr);
    }
}

void ScreenManager::HideCurrentScreen() {
    if (current_screen_index_ >= 0 && current_screen_index_ < static_cast<int>(screens_.size())) {
        Screen* screen_ptr = screens_[current_screen_index_].get();
        
        // 简单隐藏，避免复杂的父对象操作
        if (screen_ptr->GetRoot()) {
            screen_ptr->Hide();
        }
        
        ESP_LOGD(TAG, "Hiding screen: %s", screen_ptr->GetName());
    }
}