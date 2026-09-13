# Fall Detection System
An IoT based fall detection system using ESP32 and an MPU6050
# IoT-Based Fall Detection System

An automated wearable health monitoring and safety system developed using an **ESP32-WROOM** microcontroller and an **MPU6050** 6-axis IMU sensor mounted on a custom PCB. The system applies a multi-stage threshold algorithm to inertial movement data to detect falls in real time, triggering an immediate local buzzer alarm and transmitting event telemetry to the **ThingSpeak IoT Cloud** over Wi-Fi.

---

## Key Features

* **Autonomous Fall Detection:** Eliminates dependence on manual panic buttons by continuously processing acceleration and angular velocity vectors.
* **Multi-Stage Decision Algorithm:** Classifies movement into three distinct states—Free-Fall, Impact, and Posture/Orientation change—to reduce false positives from routine daily activities.
* **Immediate Local Warning:** Drives an onboard active buzzer to generate a high-decibel audible alarm for on-site assistance.
* **Remote IoT Telemetry:** Automatically sends acceleration, gyroscope, and event-flag metrics to ThingSpeak fields for remote caregiver monitoring.
* **Custom PCB Assembly:** Replaces standard prototyping jumper wires with a dedicated PCB to prevent physical impact shock from disconnecting components.

---

## Detection Algorithm & Calibration

The system samples 3-axis linear acceleration ($A_x, A_y, A_z$) and angular rate ($G_x, G_y, G_z$) to detect abnormal dynamic transitions:

| Stage | Monitored Metric | Calibrated Threshold | Event Action |
| :--- | :--- | :--- | :--- |
| **Stage 1: Free Fall** | Acceleration vector drop | `< 6.0` | Senses momentary weightlessness during descent |
| **Stage 2: Impact** | Acceleration vector spike | `> 14.0` | Detects abrupt collision with the ground |
| **Stage 3: Orientation** | Gyroscopic deviation | `10.0` – `300.0` | Confirms post-impact angle change vs. normal resting state |
| **Alert Dispatch** | Status flag verification | `fallStatus = 1` | Activates buzzer for 2 s and posts data packet to ThingSpeak API |

---

## Hardware Specifications

* **Microcontroller:** ESP32-WROOM (Dual-Core @ 240 MHz, integrated 2.4 GHz Wi-Fi)
* **Motion Sensor:** InvenSense MPU6050 (3-axis Accelerometer + 3-axis Gyroscope, I2C address `0x68`)
* **Local Alarm:** Active Buzzer (driven via GPIO 23)
* **PCB Interface:** Custom PCB routed for `SDA` and `SCL` lines with dedicated 3.3V power rails
* **Power:** Regulated 5V DC micro-USB input

---

## Circuit Connections

| Component | Pin | ESP32 GPIO | Description |
| :--- | :--- | :--- | :--- |
| **MPU6050** | SDA | GPIO 21 | I2C Data Line |
| **MPU6050** | SCL | GPIO 22 | I2C Clock Line |
| **MPU6050** | VCC / GND | 3.3V / GND | Power supply |
| **Active Buzzer** | Signal (+) | GPIO 23 | Trigger output |
| **Active Buzzer** | Ground (-) | GND | Ground reference |

---
