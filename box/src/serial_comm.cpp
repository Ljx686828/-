/*
 * 串口通信模块实现 - 1.85C-box端
 */

#include "serial_comm.h"

// 全局系统数据
SystemData systemData = {
  .waterLevelSafe = true,
  .tempAlarm = false,
  .pumpRunning = false,
  .manualModeActive = false,
  .tidalModeActive = false,
  .currentPriority = 0,
  .temperature = 25.0,
  .tempValid = false,
  .flowRate = 0.0,
  .pumpPower = 0,
  .pumpRemainingTime = 0,
  .pumpIsManual = false,
  .dataUpdated = false,
  .lastUpdateTime = 0
};

// 接收缓冲区
static uint8_t rxBuffer[64];
static int rxIndex = 0;
static bool receivingPacket = false;
static unsigned long lastByteTime = 0;
const unsigned long PACKET_TIMEOUT = 100;  // 100ms超时

// 发送缓冲区
static uint8_t txBuffer[32];

// 初始化串口通信
void initSerialComm() {
  Serial2.begin(SERIAL_COMM_BAUD, SERIAL_8N1, SERIAL_COMM_RX_PIN, SERIAL_COMM_TX_PIN);
  delay(100);
  Serial.println("[SERIAL] Serial2 initialized for ESP32 communication");
}

// 计算校验和
uint8_t calculateChecksum(uint8_t* data, int len) {
  uint8_t checksum = 0;
  for (int i = 0; i < len; i++) {
    checksum ^= data[i];
  }
  return checksum;
}

// 更新串口通信（在主循环中调用）
void updateSerialComm() {
  receiveData();
  
  // 检查超时
  if (receivingPacket && (millis() - lastByteTime > PACKET_TIMEOUT)) {
    receivingPacket = false;
    rxIndex = 0;
  }
}

// 接收数据
bool receiveData() {
  while (Serial2.available() > 0) {
    uint8_t byte = Serial2.read();
    lastByteTime = millis();
    
    // 检测数据包头部
    if (!receivingPacket) {
      if (byte == PKT_HEADER_1) {
        receivingPacket = true;
        rxIndex = 0;
        rxBuffer[rxIndex++] = byte;
      }
    } else {
      rxBuffer[rxIndex++] = byte;
      
      // 检测包结束
      if (rxIndex >= 2 && rxBuffer[rxIndex-2] == PKT_END_1 && rxBuffer[rxIndex-1] == PKT_END_2) {
        // 完整数据包接收完成
        if (rxIndex >= 6) {  // 最小数据包长度
          parseStatusPacket(rxBuffer, rxIndex);
        }
        receivingPacket = false;
        rxIndex = 0;
      }
      
      // 防止缓冲区溢出
      if (rxIndex >= sizeof(rxBuffer)) {
        receivingPacket = false;
        rxIndex = 0;
      }
    }
  }
  
  return false;
}

// 解析状态数据包
bool parseStatusPacket(uint8_t* buffer, int len) {
  if (len < 6) return false;
  
  // 检查头部
  if (buffer[0] != PKT_HEADER_1 || buffer[1] != PKT_HEADER_2) {
    return false;
  }
  
  uint8_t type = buffer[2];
  int dataLen = len - 5;  // 减去头部(2) + 类型(1) + 校验(1) + 尾部(2)
  uint8_t* data = buffer + 3;
  
  // 检查校验和
  uint8_t checksum = calculateChecksum(buffer + 2, dataLen);
  if (checksum != buffer[len - 3]) {
    Serial.println("[SERIAL] Packet checksum error");
    return false;
  }
  
  // 更新系统数据
  updateSystemData(type, data, dataLen - 1);  // 减去校验字节
  
  return true;
}

// 更新系统数据
void updateSystemData(uint8_t type, uint8_t* data, int len) {
  systemData.dataUpdated = true;
  systemData.lastUpdateTime = millis();
  
  switch (type) {
    case PKT_TYPE_SYSTEM_STATUS:
      if (len >= 2) {
        systemData.waterLevelSafe = (data[0] & 0x01) != 0;
        systemData.tempAlarm = (data[0] & 0x02) != 0;
        systemData.pumpRunning = (data[0] & 0x04) != 0;
        systemData.manualModeActive = (data[0] & 0x08) != 0;
        systemData.tidalModeActive = (data[0] & 0x10) != 0;
        systemData.currentPriority = data[1];
      }
      break;
      
    case PKT_TYPE_TEMPERATURE:
      if (len >= 5) {
        float* tempPtr = (float*)data;
        systemData.temperature = *tempPtr;
        systemData.tempValid = (data[4] == 0x01);
      }
      break;
      
    case PKT_TYPE_WATER_LEVEL:
      if (len >= 1) {
        systemData.waterLevelSafe = (data[0] == 0x01);
      }
      break;
      
    case PKT_TYPE_FLOW_RATE:
      if (len >= 4) {
        float* ratePtr = (float*)data;
        systemData.flowRate = *ratePtr;
      }
      break;
      
    case PKT_TYPE_PUMP_STATUS:
      if (len >= 9) {
        systemData.pumpRunning = (data[0] == 0x01);
        systemData.pumpPower = data[1];
        systemData.pumpRemainingTime = ((unsigned long)data[2] << 24) |
                                       ((unsigned long)data[3] << 16) |
                                       ((unsigned long)data[4] << 8) |
                                       data[5];
        systemData.pumpIsManual = (data[6] == 0x01);
      }
      break;
      
    default:
      Serial.println("[SERIAL] Unknown packet type: 0x" + String(type, HEX));
      return;
  }
}

// 发送手动启动水泵命令
void sendManualPumpCommand() {
  txBuffer[0] = CMD_HEADER_1;
  txBuffer[1] = CMD_HEADER_2;
  txBuffer[2] = CMD_MANUAL_PUMP_START;
  
  uint8_t checksum = calculateChecksum(txBuffer + 2, 1);
  txBuffer[3] = checksum;
  txBuffer[4] = PKT_END_1;
  txBuffer[5] = PKT_END_2;
  
  Serial2.write(txBuffer, 6);
  Serial.println("[SERIAL] Sent manual pump start command");
}

// 发送停止水泵命令
void sendStopPumpCommand() {
  txBuffer[0] = CMD_HEADER_1;
  txBuffer[1] = CMD_HEADER_2;
  txBuffer[2] = CMD_PUMP_STOP;
  
  uint8_t checksum = calculateChecksum(txBuffer + 2, 1);
  txBuffer[3] = checksum;
  txBuffer[4] = PKT_END_1;
  txBuffer[5] = PKT_END_2;
  
  Serial2.write(txBuffer, 6);
  Serial.println("[SERIAL] Sent pump stop command");
}

// 发送设置潮汐时间命令
void sendSetTidalTimeCommand(unsigned long onTime, unsigned long offTime) {
  txBuffer[0] = CMD_HEADER_1;
  txBuffer[1] = CMD_HEADER_2;
  txBuffer[2] = CMD_SET_TIDAL_TIME;
  txBuffer[3] = (onTime >> 24) & 0xFF;
  txBuffer[4] = (onTime >> 16) & 0xFF;
  txBuffer[5] = (onTime >> 8) & 0xFF;
  txBuffer[6] = onTime & 0xFF;
  txBuffer[7] = (offTime >> 24) & 0xFF;
  txBuffer[8] = (offTime >> 16) & 0xFF;
  txBuffer[9] = (offTime >> 8) & 0xFF;
  txBuffer[10] = offTime & 0xFF;
  
  uint8_t checksum = calculateChecksum(txBuffer + 2, 9);
  txBuffer[11] = checksum;
  txBuffer[12] = PKT_END_1;
  txBuffer[13] = PKT_END_2;
  
  Serial2.write(txBuffer, 14);
  Serial.println("[SERIAL] Sent set tidal time command");
}

