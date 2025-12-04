# ESP32-S3 触摸屏切换功能使用指南

## 功能概述

本项目为 xiaozhi-esp32 集成了基于 ESP32-S3 原生触摸按键的屏幕切换功能，支持通过触摸设备左右两侧来切换不同的屏幕页面。

## 硬件配置

### 触摸按键映射 (基于 ESP-SparkBot 硬件设计)
- **TOUCH_PAD_NUM1** (中间按键): 确认操作 (灵敏度: 0.035)
- **TOUCH_PAD_NUM2** (左侧按键): 上一个屏幕 (灵敏度: 0.08)
- **TOUCH_PAD_NUM3** (右侧按键): 下一个屏幕 (灵敏度: 0.08)

**注意**: ESP-SparkBot 硬件只有 3 个触摸按键，没有额外的 TOUCH_PAD_NUM4

## 软件架构

### 核心组件

1. **ScreenManager** (`display/screen_manager.h/cc`)
   - 屏幕管理器，负责屏幕的创建、切换和事件处理
   - 支持循环切换 (最后一个屏幕 → 第一个屏幕)
   - 提供 LVGL 手势支持和自定义事件处理

2. **TouchButtonManager** (`input/touch_button_manager.h/cc`)
   - 触摸按键管理器，基于 ESP32 原生触摸功能
   - 支持滤波、防抖和长按检测
   - 提供可配置的触摸阈值和回调机制

3. **屏幕类** (`display/screens.h/cc`)
   - **MainScreen**: 主屏幕，显示项目信息
   - **EmptyScreen**: 空屏幕模板，可扩展
   - **SettingsScreen**: 设置屏幕
   - **ScreenFactory**: 屏幕工厂，负责创建和初始化所有屏幕

## 使用方法

### 1. 构建项目
```bash
# 设置 ESP-IDF 环境
idf.py set-target esp32s3

# 构建项目
idf.py build

# 烧录固件
idf.py -p COM3 flash monitor
```

### 2. 触摸操作
- **触摸左侧**: 切换到上一个屏幕
- **触摸右侧**: 切换到下一个屏幕
- **触摸中间**: 执行确认操作

### 3. 屏幕切换顺序
1. 主屏幕 (MainScreen)
2. 屏幕 2 (EmptyScreen)
3. 屏幕 3 (EmptyScreen)
4. 屏幕 4 (EmptyScreen)
5. 设置屏幕 (SettingsScreen)

## 技术特性

### 触摸按键特性
- **采样率**: 50Hz (20ms 间隔)
- **滤波**: 5 点移动平均滤波
- **防抖**: 3 次连续采样确认
- **长按检测**: 1000ms 触发长按事件

### 屏幕管理特性
- **循环切换**: 支持首尾相连的循环切换
- **手势支持**: 同时支持 LVGL 左右滑动手势
- **事件回调**: 提供屏幕切换事件的回调机制
- **动态管理**: 支持运行时添加/删除屏幕

## 扩展开发

### 添加新屏幕
1. 继承 `Screen` 基类
2. 实现必要的虚函数
3. 在 `ScreenFactory::CreateAllScreens()` 中注册

```cpp
class MyCustomScreen : public Screen {
public:
    MyCustomScreen(const char* name = "MyScreen") { name_ = name; }
    
    void Create() override {
        // 创建屏幕 UI
    }
    
    void Destroy() override {
        // 清理资源
    }
    
    void Show() override {
        // 显示屏幕
    }
    
    void Hide() override {
        // 隐藏屏幕
    }
    
    void HandleEvent(screen_event_t event) override {
        // 处理屏幕事件
    }
};
```

### 自定义触摸按键
```cpp
// 在 esp_sparkbot_board.cc 的 InitializeButtons() 中
touch_manager.AddButton(TOUCH_PAD_NUM4, 0.06f, "Custom");

touch_manager.SetCallback([](int button_id, touch_button_event_t event) {
    if (event == TOUCH_BUTTON_EVT_PRESS) {
        // 处理自定义按键事件
    }
});
```

## 配置参数

### 触摸按键阈值调整
在 `esp_sparkbot_board.cc` 中调整阈值：
```cpp
touch_manager.AddButton(TOUCH_PAD_NUM2, 0.06f, "Left");   // 降低灵敏度
touch_manager.AddButton(TOUCH_PAD_NUM2, 0.04f, "Left");   // 提高灵敏度
```

### 屏幕切换动画
当前使用无动画切换 (`LV_SCR_LOAD_ANIM_NONE`)，可在 `screen_manager.cc` 中修改为带动画的切换。

## 故障排除

### 触摸按键不响应
1. 检查触摸通道配置是否正确
2. 调整触摸阈值 (0.02 - 0.15 范围)
3. 确认硬件连接正常

### 屏幕切换卡顿
1. 检查 LVGL 任务优先级
2. 优化屏幕创建/销毁逻辑
3. 调整触摸采样频率

### 编译错误
1. 确认 ESP-IDF 环境设置正确
2. 检查目标芯片是否为 esp32s3
3. 更新 managed_components

## 技术参考

- [ESP32-S3 触摸传感文档](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/touch_pad.html)
- [LVGL 手势识别](https://docs.lvgl.io/master/overview/gesture.html)
- [FreeRTOS 任务管理](https://www.freertos.org/Documentation/RTOS_book.html)

## 版本信息

- **目标芯片**: ESP32-S3
- **LVGL 版本**: 9.2.2
- **ESP-IDF 版本**: 5.x
- **项目版本**: v2.x