/*
 * 教育友好界面特性 - 1.85C-box
 * Educational UI Features Header
 */

#ifndef GUI_EDUCATIONAL_H
#define GUI_EDUCATIONAL_H

#include <Arduino.h>

// 函数声明
void initEducationalUI();
void updateEducationalUI();
void updateWaterFlowAnimation(bool pumpRunning);
void updateTemperatureColorGradient(float temp);
void updateWaterLevelBlink(bool danger);
void buttonPressAnimation(void* button);
void showSuccessAnimation();
void showFailureAnimation();

#endif // GUI_EDUCATIONAL_H

