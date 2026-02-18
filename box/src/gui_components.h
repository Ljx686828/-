/*
 * GUI组件模块 - 1.85C-box
 * GUI Components Module
 */

#ifndef GUI_COMPONENTS_H
#define GUI_COMPONENTS_H

#include <Arduino.h>

// 函数声明
void createGUIComponents();
void updateTemperatureDisplay(float temp, bool valid);
void updateWaterLevelDisplay(bool safe);
void updatePumpStatusDisplay(bool running, uint8_t power, unsigned long remainingTime, bool isManual);
void updateFlowRateDisplay(float rate);
void updatePriorityDisplay(uint8_t priority);
void updateSystemMessage(const char* message);
void setPumpButtonState(bool enabled);

#endif // GUI_COMPONENTS_H

