# 智能鱼菜共生系统 - 屏幕UI设计
# Smart Aquaponics System - Screen UI Design

## 📐 UI Layout Design (480x272 pixels)

This document describes the UI design for the ATF UART Serial Screen (Nextion/TJC compatible).

### Screen Layout

```
┌────────────────────────────────────────────────────────────────┐
│                                                                │
│    ╔════════════════════════════════════════════════════╗     │
│    ║     智能鱼菜共生 Smart Aquaponics    [t0]          ║     │
│    ╚════════════════════════════════════════════════════╝     │
│                                                                │
│    ┌──────────────────┐      ┌──────────────────┐             │
│    │   水温 TEMP      │      │   水位 LEVEL     │             │
│    │   [tTemp]        │      │   [tLevel]       │             │
│    │                  │      │                  │             │
│    │    25.3°C        │      │    安全 SAFE     │             │
│    │    [t1]          │      │    [t2]          │             │
│    │                  │      │                  │             │
│    └──────────────────┘      └──────────────────┘             │
│                                                                │
│    ┌─────────────────────────────────────────────┐            │
│    │            水泵状态 PUMP STATUS              │            │
│    │            [tPump]                           │            │
│    │                                              │            │
│    │              已停止 OFF                      │            │
│    │              [t3]                            │            │
│    └─────────────────────────────────────────────┘            │
│                                                                │
│    ╔═════════════════════════════════════════════╗            │
│    ║       启动水泵 START PUMP    [b0]           ║            │
│    ╚═════════════════════════════════════════════╝            │
│                                                                │
│    ┌─────────────────────────────────────────────┐            │
│    │  系统就绪 System Ready           [t4]       │            │
│    └─────────────────────────────────────────────┘            │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

## 🎨 Component Specifications

### Text Components

| ID | Name | Position (x,y) | Size (w×h) | Font | Description |
|----|------|----------------|------------|------|-------------|
| t0 | Header | (20, 10) | 440×40 | Font 2 (32pt) | Title: "智能鱼菜共生 Smart Aquaponics" |
| tTemp | Temp Label | (20, 60) | 200×25 | Font 1 (24pt) | Label: "水温 TEMP" |
| t1 | Temp Value | (20, 90) | 200×50 | Font 3 (48pt) | Temperature: "25.3°C" |
| tLevel | Level Label | (250, 60) | 200×25 | Font 1 (24pt) | Label: "水位 LEVEL" |
| t2 | Level Value | (250, 90) | 200×50 | Font 2 (32pt) | Status: "安全 SAFE" |
| tPump | Pump Label | (20, 150) | 440×25 | Font 1 (24pt) | Label: "水泵状态 PUMP STATUS" |
| t3 | Pump Status | (20, 180) | 440×40 | Font 2 (32pt) | Status: "已停止 OFF" |
| t4 | System Msg | (20, 240) | 440×25 | Font 1 (24pt) | Messages & Alerts |

### Button Components (for Touch Screen)

| ID | Name | Position (x,y) | Size (w×h) | Description |
|----|------|----------------|------------|-------------|
| b0 | Pump Button | (100, 220) | 280×45 | "启动水泵 START PUMP" |

### Color Scheme (RGB565)

| Color | RGB565 Value | Usage |
|-------|--------------|-------|
| Red | 63488 | Danger, Alarm, High Temp |
| Green | 2016 | Safe, Normal, Optimal |
| Blue | 31 | Information |
| Orange | 64512 | Warning, Attention |
| Cyan | 2047 | Pump Running |
| White | 65535 | Default Text |
| Black | 0 | Background |
| Dark Blue | 1024 | Header Background |

## 📱 Touch Screen Upgrade Guide

When upgrading from non-touch to touch screen:

### 1. Button Touch Event Setup

In Nextion Editor, set the button `b0` touch event to send:
```
printh 65 00 01 01
```

This sends: `0x65 0x00 0x01 0x01 0xFF 0xFF 0xFF`
- 0x65: Touch event
- 0x00: Page ID
- 0x01: Component ID (b0)
- 0x01: Press event

### 2. ESP32 Code Addition

Add this function to handle touch events:

```cpp
void checkDisplayTouch() {
  while (Serial2.available() >= 7) {
    if (Serial2.read() == 0x65) {
      uint8_t pageId = Serial2.read();
      uint8_t componentId = Serial2.read();
      uint8_t event = Serial2.read();
      
      // Clear end markers
      Serial2.read(); Serial2.read(); Serial2.read();
      
      // Handle b0 button press
      if (componentId == 1 && event == 1) {
        handlePumpButtonPress();
      }
    }
  }
}

void handlePumpButtonPress() {
  if (waterLevelSafe && !pumpRunning) {
    digitalWrite(PIN_WATER_PUMP, HIGH);
    pumpRunning = true;
    pumpStartTime = millis();
    currentState = STATE_PUMP_RUNNING;
    updatePumpDisplay(true, 5);
    updateSystemMessage("水泵运行中 Pump Running", COLOR_CYAN);
  }
}
```

### 3. Add to loop()

```cpp
void loop() {
  checkDisplayTouch();  // Add this line
  checkWaterLevel();
  // ... rest of loop
}
```

## 🖼️ Visual Design Guidelines

### Font Recommendations

For Chinese + English bilingual display:
- **Primary Font**: 文泉驿微米黑 (WenQuanYi Micro Hei) or similar
- **Fallback**: SimHei, Microsoft YaHei
- **Size Hierarchy**:
  - Title: 32pt
  - Values: 48pt (temperature), 32pt (status)
  - Labels: 24pt
  - Messages: 24pt

### Animation Suggestions (for Touch Screen)

1. **Button Press**: Brief color inversion on touch
2. **Alarm State**: Blinking text (use timer component)
3. **Pump Running**: Progress bar animation
4. **Temperature Change**: Smooth color transition

### Responsive States

| State | Background | Text Color | Animation |
|-------|------------|------------|-----------|
| Normal | Dark Blue | White/Green | None |
| Warning | Dark Blue | Orange | Slow blink |
| Alarm | Red | White | Fast blink |
| Pump On | Dark Blue | Cyan | Pulse |

## 📁 Nextion/TJC Project Files

### File Structure

```
ui/
├── README.md           # This file
├── aquaponics.HMI      # Nextion Editor project file
├── aquaponics.tft      # Compiled firmware for screen
├── fonts/
│   ├── font_24.zi      # 24pt Chinese font
│   ├── font_32.zi      # 32pt Chinese font
│   └── font_48.zi      # 48pt Chinese font
└── images/
    ├── bg_normal.png   # Normal background
    ├── bg_alarm.png    # Alarm background
    ├── icon_temp.png   # Temperature icon
    ├── icon_water.png  # Water level icon
    └── icon_pump.png   # Pump icon
```

### Creating the HMI File

1. Open Nextion Editor
2. Create new project: 480×272, Horizontal
3. Add components as specified above
4. Import Chinese fonts
5. Set touch events for b0
6. Compile and upload to screen

## 🔧 Troubleshooting

### Display Not Updating

1. Check baud rate (9600)
2. Verify TX/RX connections
3. Check end markers (0xFF 0xFF 0xFF)

### Chinese Characters Not Showing

1. Ensure Chinese font is imported in Nextion Editor
2. Set correct font ID for text components
3. Use UTF-8 encoding in code

### Touch Not Responding

1. Verify touch event is configured in Nextion Editor
2. Check Serial2.available() in code
3. Verify component ID matches code

## 📝 Notes for Educational Use

### Teaching Points

1. **Variables**: Temperature, water level, pump state
2. **Conditions**: if-else for safety checks
3. **Events**: Touch sensor triggers
4. **Timing**: millis() for non-blocking delays
5. **Communication**: Serial protocol with display

### Suggested Exercises

1. Change temperature warning threshold
2. Modify pump run time
3. Add new status messages
4. Create custom alarm sounds (with buzzer)
5. Log data to SD card

