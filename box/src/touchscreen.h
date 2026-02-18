/*
 * 触摸屏驱动模块 - 1.85C-box
 * Touchscreen Driver Module
 */

#ifndef TOUCHSCREEN_H
#define TOUCHSCREEN_H

#include <Arduino.h>

// 触摸事件结构
struct TouchEvent {
  bool pressed;
  uint16_t x;
  uint16_t y;
  bool hasEvent;
};

// I2C配置（根据实际硬件调整）
#define TOUCH_I2C_ADDR          0x38  // GT911默认地址，可能需要调整
#define TOUCH_I2C_SDA           10    // 根据实际硬件调整
#define TOUCH_I2C_SCL           11    // 根据实际硬件调整

// 屏幕尺寸（圆形360×360）
#define SCREEN_WIDTH            360
#define SCREEN_HEIGHT           360
#define SCREEN_RADIUS           180

// 函数声明
void initTouchscreen();
bool readTouchEvent(TouchEvent* event);
bool isPointInCircle(uint16_t x, uint16_t y, uint16_t centerX, uint16_t centerY, uint16_t radius);
bool isPointInButton(uint16_t x, uint16_t y, uint16_t btnX, uint16_t btnY, uint16_t btnW, uint16_t btnH);

#endif // TOUCHSCREEN_H

