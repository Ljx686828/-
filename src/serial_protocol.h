/*
 * 串口通信协议 - ESP32端
 * Serial Communication Protocol for ESP32
 * 
 * 用于与1.85C-box显示终端通信
 */

#ifndef SERIAL_PROTOCOL_H
#define SERIAL_PROTOCOL_H

#include <Arduino.h>

// 数据包类型定义
#define PKT_TYPE_SYSTEM_STATUS  0x01
#define PKT_TYPE_TEMPERATURE    0x02
#define PKT_TYPE_WATER_LEVEL    0x03
#define PKT_TYPE_FLOW_RATE      0x04
#define PKT_TYPE_PUMP_STATUS    0x05

// 命令类型定义
#define CMD_MANUAL_PUMP_START   0x10
#define CMD_PUMP_STOP           0x11
#define CMD_SET_TIDAL_TIME      0x20

// 数据包头部和尾部
#define PKT_HEADER_1            0xAA
#define PKT_HEADER_2            0x55
#define PKT_END_1               0x0D
#define PKT_END_2               0x0A

// 命令包头部
#define CMD_HEADER_1            0xBB
#define CMD_HEADER_2            0x66

// 串口配置
#define SERIAL2_BAUD            115200
#define SERIAL2_RX_PIN          16  // 可配置
#define SERIAL2_TX_PIN          17  // 可配置

// 系统状态结构
struct SystemStatus {
  bool waterLevelSafe;
  bool tempAlarm;
  bool pumpRunning;
  bool manualModeActive;
  bool tidalModeActive;
  uint8_t currentPriority;  // 1-4
};

// 水泵状态结构
struct PumpStatus {
  bool running;
  uint8_t power;           // 0-255
  unsigned long remainingTime;  // 剩余时间(ms)
  bool isManual;
};

// 函数声明
void initSerialProtocol();
void sendSystemStatus(const SystemStatus& status);
void sendTemperature(float temp, bool valid);
void sendWaterLevel(bool safe);
void sendFlowRate(float rate);
void sendPumpStatus(const PumpStatus& status);
void checkAndParseCommands();
bool parseCommand(uint8_t* buffer, int len);
void handleManualPumpCommand();
void handleStopPumpCommand();
void handleSetTidalTimeCommand(uint8_t* params, int len);

// 辅助函数
uint8_t calculateChecksum(uint8_t* data, int len);

#endif // SERIAL_PROTOCOL_H

