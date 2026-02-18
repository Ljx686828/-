/*
 * 智能鱼菜共生系统 v2.0 - 4级优先级控制
 * Smart Aquaponics System v2.0 - 4-Level Priority Control
 * 
 * 硬件: ESP32 普中开发板 (Puzhong Dev Board)
 * 
 * 优先级架构:
 *   Level 1: 干烧保护 (Dry-Burn Prevention) - 最高
 *   Level 2: 温度监控 (Thermal Guard)
 *   Level 3: 手动控制 (Manual Override)
 *   Level 4: 潮汐循环 (Tidal Cycle) - 最低
 * 
 * 引脚配置:
 *   INPUT:  GPIO 4 (水位), GPIO 15 (触摸), GPIO 16 (流量), GPIO 27 (温度)
 *   OUTPUT: GPIO 13 (水泵PWM), GPIO 2 (蜂鸣器)
 */

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "serial_protocol.h"

// ==================== 引脚定义 ====================
#define PIN_LIQUID_LEVEL    4     // 水位: LOW=危险, HIGH=安全
#define PIN_TEMP_SENSOR     27    // DS18B20 (OneWire)
#define PIN_TOUCH_SENSOR    15    // 触摸: HIGH=触摸
#define PIN_FLOW_SENSOR     16    // 流量脉冲 (中断)
#define PIN_PUMP            13    // 水泵PWM
#define PIN_BUZZER          2     // 蜂鸣器

// PWM配置
#define PWM_CHANNEL         0
#define PWM_FREQ            1000
#define PWM_RESOLUTION      8

// ==================== 系统参数 ====================
const float TEMP_MAX = 30.0;
const float TEMP_MIN = 18.0;

const unsigned long TEMP_READ_INTERVAL = 2000;
const unsigned long TOUCH_DEBOUNCE = 200;
const unsigned long MANUAL_PUMP_TIME = 5000;
const unsigned long FLOW_CHECK_TIMEOUT = 2000;

// 潮汐循环 (测试用短时间，正式使用改为600000/1800000)
unsigned long TIDAL_ON_TIME = 10000;   // 10秒
unsigned long TIDAL_OFF_TIME = 30000;  // 30秒

const unsigned long BEEP_INTERVAL = 1000;
const unsigned long BEEP_DURATION = 100;

// ==================== 状态变量 ====================
OneWire oneWire(PIN_TEMP_SENSOR);
DallasTemperature tempSensor(&oneWire);

// Level 1
bool waterLevelSafe = true;

// Level 2
float currentTemperature = 25.0;
bool tempValid = false;
bool tempAlarm = false;
unsigned long lastTempRead = 0;
unsigned long lastBeepTime = 0;
bool buzzerState = false;

// Level 3
bool manualModeActive = false;
unsigned long manualStartTime = 0;
unsigned long lastTouchTime = 0;

// Level 4
bool tidalPumpOn = false;
unsigned long tidalCycleStart = 0;

// 流量计
volatile unsigned long flowPulseCount = 0;
unsigned long lastFlowCheck = 0;
unsigned long lastPulseCount = 0;
float flowRate = 0.0;
bool flowError = false;

// 水泵
bool pumpRunning = false;
int pumpPower = 0;

// ==================== 中断处理 ====================
void IRAM_ATTR flowPulseISR() {
  flowPulseCount++;
}

// ==================== 函数声明 ====================
void readWaterLevel();
void readTemperature();
void calculateFlowRate();
void setPumpPower(int power);
void stopPump();
void setBuzzer(bool state);
void handleDryBurnProtection();
void handleThermalGuard();
void handleManualOverride();
void handleTidalCycle();

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println();
  Serial.println("╔════════════════════════════════════════════════════╗");
  Serial.println("║  智能鱼菜共生系统 v2.0 - 4级优先级控制              ║");
  Serial.println("║  Smart Aquaponics System v2.0                      ║");
  Serial.println("╠════════════════════════════════════════════════════╣");
  Serial.println("║  L1: 干烧保护  L2: 温度监控                        ║");
  Serial.println("║  L3: 手动控制  L4: 潮汐循环                        ║");
  Serial.println("╚════════════════════════════════════════════════════╝");
  Serial.println();
  
  // 输入引脚
  pinMode(PIN_LIQUID_LEVEL, INPUT);
  pinMode(PIN_TOUCH_SENSOR, INPUT);
  pinMode(PIN_FLOW_SENSOR, INPUT_PULLUP);
  Serial.println("[OK] Input pins configured");
  
  // 输出引脚
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  Serial.println("[OK] Buzzer initialized");
  
  // PWM
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(PIN_PUMP, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 0);
  Serial.println("[OK] Pump PWM initialized (OFF)");
  
  // 流量中断
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW_SENSOR), flowPulseISR, RISING);
  Serial.println("[OK] Flow sensor interrupt attached");
  
  // 温度传感器
  tempSensor.begin();
  tempSensor.setResolution(12);
  Serial.println("[OK] DS18B20 initialized");
  
  // 串口通信（用于1.85C-box）
  initSerialProtocol();
  
  readWaterLevel();
  tidalCycleStart = millis();
  
  Serial.println();
  Serial.println("═══════════════════════════════════════════════════════");
  Serial.println("Ready! Tidal: " + String(TIDAL_ON_TIME/1000) + "s ON / " + String(TIDAL_OFF_TIME/1000) + "s OFF");
  Serial.println("═══════════════════════════════════════════════════════");
  Serial.println();
}

// ==================== MAIN LOOP ====================
void loop() {
  // Level 1: 干烧保护 (最高优先级，每次必须检查)
  handleDryBurnProtection();
  
  if (!waterLevelSafe) {
    delay(50);
    return;  // 水位不安全时跳过所有其他操作
  }
  
  // Level 2: 温度监控
  handleThermalGuard();
  
  // Level 3: 手动控制
  handleManualOverride();
  
  // Level 4: 潮汐循环 (仅在手动模式未激活时)
  if (!manualModeActive) {
    handleTidalCycle();
  }
  
  // 流量监测
  if (pumpRunning) {
    calculateFlowRate();
  }
  
  // 串口通信：检查并处理来自1.85C-box的命令
  checkAndParseCommands();
  
  // 串口通信：定期发送系统状态数据（每200ms）
  static unsigned long lastSerialSend = 0;
  unsigned long now = millis();
  if (now - lastSerialSend >= 200) {
    // 发送系统状态
    SystemStatus sysStatus;
    sysStatus.waterLevelSafe = waterLevelSafe;
    sysStatus.tempAlarm = tempAlarm;
    sysStatus.pumpRunning = pumpRunning;
    sysStatus.manualModeActive = manualModeActive;
    sysStatus.tidalModeActive = tidalPumpOn;
    
    // 确定当前优先级
    if (!waterLevelSafe) {
      sysStatus.currentPriority = 1;  // Level 1: 干烧保护
    } else if (tempAlarm) {
      sysStatus.currentPriority = 2;  // Level 2: 温度监控
    } else if (manualModeActive) {
      sysStatus.currentPriority = 3;  // Level 3: 手动控制
    } else if (tidalPumpOn) {
      sysStatus.currentPriority = 4;  // Level 4: 潮汐循环
    } else {
      sysStatus.currentPriority = 0;  // 空闲
    }
    
    sendSystemStatus(sysStatus);
    
    // 发送温度数据
    sendTemperature(currentTemperature, tempValid);
    
    // 发送水位状态
    sendWaterLevel(waterLevelSafe);
    
    // 发送流量数据
    sendFlowRate(flowRate);
    
    // 发送水泵状态
    PumpStatus pumpStatus;
    pumpStatus.running = pumpRunning;
    pumpStatus.power = pumpPower;
    pumpStatus.isManual = manualModeActive;
    
    if (manualModeActive) {
      unsigned long elapsed = now - manualStartTime;
      pumpStatus.remainingTime = (MANUAL_PUMP_TIME > elapsed) ? (MANUAL_PUMP_TIME - elapsed) : 0;
    } else if (tidalPumpOn) {
      unsigned long elapsed = now - tidalCycleStart;
      unsigned long total = TIDAL_ON_TIME + TIDAL_OFF_TIME;
      unsigned long pos = elapsed % total;
      pumpStatus.remainingTime = (pos < TIDAL_ON_TIME) ? (TIDAL_ON_TIME - pos) : 0;
    } else {
      pumpStatus.remainingTime = 0;
    }
    
    sendPumpStatus(pumpStatus);
    
    lastSerialSend = now;
  }
  
  delay(10);
}

// ==================== Level 1: 干烧保护 ====================
void handleDryBurnProtection() {
  readWaterLevel();
  static bool lastSafe = true;
  
  if (!waterLevelSafe) {
    stopPump();
    manualModeActive = false;
    
    if (lastSafe != waterLevelSafe) {
      Serial.println();
      Serial.println("╔═══════════════════════════════════════════════╗");
      Serial.println("║  ⚠️  CRITICAL: DRY-BURN PROTECT               ║");
      Serial.println("║  水泵已紧急停止！请检查水位！                 ║");
      Serial.println("╚═══════════════════════════════════════════════╝");
      Serial.println();
    }
  } else if (lastSafe != waterLevelSafe) {
    Serial.println("[OK] 水位正常 / Water level OK");
  }
  
  lastSafe = waterLevelSafe;
}

// ==================== Level 2: 温度监控 ====================
void handleThermalGuard() {
  unsigned long now = millis();
  
  // 每2秒读取温度
  if (now - lastTempRead >= TEMP_READ_INTERVAL) {
    readTemperature();
    lastTempRead = now;
    
    if (tempValid) {
      tempAlarm = (currentTemperature > TEMP_MAX || currentTemperature < TEMP_MIN);
      
      Serial.print("[TEMP] ");
      Serial.print(currentTemperature, 1);
      Serial.print("°C ");
      Serial.println(tempAlarm ? "⚠️ OUT OF RANGE!" : "✓ OK");
    }
  }
  
  // 温度警报蜂鸣
  if (tempAlarm) {
    if (now - lastBeepTime >= BEEP_INTERVAL) {
      setBuzzer(true);
      lastBeepTime = now;
    } else if (now - lastBeepTime >= BEEP_DURATION) {
      setBuzzer(false);
    }
  } else {
    setBuzzer(false);
  }
}

// ==================== Level 3: 手动控制 ====================
void handleManualOverride() {
  unsigned long now = millis();
  static bool lastTouch = false;
  bool currentTouch = digitalRead(PIN_TOUCH_SENSOR);
  
  // 检测触摸上升沿
  if (currentTouch && !lastTouch && !manualModeActive) {
    if (now - lastTouchTime >= TOUCH_DEBOUNCE) {
      Serial.println();
      Serial.println("[TOUCH] Manual mode activated - 5 seconds");
      
      manualModeActive = true;
      manualStartTime = now;
      flowError = false;
      flowPulseCount = 0;
      lastPulseCount = 0;
      lastFlowCheck = now;
      
      setPumpPower(255);
      lastTouchTime = now;
    }
  }
  lastTouch = currentTouch;
  
  // 手动模式运行中
  if (manualModeActive) {
    unsigned long elapsed = now - manualStartTime;
    
    // 流量检测
    if (elapsed >= FLOW_CHECK_TIMEOUT && !flowError && flowPulseCount == 0) {
      Serial.println();
      Serial.println("╔════════════════════════════════════════╗");
      Serial.println("║  ERROR: NO FLOW - 无流量检测！         ║");
      Serial.println("╚════════════════════════════════════════╝");
      flowError = true;
      stopPump();
      manualModeActive = false;
    }
    
    // 5秒后停止
    if (elapsed >= MANUAL_PUMP_TIME) {
      Serial.println("[MANUAL] Mode ended | Pulses: " + String(flowPulseCount));
      stopPump();
      manualModeActive = false;
    } else if (!flowError) {
      static int lastSec = -1;
      int remaining = (MANUAL_PUMP_TIME - elapsed) / 1000 + 1;
      if (remaining != lastSec) {
        Serial.println("[MANUAL] " + String(remaining) + "s | Pulses: " + String(flowPulseCount));
        lastSec = remaining;
      }
    }
  }
}

// ==================== Level 4: 潮汐循环 ====================
void handleTidalCycle() {
  unsigned long now = millis();
  unsigned long elapsed = now - tidalCycleStart;
  unsigned long total = TIDAL_ON_TIME + TIDAL_OFF_TIME;
  unsigned long pos = elapsed % total;
  
  bool shouldBeOn = (pos < TIDAL_ON_TIME);
  
  if (shouldBeOn && !tidalPumpOn) {
    Serial.println();
    Serial.println("[TIDAL] ═══ PUMP ON ═══");
    setPumpPower(255);
    tidalPumpOn = true;
    flowPulseCount = 0;
    lastFlowCheck = now;
  } else if (!shouldBeOn && tidalPumpOn) {
    Serial.println();
    Serial.println("[TIDAL] ═══ PUMP OFF ═══ | Pulses: " + String(flowPulseCount));
    stopPump();
    tidalPumpOn = false;
  }
  
  // 状态显示 (每5秒)
  static unsigned long lastPrint = 0;
  if (now - lastPrint >= 5000) {
    if (tidalPumpOn) {
      int remain = (TIDAL_ON_TIME - pos) / 1000;
      Serial.println("[TIDAL] Running | " + String(remain) + "s left | Flow: " + String(flowRate, 1) + " p/s");
    } else {
      int remain = (total - pos) / 1000;
      Serial.println("[TIDAL] Waiting | Next start: " + String(remain) + "s");
    }
    lastPrint = now;
  }
}

// ==================== 传感器函数 ====================
void readWaterLevel() {
  waterLevelSafe = digitalRead(PIN_LIQUID_LEVEL);
}

void readTemperature() {
  tempSensor.requestTemperatures();
  float temp = tempSensor.getTempCByIndex(0);
  
  if (temp != -127.0 && temp != 85.0 && temp > -40 && temp < 85) {
    currentTemperature = temp;
    tempValid = true;
  } else {
    tempValid = false;
    Serial.println("[TEMP] ⚠️ Sensor error!");
  }
}

void calculateFlowRate() {
  unsigned long now = millis();
  unsigned long timeDiff = now - lastFlowCheck;
  
  if (timeDiff >= 500) {
    unsigned long pulseDiff = flowPulseCount - lastPulseCount;
    flowRate = (float)pulseDiff / (timeDiff / 1000.0);
    lastPulseCount = flowPulseCount;
    lastFlowCheck = now;
  }
}

// ==================== 执行器函数 ====================
void setPumpPower(int power) {
  if (!waterLevelSafe) power = 0;
  power = constrain(power, 0, 255);
  
  ledcWrite(PWM_CHANNEL, power);
  pumpPower = power;
  pumpRunning = (power > 0);
  
  if (power > 0) {
    Serial.println("[PUMP] ON - Power: " + String((power * 100) / 255) + "%");
  }
}

void stopPump() {
  ledcWrite(PWM_CHANNEL, 0);
  pumpPower = 0;
  pumpRunning = false;
}

void setBuzzer(bool state) {
  digitalWrite(PIN_BUZZER, state ? HIGH : LOW);
  buzzerState = state;
}

