# 触摸按键测试指南

## ESP-SparkBot 硬件触摸按键布局

根据原厂设计，ESP-SparkBot 只有 **3 个触摸按键**：

```
┌─────────────────────────────────┐
│           ESP-SparkBot          │
│                                 │
│    [TOUCH_PAD_NUM2]             │
│      (左侧按键)                  │
│    上一个屏幕                    │
│                                 │
│    [TOUCH_PAD_NUM1]             │
│      (中间按键)                  │
│      确认操作                    │
│                                 │
│    [TOUCH_PAD_NUM3]             │
│      (右侧按键)                  │
│    下一个屏幕                    │
└─────────────────────────────────┘
```

## 触摸灵敏度配置

| 按键 | 触摸通道 | 灵敏度 | 功能 |
|------|----------|--------|------|
| 左侧 | TOUCH_PAD_NUM2 | 0.08 | 上一个屏幕 |
| 中间 | TOUCH_PAD_NUM1 | 0.035 | 确认操作 |
| 右侧 | TOUCH_PAD_NUM3 | 0.08 | 下一个屏幕 |

## 测试步骤

1. **烧录固件**
   ```bash
   idf.py -p COM3 flash monitor
   ```

2. **观察启动日志**
   - 查找 "Touch button manager initialized for ESP32-S3"
   - 查找 "Added touch button" 相关日志
   - 查找 "Screen manager initialized"

3. **测试触摸按键**
   - 触摸左侧：应该切换到上一个屏幕
   - 触摸右侧：应该切换到下一个屏幕  
   - 触摸中间：应该触发确认操作

4. **验证屏幕切换**
   - 主屏幕 → 屏幕2 → 屏幕3 → 屏幕4 → 设置 → 主屏幕 (循环)
   - 每次切换应该有日志输出

## 预期日志输出

```
I (xxx) TouchButtonManager: Initializing touch button manager for ESP32-S3
I (xxx) TouchButtonManager: Touch button manager initialized for ESP32-S3
I (xxx) esp_sparkbot: Initializing touch buttons for ESP32-S3 (3 buttons)
I (xxx) TouchButtonManager: Added touch button 0: Left (pad 2, threshold 0.080)
I (xxx) TouchButtonManager: Added touch button 1: Right (pad 3, threshold 0.080)
I (xxx) TouchButtonManager: Added touch button 2: Center (pad 1, threshold 0.035)
I (xxx) esp_sparkbot: Touch button Left (0) pressed
I (xxx) ScreenManager: Switched to previous screen: Main
```

## 故障排除

### 触摸按键不响应
1. 检查硬件连接
2. 调整灵敏度阈值 (0.02-0.15 范围)
3. 确认 ESP32-S3 触摸功能正常

### 屏幕切换异常
1. 检查 ScreenManager 初始化
2. 确认屏幕创建顺序正确
3. 验证事件回调函数

### 灵敏度问题
- **太敏感**: 降低阈值 (如 0.08 → 0.10)
- **不敏感**: 提高阈值 (如 0.08 → 0.06)

## 硬件参考

原厂使用 ESP-IDF 的 Touch Element 库，我们重新实现了兼容的触摸管理器：
- 使用相同的触摸通道配置
- 匹配原厂灵敏度设置
- 支持按压、释放、长按事件