/*
 * ============================================
 * 智能鱼菜共生系统 / Smart Aquaponics System
 * ============================================
 * 
 * Primary Function: 鱼菜共生自动化 (Aquaponics Automation)
 * Secondary Function: STEM Education for Children (8-12 years)
 * 
 * This program teaches three important concepts:
 * 1. PERCEPTION: Reading sensors to understand the environment (感知)
 * 2. SAFETY & LOGIC: Using "if-then-else" to protect equipment (安全逻辑)
 * 3. INTERACTION: Responding to user input (交互)
 * 
 * Hardware Connections:
 * - Display: Serial2 (TX2=GPIO17, RX2=GPIO16) - 480x272 UART Screen
 * - DS18B20 Temperature: GPIO 4 (OneWire with 4.7kΩ pull-up)
 * - Liquid Level Sensor: GPIO 5 (Digital Input: LOW = No Water = DANGER)
 * - Touch Sensor: GPIO 15 (Digital Input: HIGH = Touched)
 * - Water Pump: GPIO 13 (Digital Output: HIGH = ON)
 * 
 * UI Screen Layout (Nextion/TJC compatible):
 * ┌────────────────────────────────────────────────────────┐
 * │  [Header: t0] 智能鱼菜共生 Smart Aquaponics            │
 * ├────────────────────────────────────────────────────────┤
 * │                                                        │
 * │   ┌─────────────┐    ┌─────────────┐                  │
 * │   │ 水温 TEMP   │    │ 水位 LEVEL  │                  │
 * │   │   [t1]      │    │   [t2]      │                  │
 * │   │  25.3°C     │    │    安全     │                  │
 * │   │   [n0]      │    │   [p0]      │                  │
 * │   └─────────────┘    └─────────────┘                  │
 * │                                                        │
 * │   ┌─────────────────────────────────┐                 │
 * │   │      水泵状态 PUMP STATUS        │                 │
 * │   │           [t3]                   │                 │
 * │   │         已停止 OFF               │                 │
 * │   └─────────────────────────────────┘                 │
 * │                                                        │
 * │   ┌─────────────────────────────────┐                 │
 * │   │  [b0] 启动水泵 / START PUMP     │  <- Touch Area  │
 * │   └─────────────────────────────────┘                 │
 * │                                                        │
 * │   [t4] 系统消息 / System Message                       │
 * └────────────────────────────────────────────────────────┘
 * 
 * UI Elements (for Nextion/TJC Editor):
 * - t0: Header text (title)
 * - t1: Temperature label
 * - n0: Temperature value (number component) or use t5 for text
 * - t2: Water level label  
 * - p0: Water level indicator (picture/icon) or use t6 for text
 * - t3: Pump status text
 * - b0: Pump button (for future touch screen)
 * - t4: System message / alerts
 */

#include <OneWire.h>
#include <DallasTemperature.h>

// ============================================
// STEP 1: Define our "Input Pins" (Sensors)
// 第一步：定义输入引脚（传感器）
// ============================================

#define PIN_TEMPERATURE_SENSOR 4    // DS18B20 水温传感器
#define PIN_LIQUID_LEVEL_SENSOR 5   // 水位传感器: LOW = 无水 (危险!)
#define PIN_TOUCH_SENSOR 15         // 触摸传感器: HIGH = 触摸

// ============================================
// STEP 2: Define our "Output Pins" (Actuators)
// 第二步：定义输出引脚（执行器）
// ============================================

#define PIN_WATER_PUMP 13           // 水泵控制: HIGH = 开启

// ============================================
// STEP 3: System Configuration
// 第三步：系统配置
// ============================================

// Temperature thresholds for aquaponics (鱼菜共生温度阈值)
const float TEMP_WARNING_HIGH = 28.0;   // 高温警告 (°C)
const float TEMP_WARNING_LOW = 18.0;    // 低温警告 (°C)
const float TEMP_OPTIMAL_MIN = 22.0;    // 最佳温度下限
const float TEMP_OPTIMAL_MAX = 26.0;    // 最佳温度上限

// Timing configuration (时间配置)
const unsigned long TEMP_READ_INTERVAL = 2000;   // 温度读取间隔 (2秒)
const unsigned long PUMP_RUN_TIME = 5000;        // 水泵运行时间 (5秒)
const unsigned long DISPLAY_UPDATE_INTERVAL = 500; // 显示更新间隔 (0.5秒)
const unsigned long TOUCH_DEBOUNCE_TIME = 200;   // 触摸防抖时间 (200ms)

// ============================================
// STEP 4: Create Sensor Objects
// 第四步：创建传感器对象
// ============================================

OneWire oneWire(PIN_TEMPERATURE_SENSOR);
DallasTemperature tempSensor(&oneWire);

// ============================================
// STEP 5: System State Variables
// 第五步：系统状态变量
// ============================================

// Temperature state (温度状态)
float currentTemperature = 0.0;
bool temperatureValid = false;
unsigned long lastTempRead = 0;

// Water level state (水位状态)
bool waterLevelSafe = true;
bool lastWaterLevelSafe = true;

// Pump state (水泵状态)
bool pumpRunning = false;
unsigned long pumpStartTime = 0;
unsigned long pumpRemainingTime = 0;

// Touch sensor state (触摸传感器状态)
bool touchSensorState = false;
bool lastTouchSensorState = false;
unsigned long lastTouchTime = 0;

// Display state tracking (显示状态跟踪) - Prevents display flooding
unsigned long lastDisplayUpdate = 0;
int lastDisplayedTemp = -999;        // Track last displayed temperature (x10 for 1 decimal)
bool lastDisplayedWaterSafe = true;
bool lastDisplayedPumpState = false;
int lastDisplayedPumpTime = -1;
String lastSystemMessage = "";

// System state (系统状态)
enum SystemState {
  STATE_NORMAL,           // 正常运行
  STATE_ALARM_NO_WATER,   // 警报：无水
  STATE_ALARM_TEMP_HIGH,  // 警报：温度过高
  STATE_ALARM_TEMP_LOW,   // 警报：温度过低
  STATE_PUMP_RUNNING      // 水泵运行中
};
SystemState currentState = STATE_NORMAL;
SystemState lastState = STATE_NORMAL;

// ============================================
// STEP 6: Function Declarations
// 第六步：函数声明
// ============================================

void sendDisplayCommand(String command);
void initializeDisplay();
void updateTemperatureDisplay(float temp, bool valid);
void updateWaterLevelDisplay(bool safe);
void updatePumpDisplay(bool running, int remainingSeconds);
void updateSystemMessage(String message, uint16_t color);
void checkWaterLevel();
void checkTouchSensor();
void checkPumpTimer();
void readTemperature();
void updateDisplayIfNeeded();

// ============================================
// Color Definitions (Nextion format: RGB565)
// 颜色定义
// ============================================

const uint16_t COLOR_RED = 63488;      // 红色 - 危险/警告
const uint16_t COLOR_GREEN = 2016;     // 绿色 - 安全/正常
const uint16_t COLOR_BLUE = 31;        // 蓝色 - 信息
const uint16_t COLOR_ORANGE = 64512;   // 橙色 - 注意
const uint16_t COLOR_WHITE = 65535;    // 白色
const uint16_t COLOR_BLACK = 0;        // 黑色
const uint16_t COLOR_CYAN = 2047;      // 青色 - 水泵运行

// ============================================
// SETUP: Runs Once When System Starts
// 设置：系统启动时运行一次
// ============================================

void setup() {
  // Step 1: Start Serial for debugging (调试串口)
  Serial.begin(115200);
  Serial.println();
  Serial.println("========================================");
  Serial.println("智能鱼菜共生系统 / Smart Aquaponics System");
  Serial.println("========================================");
  Serial.println("Initializing...");
  
  // Step 2: Initialize Serial2 for display (显示屏串口)
  Serial2.begin(9600);
  delay(500);  // Wait for display to boot (等待显示屏启动)
  
  // Step 3: Initialize temperature sensor (初始化温度传感器)
  tempSensor.begin();
  tempSensor.setResolution(12);  // 12-bit resolution for accuracy
  Serial.println("[OK] Temperature sensor initialized");
  
  // Step 4: Configure INPUT pins (配置输入引脚)
  pinMode(PIN_LIQUID_LEVEL_SENSOR, INPUT);
  pinMode(PIN_TOUCH_SENSOR, INPUT);
  Serial.println("[OK] Input pins configured");
  
  // Step 5: Configure OUTPUT pins - SAFETY FIRST! (配置输出引脚 - 安全第一!)
  pinMode(PIN_WATER_PUMP, OUTPUT);
  digitalWrite(PIN_WATER_PUMP, LOW);  // Ensure pump is OFF at startup!
  Serial.println("[OK] Water pump initialized (OFF for safety)");
  
  // Step 6: Initialize display UI (初始化显示界面)
  initializeDisplay();
  Serial.println("[OK] Display initialized");
  
  // Step 7: Initial sensor read (首次传感器读取)
  waterLevelSafe = digitalRead(PIN_LIQUID_LEVEL_SENSOR);
  tempSensor.requestTemperatures();
  currentTemperature = tempSensor.getTempCByIndex(0);
  temperatureValid = (currentTemperature != -127.0 && currentTemperature != 85.0);
  
  Serial.println("========================================");
  Serial.println("System Ready! / 系统就绪!");
  Serial.println("========================================");
}

// ============================================
// Initialize Display UI
// 初始化显示界面
// ============================================

void initializeDisplay() {
  // Clear any previous state (清除之前的状态)
  delay(100);
  
  // Set page to main page (if using multiple pages)
  sendDisplayCommand("page 0");
  delay(50);
  
  // Header / 标题
  sendDisplayCommand("t0.txt=\"智能鱼菜共生 Aquaponics\"");
  delay(20);
  
  // Temperature section / 温度区域
  sendDisplayCommand("tTemp.txt=\"水温 TEMP\"");
  delay(20);
  sendDisplayCommand("t1.txt=\"--.-°C\"");
  sendDisplayCommand("t1.pco=" + String(COLOR_WHITE));
  delay(20);
  
  // Water level section / 水位区域
  sendDisplayCommand("tLevel.txt=\"水位 LEVEL\"");
  delay(20);
  sendDisplayCommand("t2.txt=\"检测中...\"");
  sendDisplayCommand("t2.pco=" + String(COLOR_WHITE));
  delay(20);
  
  // Pump status section / 水泵状态区域
  sendDisplayCommand("tPump.txt=\"水泵 PUMP\"");
  delay(20);
  sendDisplayCommand("t3.txt=\"已停止 OFF\"");
  sendDisplayCommand("t3.pco=" + String(COLOR_WHITE));
  delay(20);
  
  // Button text (for future touch) / 按钮文字
  sendDisplayCommand("b0.txt=\"启动水泵 START\"");
  delay(20);
  
  // System message / 系统消息
  sendDisplayCommand("t4.txt=\"系统就绪 System Ready\"");
  sendDisplayCommand("t4.pco=" + String(COLOR_GREEN));
  delay(20);
}

// ============================================
// MAIN LOOP: The "Heartbeat" of the System
// 主循环：系统的"心跳"
// ============================================

void loop() {
  // ============================================
  // PRIORITY 1: SAFETY - Check Water Level
  // 优先级1：安全 - 检查水位
  // ============================================
  // This is the MOST IMPORTANT check!
  // 这是最重要的检查！
  
  checkWaterLevel();
  
  // If no water, skip everything else (safety lock)
  // 如果没有水，跳过其他所有操作（安全锁）
  if (!waterLevelSafe) {
    delay(100);
    return;
  }
  
  // ============================================
  // PRIORITY 2: User Interaction - Touch Sensor
  // 优先级2：用户交互 - 触摸传感器
  // ============================================
  
  checkTouchSensor();
  
  // ============================================
  // PRIORITY 3: Pump Timer Management
  // 优先级3：水泵计时管理
  // ============================================
  
  checkPumpTimer();
  
  // ============================================
  // PRIORITY 4: Environment Monitoring - Temperature
  // 优先级4：环境监测 - 温度
  // ============================================
  
  readTemperature();
  
  // ============================================
  // PRIORITY 5: Update Display (rate-limited)
  // 优先级5：更新显示（限速）
  // ============================================
  
  updateDisplayIfNeeded();
  
  // Small delay to prevent CPU overload
  // 小延迟防止CPU过载
  delay(10);
}

// ============================================
// Check Water Level (Safety Critical!)
// 检查水位（安全关键！）
// ============================================

void checkWaterLevel() {
  // Read current water level
  // 读取当前水位
  waterLevelSafe = digitalRead(PIN_LIQUID_LEVEL_SENSOR);
  // Remember: LOW = No Water = DANGER!
  // 记住：LOW = 无水 = 危险！
  
  if (!waterLevelSafe) {
    // EMERGENCY: No water detected!
    // 紧急情况：检测到无水！
    
    // Step 1: IMMEDIATELY turn OFF pump (protect from dry-running)
    // 第一步：立即关闭水泵（防止干烧）
    digitalWrite(PIN_WATER_PUMP, LOW);
    pumpRunning = false;
    
    // Step 2: Update state
    // 第二步：更新状态
    currentState = STATE_ALARM_NO_WATER;
    
    // Step 3: Update display only if state changed
    // 第三步：仅在状态改变时更新显示
    if (waterLevelSafe != lastWaterLevelSafe || currentState != lastState) {
      updateWaterLevelDisplay(false);
      updatePumpDisplay(false, 0);
      updateSystemMessage("警报:请加水! ALARM:ADD WATER!", COLOR_RED);
      
      Serial.println("!!! ALARM: NO WATER DETECTED !!!");
      Serial.println("!!! 警报：检测到无水 !!!");
      
      lastWaterLevelSafe = waterLevelSafe;
      lastState = currentState;
    }
  } else {
    // Water level is safe
    // 水位安全
    
    // Update display only if state changed from unsafe to safe
    // 仅在状态从不安全变为安全时更新显示
    if (waterLevelSafe != lastWaterLevelSafe) {
      updateWaterLevelDisplay(true);
      
      if (!pumpRunning) {
        currentState = STATE_NORMAL;
        updateSystemMessage("水位正常 Water OK", COLOR_GREEN);
      }
      
      Serial.println("[OK] Water level restored / 水位恢复正常");
      lastWaterLevelSafe = waterLevelSafe;
    }
  }
}

// ============================================
// Check Touch Sensor (User Interaction)
// 检查触摸传感器（用户交互）
// ============================================

void checkTouchSensor() {
  touchSensorState = digitalRead(PIN_TOUCH_SENSOR);
  unsigned long currentTime = millis();
  
  // Detect rising edge (LOW -> HIGH) with debouncing
  // 检测上升沿（LOW -> HIGH）带防抖
  if (touchSensorState == HIGH && lastTouchSensorState == LOW) {
    // Check debounce time
    // 检查防抖时间
    if (currentTime - lastTouchTime >= TOUCH_DEBOUNCE_TIME) {
      // Valid touch detected!
      // 检测到有效触摸！
      
      Serial.println("[TOUCH] Sensor activated / 触摸传感器激活");
      
      // Only start pump if water is safe and pump is not already running
      // 仅在水位安全且水泵未运行时启动
      if (waterLevelSafe && !pumpRunning) {
        // Start the pump!
        // 启动水泵！
        digitalWrite(PIN_WATER_PUMP, HIGH);
        pumpRunning = true;
        pumpStartTime = millis();
        currentState = STATE_PUMP_RUNNING;
        
        Serial.println("[PUMP] Started for 5 seconds / 启动5秒");
        
        // Update display immediately
        // 立即更新显示
        updatePumpDisplay(true, 5);
        updateSystemMessage("水泵运行中 Pump Running", COLOR_CYAN);
      } else if (pumpRunning) {
        Serial.println("[PUMP] Already running / 水泵已在运行");
      }
      
      lastTouchTime = currentTime;
    }
  }
  
  lastTouchSensorState = touchSensorState;
}

// ============================================
// Check Pump Timer
// 检查水泵计时器
// ============================================

void checkPumpTimer() {
  if (pumpRunning) {
    unsigned long elapsed = millis() - pumpStartTime;
    
    if (elapsed >= PUMP_RUN_TIME) {
      // Time's up - turn off pump
      // 时间到 - 关闭水泵
      digitalWrite(PIN_WATER_PUMP, LOW);
      pumpRunning = false;
      currentState = STATE_NORMAL;
      
      Serial.println("[PUMP] Stopped (5 seconds elapsed) / 已停止（5秒已过）");
      
      // Update display
      // 更新显示
      updatePumpDisplay(false, 0);
      updateSystemMessage("水泵已停止 Pump Stopped", COLOR_GREEN);
      lastDisplayedPumpTime = -1;
    } else {
      // Update remaining time display
      // 更新剩余时间显示
      int remainingSeconds = (PUMP_RUN_TIME - elapsed) / 1000 + 1;
      
      // Only update if seconds changed
      // 仅在秒数改变时更新
      if (remainingSeconds != lastDisplayedPumpTime) {
        updatePumpDisplay(true, remainingSeconds);
        lastDisplayedPumpTime = remainingSeconds;
      }
    }
  }
}

// ============================================
// Read Temperature Sensor
// 读取温度传感器
// ============================================

void readTemperature() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastTempRead >= TEMP_READ_INTERVAL) {
    // Request temperature reading
    // 请求温度读取
    tempSensor.requestTemperatures();
    float temp = tempSensor.getTempCByIndex(0);
    
    // Validate reading (DS18B20 returns -127 or 85 on error)
    // 验证读数（DS18B20 错误时返回 -127 或 85）
    if (temp != -127.0 && temp != 85.0) {
      currentTemperature = temp;
      temperatureValid = true;
      
      // Check temperature thresholds
      // 检查温度阈值
      if (temp > TEMP_WARNING_HIGH) {
        if (currentState == STATE_NORMAL || currentState == STATE_ALARM_TEMP_LOW) {
          currentState = STATE_ALARM_TEMP_HIGH;
          updateSystemMessage("警告:温度过高! TEMP HIGH!", COLOR_ORANGE);
        }
      } else if (temp < TEMP_WARNING_LOW) {
        if (currentState == STATE_NORMAL || currentState == STATE_ALARM_TEMP_HIGH) {
          currentState = STATE_ALARM_TEMP_LOW;
          updateSystemMessage("警告:温度过低! TEMP LOW!", COLOR_ORANGE);
        }
      } else if (currentState == STATE_ALARM_TEMP_HIGH || currentState == STATE_ALARM_TEMP_LOW) {
        currentState = STATE_NORMAL;
        updateSystemMessage("温度正常 Temp Normal", COLOR_GREEN);
      }
      
      Serial.print("[TEMP] ");
      Serial.print(temp, 1);
      Serial.println("°C");
    } else {
      temperatureValid = false;
      Serial.println("[TEMP] Sensor error! / 传感器错误！");
    }
    
    lastTempRead = currentTime;
  }
}

// ============================================
// Update Display If Needed (Rate Limited)
// 按需更新显示（限速）
// ============================================

void updateDisplayIfNeeded() {
  unsigned long currentTime = millis();
  
  // Rate limit display updates
  // 限制显示更新频率
  if (currentTime - lastDisplayUpdate < DISPLAY_UPDATE_INTERVAL) {
    return;
  }
  
  // Update temperature display if changed
  // 如果温度改变则更新显示
  int tempInt = (int)(currentTemperature * 10);  // Convert to int for comparison
  if (tempInt != lastDisplayedTemp || !temperatureValid) {
    updateTemperatureDisplay(currentTemperature, temperatureValid);
    lastDisplayedTemp = tempInt;
  }
  
  lastDisplayUpdate = currentTime;
}

// ============================================
// Display Update Functions
// 显示更新函数
// ============================================

void updateTemperatureDisplay(float temp, bool valid) {
  if (valid) {
    // Format: "25.3°C"
    String tempStr = String(temp, 1) + "°C";
    sendDisplayCommand("t1.txt=\"" + tempStr + "\"");
    
    // Set color based on temperature range
    // 根据温度范围设置颜色
    uint16_t color;
    if (temp > TEMP_WARNING_HIGH || temp < TEMP_WARNING_LOW) {
      color = COLOR_RED;
    } else if (temp >= TEMP_OPTIMAL_MIN && temp <= TEMP_OPTIMAL_MAX) {
      color = COLOR_GREEN;
    } else {
      color = COLOR_ORANGE;
    }
    sendDisplayCommand("t1.pco=" + String(color));
  } else {
    sendDisplayCommand("t1.txt=\"错误 ERROR\"");
    sendDisplayCommand("t1.pco=" + String(COLOR_RED));
  }
}

void updateWaterLevelDisplay(bool safe) {
  if (safe) {
    sendDisplayCommand("t2.txt=\"安全 SAFE\"");
    sendDisplayCommand("t2.pco=" + String(COLOR_GREEN));
  } else {
    sendDisplayCommand("t2.txt=\"危险! DANGER!\"");
    sendDisplayCommand("t2.pco=" + String(COLOR_RED));
  }
  lastDisplayedWaterSafe = safe;
}

void updatePumpDisplay(bool running, int remainingSeconds) {
  if (running) {
    String pumpStr = "运行中 " + String(remainingSeconds) + "s";
    sendDisplayCommand("t3.txt=\"" + pumpStr + "\"");
    sendDisplayCommand("t3.pco=" + String(COLOR_CYAN));
    sendDisplayCommand("b0.txt=\"运行中 RUNNING\"");
  } else {
    sendDisplayCommand("t3.txt=\"已停止 OFF\"");
    sendDisplayCommand("t3.pco=" + String(COLOR_WHITE));
    sendDisplayCommand("b0.txt=\"启动水泵 START\"");
  }
  lastDisplayedPumpState = running;
}

void updateSystemMessage(String message, uint16_t color) {
  if (message != lastSystemMessage) {
    sendDisplayCommand("t4.txt=\"" + message + "\"");
    sendDisplayCommand("t4.pco=" + String(color));
    lastSystemMessage = message;
    
    Serial.print("[MSG] ");
    Serial.println(message);
  }
}

// ============================================
// Send Command to Display
// 发送命令到显示屏
// ============================================

void sendDisplayCommand(String command) {
  Serial2.print(command);
  Serial2.write(0xFF);
  Serial2.write(0xFF);
  Serial2.write(0xFF);
}

// ============================================
// Handle Touch Screen Events (Future Use)
// 处理触摸屏事件（未来使用）
// ============================================
// When upgrading to touch screen, add this function to handle
// button press events from the display:
//
// void checkDisplayTouch() {
//   if (Serial2.available() >= 7) {
//     // Nextion touch event format: 0x65 PAGE_ID COMPONENT_ID EVENT 0xFF 0xFF 0xFF
//     if (Serial2.read() == 0x65) {
//       uint8_t pageId = Serial2.read();
//       uint8_t componentId = Serial2.read();
//       uint8_t event = Serial2.read();
//       
//       // Clear remaining bytes
//       Serial2.read(); Serial2.read(); Serial2.read();
//       
//       // Check if b0 (pump button) was pressed
//       if (componentId == 1 && event == 1) {  // b0 pressed
//         // Same logic as touch sensor
//         if (waterLevelSafe && !pumpRunning) {
//           digitalWrite(PIN_WATER_PUMP, HIGH);
//           pumpRunning = true;
//           pumpStartTime = millis();
//           // ... rest of pump start logic
//         }
//       }
//     }
//   }
// }
