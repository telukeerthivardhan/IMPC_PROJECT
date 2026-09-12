# Design Decisions — Industrial Motor Protection Controller

## 1. State Machine Architecture

**Why this design?**
We implemented a three-state finite state machine (IDLE → RUNNING → FAULT) to maintain simplicity and clarity in system behavior. With only three states, the control logic is straightforward and easy to understand, making it simple to verify whether the IMPC is working correctly. This minimal architecture avoids unnecessary complexity while providing complete coverage of all operating modes: idle waiting state, active motor control, and protected fault mode. The clear node-to-node transitions make the system behavior transparent and verifiable.

---

## 2. Fault Threshold Selection

### Overcurrent: 2.5A Trip / 2.2A Clear
**Reasoning:** 
We modeled this project on a small-scale motor system with a maximum sensor reading of approximately 3 amps. We set the overcurrent trip threshold at 2.5A (roughly 83% of maximum rated current) to provide a safety margin before damage occurs, and the clear threshold at 2.2A to enable hysteresis-based recovery. This 0.3A gap prevents rapid chatter around the threshold.

### Overtemperature: 80°C Trip / 75°C Clear
**Reasoning:** 
Using the same margin-based approach, we set overtemperature trip at 80°C (a typical motor winding insulation limit in small industrial motors) with clear threshold at 75°C. The 5°C hysteresis margin ensures the system won't oscillate between fault and recovery if temperature hovers near the threshold.

### Overspeed: 3500 RPM Trip / 3300 RPM Clear
**Reasoning:** 
We selected 3500 RPM as a realistic motor redline speed for our small motor model, with 3300 RPM as the recovery threshold. The 200 RPM gap (roughly 6% hysteresis band) provides stable protection against hunting behavior.

### Motor Stall: PWM > 20% AND Speed < 100 RPM for 2 seconds
**Reasoning:** 
We require both a PWM command (> 20%, indicating genuine motor-on demand) and a speed condition (< 100 RPM for 2+ seconds) to detect stall. The 2-second window prevents false triggers during normal motor startup, while 20% PWM threshold ensures we only check stall when the motor is actually being commanded to run.

---

## 3. Hysteresis Implementation

**Why hysteresis instead of single threshold?**
Without hysteresis, a sensor reading oscillating around 2.49–2.51A would cause the system to rapidly chatter between fault and normal operation (RUNNING → FAULT → RUNNING → FAULT...). Hysteresis solves this by using two thresholds: a trip threshold (2.5A) and a lower recovery threshold (2.2A). The motor must cross back below the recovery threshold before reset is allowed, creating a deliberate margin that stabilizes the system and prevents false recoveries. This is a standard industrial control technique that improves robustness.

---

## 4. Closed-Loop vs Open-Loop PWM Control

**Current Implementation:** Open-loop (PWM directly proportional to speed setpoint)

**Why simplified for simulation?** 
In this simulation, speed feedback is just a potentiometer that you manually turn — it doesn't automatically respond to PWM changes like a real motor would. A true closed-loop controller compares the target setpoint against actual measured speed to calculate an error, then continuously adjusts PWM to minimize that error. Since our "motor" doesn't actually respond to commands, closed-loop correction would have nothing meaningful to correct against. Open-loop is honest and appropriate for this simulation.

**Production Approach:**
A real motor protection controller would use closed-loop control: read setpoint and actual speed from a real tachometer, calculate speed error, apply proportional/integral/derivative (PID) corrections to PWM, continuously driving actual speed toward the target. This provides stable speed regulation and better transient response.

---

## 5. Auto-Recovery Timer (10 seconds)

**Why 10 seconds?**
We selected 10 seconds as a reasonable stabilization window. For overtemperature faults, the motor windings need time to cool down after being shut off — 6–7 seconds is too short for meaningful thermal decay. Ten seconds gives the system adequate time to recover to safe operating conditions without being excessively conservative. This duration balances responsiveness against realistic thermal dynamics.

---

## Known Limitations & Design Tradeoffs

### Speed Feedback Simplification
**Limitation:** Speed feedback comes from a manual potentiometer, not an automatic tachometer  
**Why:** No physical motor exists in the simulation  
**Production Reality:** Real systems use tachometer or encoder feedback that automatically reports actual motor speed

### 500ms Loop Delay
**Limitation:** Main control loop runs only twice per second (500ms between cycles), making system response feel sluggish  
**Why:** Conservative choice for initial development; easier to debug at slower speed  
**Production Optimization:** Real controllers typically run at 50–100ms loop rates for responsive control

### Open-Loop Motor Control
**Limitation:** PWM directly follows setpoint; doesn't adjust based on actual speed feedback  
**Why:** Without a real motor responding, closed-loop error correction has nothing meaningful to act on  
**Production Reality:** Real systems implement PID or other feedback controllers to maintain target speed despite load changes

### Simulation-Only Implementation
**Limitation:** No real motor hardware, no actual current draw, no thermal dynamics simulation  
**Why:** Capstone project scope; demonstrates embedded systems concepts without requiring physical hardware  
**Production Reality:** Real industrial controllers interface with actual motor drivers, current sensors, and thermal management systems

---

## Implementation Highlights

**Edge Detection on Buttons:**  
Buttons use rising/falling edge detection rather than simple level reads, preventing accidental re-triggers from button bounce and ensuring one press = one state change.

**Fault Latching:**  
Faults latch immediately upon detection and require explicit manual reset (with condition clearing via hysteresis). This prevents unintended auto-recovery that could mask underlying problems.

**Multi-Fault Handling:**  
When multiple faults occur simultaneously, the system reports and latches the first detected fault. All conditions must clear (drop below respective hysteresis thresholds) before reset is permitted, ensuring comprehensive fault clearing before resuming operation.

