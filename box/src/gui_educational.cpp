/*
 * 教育友好界面特性 - 1.85C-box
 * Educational UI Features
 */

#include "gui_educational.h"
#include "gui_components.h"
#include "serial_comm.h"

// 动画状态
static bool waterFlowAnimation = false;
static unsigned long lastAnimationUpdate = 0;
const unsigned long ANIMATION_INTERVAL = 100;  // 100ms动画更新间隔

// 初始化教育友好特性
void initEducationalUI() {
  Serial.println("[EDU] Initializing educational UI features...");
  
  // TODO: 实际LVGL动画初始化
  // 1. 创建水流动画对象
  // 2. 创建颜色渐变动画
  // 3. 创建闪烁动画
  // 4. 创建按钮缩放动画
}

// 更新水流动画（水泵运行时）
void updateWaterFlowAnimation(bool pumpRunning) {
  if (pumpRunning && !waterFlowAnimation) {
    waterFlowAnimation = true;
    // TODO: 启动水流动画
    // lv_anim_t a;
    // lv_anim_init(&a);
    // lv_anim_set_var(&a, waterFlowObj);
    // lv_anim_set_values(&a, 0, 360);
    // lv_anim_set_time(&a, 2000);
    // lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    // lv_anim_set_exec_cb(&a, waterFlowAnimFunc);
    // lv_anim_start(&a);
  } else if (!pumpRunning && waterFlowAnimation) {
    waterFlowAnimation = false;
    // TODO: 停止水流动画
  }
}

// 更新温度颜色渐变
void updateTemperatureColorGradient(float temp) {
  // TODO: 根据温度值计算颜色并应用渐变
  // if (temp < 20) {
  //   // 蓝色渐变
  // } else if (temp > 28) {
  //   // 红色渐变
  // } else {
  //   // 绿色渐变
  // }
}

// 更新水位警告闪烁
void updateWaterLevelBlink(bool danger) {
  // TODO: 危险时启动闪烁动画
  // if (danger) {
  //   // 启动红色闪烁动画
  // } else {
  //   // 停止闪烁
  // }
}

// 按钮按下动画反馈
void buttonPressAnimation(void* button) {
  // TODO: 按钮按下时的缩放动画
  // lv_anim_t a;
  // lv_anim_init(&a);
  // lv_anim_set_var(&a, button);
  // lv_anim_set_values(&a, 100, 90);  // 缩小到90%
  // lv_anim_set_time(&a, 100);
  // lv_anim_set_exec_cb(&a, scaleAnimFunc);
  // lv_anim_start(&a);
}

// 操作成功动画
void showSuccessAnimation() {
  // TODO: 显示绿色对勾动画
  Serial.println("[EDU] Success animation");
}

// 操作失败动画
void showFailureAnimation() {
  // TODO: 显示红色X动画
  Serial.println("[EDU] Failure animation");
}

// 更新所有教育友好特性
void updateEducationalUI() {
  unsigned long now = millis();
  
  if (now - lastAnimationUpdate >= ANIMATION_INTERVAL) {
    // 更新水流动画
    updateWaterFlowAnimation(systemData.pumpRunning);
    
    // 更新温度颜色渐变
    if (systemData.tempValid) {
      updateTemperatureColorGradient(systemData.temperature);
    }
    
    // 更新水位警告闪烁
    updateWaterLevelBlink(!systemData.waterLevelSafe);
    
    lastAnimationUpdate = now;
  }
}

