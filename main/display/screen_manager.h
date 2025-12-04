#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include <lvgl.h>
#include <vector>
#include <functional>
#include <memory>

// 屏幕切换事件
typedef enum {
    SCREEN_EVENT_NEXT = 100,     // 下一个屏幕
    SCREEN_EVENT_PREVIOUS = 101, // 上一个屏幕
    SCREEN_EVENT_GOTO = 102      // 跳转到指定屏幕
} screen_event_t;

// 屏幕基类
class Screen {
public:
    virtual ~Screen() = default;
    virtual void Create() = 0;
    virtual void Destroy() = 0;
    virtual void Show() = 0;
    virtual void Hide() = 0;
    virtual void HandleEvent(screen_event_t event) {}
    
    lv_obj_t* GetRoot() { return root_; }
    const char* GetName() { return name_; }
    
protected:
    lv_obj_t* root_ = nullptr;
    const char* name_ = "Screen";
};

// 屏幕管理器
class ScreenManager {
public:
    static ScreenManager& GetInstance();
    
    // 屏幕管理
    void AddScreen(std::unique_ptr<Screen> screen);
    void RemoveScreen(const char* name);
    void SwitchToScreen(const char* name);
    void SwitchToNextScreen();
    void SwitchToPreviousScreen();
    
    // 事件处理
    void HandleEvent(screen_event_t event);
    void SetEventCallback(std::function<void(screen_event_t, const char*)> callback);
    
    // 获取当前屏幕信息
    Screen* GetCurrentScreen();
    const char* GetCurrentScreenName();
    size_t GetScreenCount();
    
    // LVGL事件处理器
    static void LvglEventHandler(lv_event_t* e);
    
private:
    ScreenManager() = default;
    ~ScreenManager() = default;
    ScreenManager(const ScreenManager&) = delete;
    ScreenManager& operator=(const ScreenManager&) = delete;
    
    std::vector<std::unique_ptr<Screen>> screens_;
    int current_screen_index_ = -1;
    std::function<void(screen_event_t, const char*)> event_callback_;
    
    void ShowCurrentScreen();
    void HideCurrentScreen();
};

#endif // SCREEN_MANAGER_H