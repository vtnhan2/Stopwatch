# Stopwatch System for NUC140 MCU Board

## Overview
This project implements a stopwatch system with alarm functionality on the NUC140 MCU board. The system features multiple operating modes, 7-segment display output, and keypad input control.

## Features

- **Five operating modes**:
  - Idle mode (LED5 on)
  - Alarm Set mode (LED8 on)
  - Count mode (LED6 on)
  - Pause mode (LED7 on)
  - Check mode (displays lap times)

- **Precision timing**:
  - 1/10 second resolution using Timer0 interrupt
  - Configurable alarm time (00-59 seconds)

- **Lap time functionality**:
  - Stores up to 5 lap times
  - View recorded laps in Check mode

- **Buzzer notification**:
  - Triggers when elapsed time matches alarm time

## Hardware Requirements

- NUC140 MCU board
- 4-digit 7-segment display
- Keypad with buttons (K1, K3, K5, K9, GPB15)
- Buzzer
- LEDs (LED5-LED8)

## System Modes

### Idle Mode
- Default state after reset
- LED5 illuminated
- Displays "0000" on 7-segment
- Press K3 to enter Alarm Set mode

### Alarm Set Mode
- LED8 illuminated
- Set alarm time (00-59 seconds) using GPB15
- Press K3 to confirm and return to Idle mode

### Count Mode
- LED6 illuminated
- Stopwatch counts up with 1/10 second precision
- Press K1 to pause (enters Pause mode)
- Press K9 to record lap time (max 5 laps)
- Buzzer triggers when alarm time is reached

### Pause Mode
- LED7 illuminated
- Press K1 to resume counting
- Press K9 to reset to Idle mode
- Press K5 to enter Check mode

### Check Mode
- Displays recorded lap times
- Format: [Lap Number][Seconds]
- Press GPB15 to cycle through laps
- Press K5 to return to Pause mode

## Code Structure

- **Main components**:
  - System initialization (`System_Config`, `SetUp`)
  - Timer0 interrupt handler (`TMR0_IRQHandler`)
  - 7-segment display control (`segment_Set`, `segment_Show`)
  - Keypad scanning (`KeyPadEnable`, `KeyPadScanning`)
  - Mode handlers (`idleMode`, `alarmSetMode`, etc.)

- **Key variables**:
  - `currentState`: Tracks system mode (IDLE, ALARM_SET, etc.)
  - `alarmTime`: Stores configured alarm time
  - `elapsedTime`: Tracks stopwatch time (1/10 seconds)
  - `lapTimes`: Array storing up to 5 lap times

## Usage Instructions

1. Power on the system (starts in Idle mode)
2. Press K3 to set alarm time:
   - Use GPB15 to adjust time (00-59)
   - Press K3 to confirm
3. Press K1 to start stopwatch
4. During operation:
   - Press K1 to pause/resume
   - Press K9 to record lap time
5. When paused:
   - Press K5 to view lap times
   - Press K9 to reset to Idle mode

## Technical Details

- **Timer Configuration**:
  - Timer0 interrupt triggers every 0.1 second
  - System clock configured for 12MHz operation

- **Display**:
  - 7-segment display multiplexed across 4 digits
  - Custom patterns for digits 0-9

- **Debouncing**:
  - Simple delay-based debouncing for key inputs

## Customization

To modify system behavior:
1. Adjust `TIMER0->TCMPR` for different timing precision
2. Change `pattern[]` array for different 7-segment displays
3. Modify key mappings in `KeyPadScanning()`
4. Adjust alarm time range (currently 00-59 seconds)
