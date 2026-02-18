/*
 * GUI组件模块实现 - 1.85C-box
 */

#include "gui_components.h"
#include "gui.h"
#include "serial_comm.h"

// 创建GUI组件
void createGUIComponents() {
  Serial.println("[GUI] Creating GUI components...");
  
  // TODO: 实际LVGL组件创建代码
  // 这里提供组件布局和创建框架
  
  // 实际代码示例框架：
  /*
  // 创建主屏幕对象
  lv_obj_t* screen = lv_scr_act();
  lv_obj_set_style_bg_color(screen, LV_COLOR_MAKE(0, 0, 0), 0);  // 黑色背景
  
  // 创建圆形mask（适配圆形屏幕）
  lv_obj_t* mask = lv_obj_create(screen);
  lv_obj_set_size(mask, GUI_SCREEN_WIDTH, GUI_SCREEN_HEIGHT);
  lv_obj_set_pos(mask, 0, 0);
  lv_obj_set_style_radius(mask, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_clip_corner(mask, true, 0);
  
  // 创建标题
  lv_obj_t* title = lv_label_create(screen);
  lv_label_set_text(title, "Smart Aquaponics");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
  
  // 创建温度显示（大号，居中上方）
  gui_tempLabel = lv_label_create(screen);
  lv_label_set_text(gui_tempLabel, "25.0°C");
  lv_obj_set_style_text_font(gui_tempLabel, &lv_font_montserrat_48, 0);
  lv_obj_align(gui_tempLabel, LV_ALIGN_CENTER, 0, -80);
  
  // 创建水位状态显示
  gui_waterLabel = lv_label_create(screen);
  lv_label_set_text(gui_waterLabel, "安全 SAFE");
  lv_obj_set_style_text_font(gui_waterLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(gui_waterLabel, LV_ALIGN_CENTER, 0, -20);
  
  // 创建水泵状态显示
  gui_pumpLabel = lv_label_create(screen);
  lv_label_set_text(gui_pumpLabel, "停止 STOPPED");
  lv_obj_set_style_text_font(gui_pumpLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(gui_pumpLabel, LV_ALIGN_CENTER, 0, 20);
  
  // 创建进度条
  gui_progressBar = lv_bar_create(screen);
  lv_obj_set_size(gui_progressBar, 200, 20);
  lv_obj_align(gui_progressBar, LV_ALIGN_CENTER, 0, 50);
  lv_bar_set_range(gui_progressBar, 0, 100);
  lv_bar_set_value(gui_progressBar, 0, LV_ANIM_OFF);
  
  // 创建流量显示
  gui_flowLabel = lv_label_create(screen);
  lv_label_set_text(gui_flowLabel, "0.0 p/s");
  lv_obj_set_style_text_font(gui_flowLabel, &lv_font_montserrat_18, 0);
  lv_obj_align(gui_flowLabel, LV_ALIGN_CENTER, 0, 80);
  
  // 创建手动控制按钮
  gui_pumpButton = lv_btn_create(screen);
  lv_obj_set_size(gui_pumpButton, 150, 50);
  lv_obj_align(gui_pumpButton, LV_ALIGN_CENTER, 0, 120);
  lv_obj_t* btnLabel = lv_label_create(gui_pumpButton);
  lv_label_set_text(btnLabel, "启动水泵");
  lv_obj_center(btnLabel);
  
  // 创建优先级显示
  gui_priorityLabel = lv_label_create(screen);
  lv_label_set_text(gui_priorityLabel, "IDLE");
  lv_obj_set_style_text_font(gui_priorityLabel, &lv_font_montserrat_16, 0);
  lv_obj_align(gui_priorityLabel, LV_ALIGN_TOP_LEFT, 10, 10);
  
  // 创建系统消息显示
  gui_messageLabel = lv_label_create(screen);
  lv_label_set_text(gui_messageLabel, "系统就绪");
  lv_obj_set_style_text_font(gui_messageLabel, &lv_font_montserrat_14, 0);
  lv_obj_align(gui_messageLabel, LV_ALIGN_BOTTOM_MID, 0, -10);
  */
  
  Serial.println("[GUI] GUI components framework ready");
}

// 更新温度显示
void updateTemperatureDisplay(float temp, bool valid) {
  if (!valid) return;
  
  // TODO: 实际LVGL更新代码
  // lv_label_set_text_fmt(gui_tempLabel, "%.1f°C", temp);
  
  // 根据温度设置颜色
  // if (temp < 20) {
  //   lv_obj_set_style_text_color(gui_tempLabel, LV_COLOR_MAKE(0, 100, 255), 0);  // 蓝色
  // } else if (temp > 28) {
  //   lv_obj_set_style_text_color(gui_tempLabel, LV_COLOR_MAKE(255, 0, 0), 0);  // 红色
  // } else {
  //   lv_obj_set_style_text_color(gui_tempLabel, LV_COLOR_MAKE(0, 255, 0), 0);  // 绿色
  // }
}

// 更新水位状态显示
void updateWaterLevelDisplay(bool safe) {
  // TODO: 实际LVGL更新代码
  // if (safe) {
  //   lv_label_set_text(gui_waterLabel, "安全 SAFE");
  //   lv_obj_set_style_text_color(gui_waterLabel, LV_COLOR_MAKE(0, 255, 0), 0);
  // } else {
  //   lv_label_set_text(gui_waterLabel, "危险 DANGER");
  //   lv_obj_set_style_text_color(gui_waterLabel, LV_COLOR_MAKE(255, 0, 0), 0);
  // }
}

// 更新水泵状态显示
void updatePumpStatusDisplay(bool running, uint8_t power, unsigned long remainingTime, bool isManual) {
  // TODO: 实际LVGL更新代码
  // if (running) {
  //   lv_label_set_text(gui_pumpLabel, "运行 RUNNING");
  //   lv_obj_set_style_text_color(gui_pumpLabel, LV_COLOR_MAKE(0, 255, 0), 0);
  //   
  //   // 更新进度条
  //   if (remainingTime > 0) {
  //     unsigned long totalTime = isManual ? 5000 : 10000;
  //     int percent = (remainingTime * 100) / totalTime;
  //     lv_bar_set_value(gui_progressBar, percent, LV_ANIM_ON);
  //   }
  // } else {
  //   lv_label_set_text(gui_pumpLabel, "停止 STOPPED");
  //   lv_obj_set_style_text_color(gui_pumpLabel, LV_COLOR_MAKE(128, 128, 128), 0);
  //   lv_bar_set_value(gui_progressBar, 0, LV_ANIM_ON);
  // }
}

// 更新流量显示
void updateFlowRateDisplay(float rate) {
  // TODO: 实际LVGL更新代码
  // lv_label_set_text_fmt(gui_flowLabel, "%.1f p/s", rate);
}

// 更新优先级显示
void updatePriorityDisplay(uint8_t priority) {
  // TODO: 实际LVGL更新代码
  // if (priority > 0) {
  //   lv_label_set_text_fmt(gui_priorityLabel, "L%d", priority);
  // } else {
  //   lv_label_set_text(gui_priorityLabel, "IDLE");
  // }
}

// 更新系统消息
void updateSystemMessage(const char* message) {
  // TODO: 实际LVGL更新代码
  // lv_label_set_text(gui_messageLabel, message);
}

// 设置水泵按钮状态
void setPumpButtonState(bool enabled) {
  // TODO: 实际LVGL更新代码
  // lv_obj_set_state(gui_pumpButton, enabled ? LV_STATE_DEFAULT : LV_STATE_DISABLED);
}

