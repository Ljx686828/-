/*
 * 音频反馈模块 - 1.85C-box
 * Audio Feedback Module
 */

#ifndef AUDIO_H
#define AUDIO_H

#include <Arduino.h>

// 音效类型
enum AudioType {
  AUDIO_BUTTON_CLICK,
  AUDIO_ALARM_WATER,
  AUDIO_ALARM_TEMP,
  AUDIO_SUCCESS,
  AUDIO_PUMP_START,
  AUDIO_PUMP_STOP
};

// 函数声明
void initAudio();
void playAudio(AudioType type);
void stopAudio();

#endif // AUDIO_H

