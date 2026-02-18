/*
 * LVGL GUI核心模块 - 1.85C-box
 * LVGL GUI Core Module
 */

#ifndef GUI_H
#define GUI_H

#include <Arduino.h>

// LVGL配置
#define LVGL_BUFFER_SIZE        (10 * 1024)  // 10KB显示缓冲区
#define LVGL_REFRESH_PERIOD     50            // 50ms刷新周期

// 屏幕配置（圆形360×360）
#define GUI_SCREEN_WIDTH        360
#define GUI_SCREEN_HEIGHT       360
#define GUI_SCREEN_RADIUS       180
#define GUI_CENTER_X            180
#define GUI_CENTER_Y            180

// 函数声明
void initGUI();
void updateGUI();
void guiTask(void* parameter);

// GUI组件句柄（在gui.cpp中定义）
extern void* gui_tempLabel;
extern void* gui_waterLabel;
extern void* gui_pumpLabel;
extern void* gui_flowLabel;
extern void* gui_pumpButton;
extern void* gui_progressBar;
extern void* gui_messageLabel;
extern void* gui_priorityLabel;

#endif // GUI_H

