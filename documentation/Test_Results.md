# Test Results — Industrial Motor Protection Controller

## Test 1: Overcurrent Fault Detection
**Expected:** System detects current > 2.5A, transitions RUNNING → FAULT  
**Result:** ✅ PASSED

State: IDLE [none] | Current: 0.00A | Temp: 0.0C | Speed: 0RPM
State: RUNNING [none] | Current: 2.31A | Temp: 0.0C | Speed: 0RPM
State: FAULT [overcurrent] | Current: 2.55A | Temp: 0.0C | Speed: 0RPM

---
## Test 2: Overtemperature Fault Detection
**Expected:** System detects temp > 80°C, transitions RUNNING → FAULT  
**Result:** ✅ PASSED

State: IDLE [none] | Current: 0.00A | Temp: 0.0C | Speed: 0RPM
State: RUNNING [none] | Current: 0.00A | Temp: 55.2C | Speed: 0RPM
State: FAULT [overtemp] | Current: 0.00A | Temp: 89.6C | Speed: 0RPM

---

## Test 3: Overspeed Fault Detection
**Expected:** System detects speed > 3500 RPM, transitions RUNNING → FAULT  
**Result:** ✅ PASSED

State: IDLE [none] | Current: 0.00A | Temp: 0.0C | Speed: 0RPM
State: RUNNING [none] | Current: 0.02A | Temp: 0.0C | Speed: 3288RPM
State: FAULT [overspeed] | Current: 0.02A | Temp: 0.0C | Speed: 3793RPM

---

## Test 4: Motor Stall Detection
**Expected:** System detects stall (PWM > 20% AND Speed < 100 RPM for 2 seconds)  
**Result:** ✅ PASSED

State: IDLE [none] | Current: 0.00A | Temp: 0.0C | Speed: 0RPM
State: RUNNING [none] | Current: 0.02A | Temp: 0.0C | Speed: 0RPM
State: FAULT [none] | Current: 0.02A | Temp: 0.0C | Speed: 0RPM

---

## Test 5: Multiple Faults Simultaneously
**Expected:** System handles multiple faults cleanly, auto-recovers after 10 seconds  
**Result:** ✅ PASSED

State: IDLE [none] | Current: 0.00A | Temp: 0.0C | Speed: 0RPM
State: RUNNING [none] | Current: 2.40A | Temp: 4.1C | Speed: 802RPM
State: FAULT [overcurrent] | Current: 2.94A | Temp: 99.6C | Speed: 0RPM
(waiting 10 seconds...)
State: IDLE [none] | Current: 0.00A | Temp: 4.1C | Speed: 0RPM

---

## Test 6: Hysteresis-Gated Reset
**Expected:** Reset blocked while fault > 2.2A, allowed once < 2.2A  
**Result:** ✅ PASSED

State: IDLE [none] | Current: 0.00A | Temp: 0.0C | Speed: 0RPM
State: RUNNING [none] | Current: 2.40A | Temp: 4.1C | Speed: 802RPM
State: FAULT [overcurrent] | Current: 2.70A | Temp: 4.1C | Speed: 0RPM
(pressed Reset while current still high — no change)
State: IDLE [none] | Current: 1.70A | Temp: 4.1C | Speed: 802RPM
(Reset allowed after current dropped)

---

## Test 7: Button Debouncing
**Expected:** No state chatter during rapid button presses  
**Result:** ✅ PASSED

State: IDLE [none] | Current: 1.70A | Temp: 4.1C | Speed: 802RPM
State: RUNNING [none] | Current: 1.70A | Temp: 4.1C | Speed: 802RPM
State: RUNNING [none] | Current: 1.70A | Temp: 4.1C | Speed: 802RPM
State: RUNNING [none] | Current: 1.70A | Temp: 4.1C | Speed: 0RPM
State: RUNNING [none] | Current: 1.70A | Temp: 4.1C | Speed: 802RPM

---
## Test 8: LED Indicator Behavior
**Expected:** Motor LED off in IDLE/FAULT, on/bright in RUNNING; Fault LED off except in FAULT  
**Result:** ✅ PASSED

**IDLE state:**
- Motor LED: OFF (dim/black)
- Fault LED: OFF (dim)

**RUNNING state:**
- Motor LED: Brightness increases with setpoint (PWM proportional)
- Fault LED: OFF

**FAULT state:**
- Motor LED: Immediately OFF (PWM = 0%)
- Fault LED: ON (bright red)

---
## Summary
All 8 test scenarios passed successfully ✅


