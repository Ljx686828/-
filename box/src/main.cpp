/*
 * 1.85C-box 主程序
 * Main Program for 1.85C-box Display Terminal
 * 
 * 功能：
 * - 通过串口接收ESP32系统状态数据
 * - 在触摸屏上显示系统状态
 * - 通过触摸屏发送控制命令给ESP32
 * - 提供教育友好的可视化界面
 */

#include <Arduino.h>
#include "serial_comm.h"
#include "touchscreen.h"
#include "gui.h"
#include "gui_components.h"
#include "gui_educational.h"
#include "audio.h"

// 触摸事件处理
TouchEvent touchEvent;
bool lastButtonPressed = false;

// 系统消息
char systemMessage[64] = "系统就绪 / System Ready";

void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println();
  Serial.println("╔════════════════════════════════════════════════════╗");
  Serial.println("║  1.85C-box 显示终端 - 鱼菜共生系统                  ║");
  Serial.println("║  1.85C-box Display Terminal                        ║");
  Serial.println("╚════════════════════════════════════════════════════╝");
  Serial.println();
  
  // 初始化串口通信
  initSerialComm();
  Serial.println("[OK] Serial communication initialized");
  
  // 初始化触摸屏
  initTouchscreen();
  Serial.println("[OK] Touchscreen initialized");
  
  // 初始化GUI
  initGUI();
  createGUIComponents();
  Serial.println("[OK] GUI initialized");
  
  // 初始化教育友好特性
  initEducationalUI();
  Serial.println("[OK] Educational UI features initialized");
  
  // 初始化音频
  initAudio();
  Serial.println("[OK] Audio system initialized");
  
  Serial.println();
  Serial.println("═══════════════════════════════════════════════════════");
  Serial.println("Ready! Waiting for ESP32 data...");
  Serial.println("═══════════════════════════════════════════════════════");
  Serial.println();
}

void loop() {
  // 更新串口通信（接收ESP32数据）
  updateSerialComm();
  
  // 更新GUI显示
  if (systemData.dataUpdated) {
    // 更新各个GUI组件
    updateTemperatureDisplay(systemData.temperature, systemData.tempValid);
    updateWaterLevelDisplay(systemData.waterLevelSafe);
    updatePumpStatusDisplay(
      systemData.pumpRunning,
      systemData.pumpPower,
      systemData.pumpRemainingTime,
      systemData.pumpIsManual
    );
    updateFlowRateDisplay(systemData.flowRate);
    updatePriorityDisplay(systemData.currentPriority);
    
    // 更新系统消息
    if (!systemData.waterLevelSafe) {
      strcpy(systemMessage, "⚠️ 水位危险！");
      playAudio(AUDIO_ALARM_WATER);
    } else if (systemData.tempAlarm) {
      strcpy(systemMessage, "⚠️ 温度异常！");
      playAudio(AUDIO_ALARM_TEMP);
    } else if (systemData.pumpRunning) {
      if (systemData.pumpIsManual) {
        strcpy(systemMessage, "手动模式运行中");
      } else {
        strcpy(systemMessage, "潮汐循环运行中");
      }
    } else {
      strcpy(systemMessage, "系统正常 / System Normal");
    }
    updateSystemMessage(systemMessage);
    
    systemData.dataUpdated = false;
  }
  
  // 更新GUI（LVGL任务处理）
  updateGUI();
  
  // 更新教育友好特性（动画等）
  updateEducationalUI();
  
  // 处理触摸事件
  if (readTouchEvent(&touchEvent)) {
    if (touchEvent.pressed) {
      // 检查是否点击了水泵按钮
      // 按钮位置：中心下方，150x50大小
      uint16_t btnX = GUI_CENTER_X - 75;
      uint16_t btnY = GUI_CENTER_Y + 120;
      
      if (isPointInButton(touchEvent.x, touchEvent.y, btnX, btnY, 150, 50)) {
        if (!lastButtonPressed) {
          // 按钮按下
          lastButtonPressed = true;
          
          // 播放按钮点击音
          playAudio(AUDIO_BUTTON_CLICK);
          
          // 发送手动启动命令给ESP32
          if (!systemData.pumpRunning) {
            sendManualPumpCommand();
            strcpy(systemMessage, "发送启动命令...");
            updateSystemMessage(systemMessage);
            playAudio(AUDIO_PUMP_START);
          } else {
            sendStopPumpCommand();
            strcpy(systemMessage, "发送停止命令...");
            updateSystemMessage(systemMessage);
            playAudio(AUDIO_PUMP_STOP);
          }
          
          // 按钮动画反馈
          // buttonPressAnimation(gui_pumpButton);
        }
      }
    } else {
      lastButtonPressed = false;
    }
  }
  
  // 检查连接状态
  static unsigned long lastDataTime = 0;
  if (systemData.lastUpdateTime > 0) {
    unsigned long timeSinceUpdate = millis() - systemData.lastUpdateTime;
    if (timeSinceUpdate > 2000) {
      // 超过2秒没有收到数据，可能连接断开
      strcpy(systemMessage, "⚠️ 连接断开 / Connection Lost");
      updateSystemMessage(systemMessage);
    }
  }
  
  delay(10);  // 10ms延迟，保持响应性
}

