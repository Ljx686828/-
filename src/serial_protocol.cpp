/*
 * 串口通信协议实现 - ESP32端
 */

#include "serial_protocol.h"

// 串口发送缓冲区
static uint8_t txBuffer[64];
static unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 200;  // 200ms发送间隔

// 命令接收缓冲区
static uint8_t rxBuffer[32];
static int rxIndex = 0;
static bool receivingCommand = false;

// 外部变量声明（在main.cpp中定义）
extern bool waterLevelSafe;
extern bool tempAlarm;
extern bool pumpRunning;
extern bool manualModeActive;
extern bool tidalPumpOn;
extern float currentTemperature;
extern bool tempValid;
extern float flowRate;
extern int pumpPower;
extern unsigned long TIDAL_ON_TIME;
extern unsigned long TIDAL_OFF_TIME;

// 初始化串口通信
void initSerialProtocol() {
  Serial2.begin(SERIAL2_BAUD, SERIAL_8N1, SERIAL2_RX_PIN, SERIAL2_TX_PIN);
  delay(100);
  Serial.println("[SERIAL] Serial2 initialized for 1.85C-box communication");
}

// 计算校验和
uint8_t calculateChecksum(uint8_t* data, int len) {
  uint8_t checksum = 0;
  for (int i = 0; i < len; i++) {
    checksum ^= data[i];
  }
  return checksum;
}

// 发送系统状态
void sendSystemStatus(const SystemStatus& status) {
  txBuffer[0] = PKT_HEADER_1;
  txBuffer[1] = PKT_HEADER_2;
  txBuffer[2] = PKT_TYPE_SYSTEM_STATUS;
  txBuffer[3] = (status.waterLevelSafe ? 0x01 : 0x00) |
                 (status.tempAlarm ? 0x02 : 0x00) |
                 (status.pumpRunning ? 0x04 : 0x00) |
                 (status.manualModeActive ? 0x08 : 0x00) |
                 (status.tidalModeActive ? 0x10 : 0x00);
  txBuffer[4] = status.currentPriority;
  
  uint8_t checksum = calculateChecksum(txBuffer + 2, 3);
  txBuffer[5] = checksum;
  txBuffer[6] = PKT_END_1;
  txBuffer[7] = PKT_END_2;
  
  Serial2.write(txBuffer, 8);
}

// 发送温度数据
void sendTemperature(float temp, bool valid) {
  txBuffer[0] = PKT_HEADER_1;
  txBuffer[1] = PKT_HEADER_2;
  txBuffer[2] = PKT_TYPE_TEMPERATURE;
  
  // 将float转换为4字节
  uint8_t* tempBytes = (uint8_t*)&temp;
  txBuffer[3] = tempBytes[0];
  txBuffer[4] = tempBytes[1];
  txBuffer[5] = tempBytes[2];
  txBuffer[6] = tempBytes[3];
  txBuffer[7] = valid ? 0x01 : 0x00;
  
  uint8_t checksum = calculateChecksum(txBuffer + 2, 6);
  txBuffer[8] = checksum;
  txBuffer[9] = PKT_END_1;
  txBuffer[10] = PKT_END_2;
  
  Serial2.write(txBuffer, 11);
}

// 发送水位状态
void sendWaterLevel(bool safe) {
  txBuffer[0] = PKT_HEADER_1;
  txBuffer[1] = PKT_HEADER_2;
  txBuffer[2] = PKT_TYPE_WATER_LEVEL;
  txBuffer[3] = safe ? 0x01 : 0x00;
  
  uint8_t checksum = calculateChecksum(txBuffer + 2, 2);
  txBuffer[4] = checksum;
  txBuffer[5] = PKT_END_1;
  txBuffer[6] = PKT_END_2;
  
  Serial2.write(txBuffer, 7);
}

// 发送流量数据
void sendFlowRate(float rate) {
  txBuffer[0] = PKT_HEADER_1;
  txBuffer[1] = PKT_HEADER_2;
  txBuffer[2] = PKT_TYPE_FLOW_RATE;
  
  // 将float转换为4字节
  uint8_t* rateBytes = (uint8_t*)&rate;
  txBuffer[3] = rateBytes[0];
  txBuffer[4] = rateBytes[1];
  txBuffer[5] = rateBytes[2];
  txBuffer[6] = rateBytes[3];
  
  uint8_t checksum = calculateChecksum(txBuffer + 2, 5);
  txBuffer[7] = checksum;
  txBuffer[8] = PKT_END_1;
  txBuffer[9] = PKT_END_2;
  
  Serial2.write(txBuffer, 10);
}

// 发送水泵状态
void sendPumpStatus(const PumpStatus& status) {
  txBuffer[0] = PKT_HEADER_1;
  txBuffer[1] = PKT_HEADER_2;
  txBuffer[2] = PKT_TYPE_PUMP_STATUS;
  txBuffer[3] = status.running ? 0x01 : 0x00;
  txBuffer[4] = status.power;
  
  // 将unsigned long转换为4字节
  txBuffer[5] = (status.remainingTime >> 24) & 0xFF;
  txBuffer[6] = (status.remainingTime >> 16) & 0xFF;
  txBuffer[7] = (status.remainingTime >> 8) & 0xFF;
  txBuffer[8] = status.remainingTime & 0xFF;
  txBuffer[9] = status.isManual ? 0x01 : 0x00;
  
  uint8_t checksum = calculateChecksum(txBuffer + 2, 8);
  txBuffer[10] = checksum;
  txBuffer[11] = PKT_END_1;
  txBuffer[12] = PKT_END_2;
  
  Serial2.write(txBuffer, 13);
}

// 检查并解析命令
void checkAndParseCommands() {
  while (Serial2.available() > 0) {
    uint8_t byte = Serial2.read();
    
    // 检测命令包头部
    if (!receivingCommand) {
      if (byte == CMD_HEADER_1) {
        receivingCommand = true;
        rxIndex = 0;
        rxBuffer[rxIndex++] = byte;
      }
    } else {
      rxBuffer[rxIndex++] = byte;
      
      // 检测包结束
      if (rxIndex >= 2 && rxBuffer[rxIndex-2] == PKT_END_1 && rxBuffer[rxIndex-1] == PKT_END_2) {
        // 完整命令包接收完成
        if (rxIndex >= 6) {  // 最小命令包长度
          parseCommand(rxBuffer, rxIndex);
        }
        receivingCommand = false;
        rxIndex = 0;
      }
      
      // 防止缓冲区溢出
      if (rxIndex >= sizeof(rxBuffer)) {
        receivingCommand = false;
        rxIndex = 0;
      }
    }
  }
}

// 解析命令
bool parseCommand(uint8_t* buffer, int len) {
  if (len < 6) return false;
  
  // 检查头部
  if (buffer[0] != CMD_HEADER_1 || buffer[1] != CMD_HEADER_2) {
    return false;
  }
  
  // 检查校验和
  uint8_t checksum = calculateChecksum(buffer + 2, len - 5);
  if (checksum != buffer[len - 3]) {
    Serial.println("[SERIAL] Command checksum error");
    return false;
  }
  
  uint8_t cmd = buffer[2];
  
  switch (cmd) {
    case CMD_MANUAL_PUMP_START:
      handleManualPumpCommand();
      break;
    case CMD_PUMP_STOP:
      handleStopPumpCommand();
      break;
    case CMD_SET_TIDAL_TIME:
      if (len >= 10) {
        handleSetTidalTimeCommand(buffer + 3, len - 6);
      }
      break;
    default:
      Serial.println("[SERIAL] Unknown command: 0x" + String(cmd, HEX));
      return false;
  }
  
  return true;
}

// 处理手动启动水泵命令
void handleManualPumpCommand() {
  Serial.println("[SERIAL] Received manual pump start command from 1.85C-box");
  
  // 触发手动控制（模拟触摸传感器）
  if (!manualModeActive && waterLevelSafe) {
    extern unsigned long manualStartTime;
    extern unsigned long lastTouchTime;
    extern bool flowError;
    extern volatile unsigned long flowPulseCount;
    extern unsigned long lastPulseCount;
    extern unsigned long lastFlowCheck;
    extern void setPumpPower(int power);
    
    unsigned long now = millis();
    manualModeActive = true;
    manualStartTime = now;
    flowError = false;
    flowPulseCount = 0;
    lastPulseCount = 0;
    lastFlowCheck = now;
    
    setPumpPower(255);
    Serial.println("[SERIAL] Manual mode activated via 1.85C-box");
  }
}

// 处理停止水泵命令
void handleStopPumpCommand() {
  Serial.println("[SERIAL] Received pump stop command from 1.85C-box");
  
  extern void stopPump();
  extern bool manualModeActive;
  
  stopPump();
  manualModeActive = false;
}

// 处理设置潮汐时间命令
void handleSetTidalTimeCommand(uint8_t* params, int len) {
  if (len >= 8) {
    unsigned long onTime = ((unsigned long)params[0] << 24) |
                           ((unsigned long)params[1] << 16) |
                           ((unsigned long)params[2] << 8) |
                           params[3];
    unsigned long offTime = ((unsigned long)params[4] << 24) |
                            ((unsigned long)params[5] << 16) |
                            ((unsigned long)params[6] << 8) |
                            params[7];
    
    extern unsigned long TIDAL_ON_TIME;
    extern unsigned long TIDAL_OFF_TIME;
    
    TIDAL_ON_TIME = onTime;
    TIDAL_OFF_TIME = offTime;
    
    Serial.println("[SERIAL] Tidal time updated: " + String(TIDAL_ON_TIME/1000) + 
                   "s ON / " + String(TIDAL_OFF_TIME/1000) + "s OFF");
  }
}

