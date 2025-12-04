#include "screen_manager.h"
#include <esp_log.h>
#include <algorithm>
#include <cstring>

static const char* TAG = "ScreenManager";

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
        
        // 删除屏幕
        (*it)->Destroy();
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
    
    HideCurrentScreen();
    current_screen_index_ = next_index;
    ShowCurrentScreen();
    
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
    
    HideCurrentScreen();
    current_screen_index_ = prev_index;
    ShowCurrentScreen();
    
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
        auto& screen = screens_[current_screen_index_];
        
        if (!screen->GetRoot()) {
            screen->Create();
        }
        
        screen->Show();
        ESP_LOGD(TAG, "Showing screen: %s", screen->GetName());
    }
}

void ScreenManager::HideCurrentScreen() {
    if (current_screen_index_ >= 0 && current_screen_index_ < static_cast<int>(screens_.size())) {
        auto& screen = screens_[current_screen_index_];
        screen->Hide();
        ESP_LOGD(TAG, "Hiding screen: %s", screen->GetName());
    }
}