# Industrial Motor Protection Controller (IMPC)

A real-time embedded motor protection system implemented on STM32F103C8T6 (Blue Pill) that detects and responds to overcurrent, overtemperature, overspeed, and motor stall conditions.

## Features

- **4-Fault Detection:** Overcurrent (2.5A), Overtemperature (80°C), Overspeed (3500 RPM), Motor Stall (2-second window)
- **State Machine Control:** IDLE → RUNNING → FAULT with hysteresis-based recovery
- **Automatic Shutdown:** PWM forced to 0% when fault detected; motor LED turns off
- **Hysteresis Protection:** Prevents chattering at threshold boundaries (e.g., 2.5A trip, 2.2A clear)
- **10-Second Auto-Recovery:** System automatically returns to IDLE once all fault conditions clear
- **Real-Time Diagnostics:** UART serial output at 115200 baud showing state, fault reason, and all sensor readings
- **LED Indicators:** Motor speed LED (brightness ∝ PWM), Fault LED (RED when faulted)

## Hardware

- **Microcontroller:** STM32F103C8T6 (Blue Pill)
- **Simulation:** Wokwi (browser-based STM32 simulator)
- **Inputs (ADC):** Speed Setpoint (PA0), Speed Feedback (PA1), Current Sense (PA2), Temperature (PA3)
- **Buttons:** Start/Stop (PB1), Reset (PB10)
- **Outputs:** PWM Motor Control (PA6), Fault LED (PB0), UART Diagnostics (PA9)

## Quick Start

### Prerequisites
- STM32CubeIDE (free download)
- VS Code + Wokwi extension (for simulation)

### Build Firmware
1. Open IMPC_PROJECT.ioc in STM32CubeMX
2. Generate code
3. Build in STM32CubeIDE: Project → Build

### Run Simulation
1. Open VS Code with this project folder
2. Install Wokwi extension
3. Open wokwi.toml and run simulation
4. Watch serial monitor at 115200 baud

## Project Structure
IMPC_PROJECT/
├── firmware/ # C source code and configuration
│ ├── main.c # Core firmware logic
│ └── wokwi.toml # Wokwi simulator config
├── hardware/ # Circuit design
│ └── diagram.json # Wokwi circuit schematic
└── documentation/ # Design & test docs
├── Design_Decisions.md # Engineering rationale
├── Test_Results.md # All 8 test scenarios
├── Architecture_Diagram.png # Block diagram
├── StateMachine_Diagram.png # State transitions
└── Fault_Thresholds_Table.png # Fault parameters


## Test Results

All 8 test scenarios verified passing:
- ✅ Overcurrent fault detection
- ✅ Overtemperature fault detection
- ✅ Overspeed fault detection
- ✅ Motor stall detection
- ✅ Multiple simultaneous faults
- ✅ Hysteresis-gated reset
- ✅ Button debouncing
- ✅ LED indicators

See `documentation/Test_Results.md` for detailed logs.

## Design Highlights

- **State Machine:** Simple 3-state (IDLE/RUNNING/FAULT) for clarity and verifiability
- **Hysteresis:** 0.3A (current), 5°C (temp), 200 RPM (speed) margins prevent threshold chattering
- **Edge Detection:** Button debouncing eliminates false triggers
- **Fault Latching:** Faults stay latched until manual reset + condition clearing (safe-state design)
- **Auto-Recovery:** 10-second thermal cooldown window for realistic fault recovery

See `documentation/Design_Decisions.md` for full engineering rationale.

## Known Limitations

- **Speed Feedback:** Manual potentiometer (simulated), not real tachometer
- **Loop Delay:** 500ms cycle time (could optimize to 50ms for production)
- **Open-Loop Control:** PWM proportional to setpoint; no PID feedback correction
- **Simulation Only:** No real motor hardware or thermal dynamics

## Usage Example

1. Press **Start/Stop** button → System enters RUNNING
2. Turn **speed setpoint knob** → Motor LED brightness increases (PWM proportional)
3. Turn **current knob past 2.5A** → System detects overcurrent → FAULT state
   - Motor LED turns OFF
   - Fault LED turns ON (red)
   - UART prints `[overcurrent]`
4. Turn current knob back below 2.2A → Hysteresis clears
5. Press **Reset** button → System returns to IDLE
6. Wait 10 seconds (alternative to manual reset) → Auto-recovery to IDLE

## Building Locally

```bash
git clone https://github.com/telukeerthivardhan/IMPC_PROJECT.git
cd IMPC_PROJECT
# Open firmware/ in STM32CubeIDE
# Open in VS Code for Wokwi simulation
```

## Contact & Questions

For questions about this project, see `documentation/Design_Decisions.md` and `documentation/Test_Results.md`.

---

**Capstone Project** | STM32F103C8T6 | Embedded Systems | Motor Protection

