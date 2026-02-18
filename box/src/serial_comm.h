/*
 * 串口通信模块 - 1.85C-box端
 * Serial Communication Module for 1.85C-box
 * 
 * 用于与ESP32系统通信
 */

#ifndef SERIAL_COMM_H
#define SERIAL_COMM_H

#include <Arduino.h>

// 数据包类型定义（与ESP32端一致）
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

// 串口配置（使用GPIO43/44或Serial2）
#define SERIAL_COMM_BAUD        115200
#define SERIAL_COMM_RX_PIN      43
#define SERIAL_COMM_TX_PIN      44

// 系统状态结构
struct SystemData {
  bool waterLevelSafe;
  bool tempAlarm;
  bool pumpRunning;
  bool manualModeActive;
  bool tidalModeActive;
  uint8_t currentPriority;
  
  float temperature;
  bool tempValid;
  
  float flowRate;
  
  uint8_t pumpPower;
  unsigned long pumpRemainingTime;
  bool pumpIsManual;
  
  bool dataUpdated;
  unsigned long lastUpdateTime;
};

// 函数声明
void initSerialComm();
void updateSerialComm();
bool receiveData();
bool parseStatusPacket(uint8_t* buffer, int len);
void updateSystemData(uint8_t type, uint8_t* data, int len);
void sendManualPumpCommand();
void sendStopPumpCommand();
void sendSetTidalTimeCommand(unsigned long onTime, unsigned long offTime);

// 全局系统数据
extern SystemData systemData;

// 辅助函数
uint8_t calculateChecksum(uint8_t* data, int len);

#endif // SERIAL_COMM_H

