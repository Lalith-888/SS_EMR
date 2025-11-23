# SS-EMR Smart Room Infrastructure (ESP8266 IoT System)

## 📌 Project Overview

The **SS-EMR Smart Room Infrastructure** is an IoT-based automation system built using an **ESP8266 (NodeMCU)** that monitors and controls environmental conditions inside a room. It integrates multiple sensors, actuators, cloud connectivity (Blynk IoT), and a locally hosted real-time dashboard via WebSockets.

The system is designed to:

* Monitor **temperature, humidity, air quality, motion, flame, and occupancy**
* Automatically control a **fan (via servo)** and **buzzer alarm**
* Provide **manual and automatic modes** of operation
* Display real-time data on:

  * **Blynk cloud mobile app**
  * **Local Web Dashboard** (ESP8266-hosted)
* Use **TinyML (rule-based placeholder)** to detect anomalies

This document explains the **full technical workings** of the system.

---

## 🧠 System Architecture

### The system consists of the following subsystems:

### **1. Sensor Layer**

| Sensor       | Purpose                       | Pin    |
| ------------ | ----------------------------- | ------ |
| DHT11        | Temperature + Humidity        | D6     |
| MQ135        | Air Quality Index (analog)    | A0     |
| PIR          | Motion detection              | D7     |
| Flame Sensor | Fire detection                | D8     |
| IR Sensors   | Bidirectional people counting | D3, D4 |

### **2. Processing Layer (ESP8266)**

NodeMCU acts as:

* Sensor fusion processor
* Control decision-maker
* Web server (HTTP + WebSocket)
* Cloud device using Blynk
* TinyML analyzer

### **3. Actuation Layer**

| Actuator    | Purpose                         | Pin |
| ----------- | ------------------------------- | --- |
| Servo Motor | Fan substitute (speed by angle) | D5  |
| Buzzer      | Alarm for flame/TinyML          | D0  |

### **4. Network & Interface Layer**

* Local Dashboard (HTML + JS + WebSockets)
* Blynk Cloud Dashboard (mobile)
* Serial debugging (USB)

---

## 🌐 Communication Architecture

### **1. HTTP Server (Port 80)**

* Serves the mobile-friendly control dashboard
* Provides `/status` endpoint returning sensor data (JSON)

### **2. WebSocket Server (Port 81)**

Used for:

* Pushing real-time sensor updates every second
* Receiving instant commands from UI (fan ON, servo OPEN, buzzer ON, etc.)
* Logging user actions

This enables **low-latency two-way communication**.

### **3. Blynk Cloud Communication**

The device connects to **Blynk IoT** using:

* Template ID
* Device Name
* Authentication Token

Sensor values are sent every second to datastreams:

* V0 → Temperature
* V1 → Humidity
* V2 → Air Quality (AQI)
* V3 → Occupancy
* V4 → Motion
* V5 → Flame
* V11 → Fan ON/OFF
* V12 → Buzzer ON/OFF
* V30 → Auto Mode

This provides cloud monitoring + phone app control.

---

## 🔧 Sensor Processing & Logic Flow

### **1. Temperature & Air Quality Control**

* If `temp ≥ 30°C` OR `AQI ≥ 400` → Fan turns ON
* Else → Fan turns OFF

### **2. Motion-Based Light/Fan Automation**

* PIR detects motion
* Maintains a **timestamp** of last activity
* If no motion for 60 seconds → auto turn OFF light (if light was implemented)

### **3. Flame Detection Logic**

If flame sensor detects fire:

* Buzzer ON
* Auto shutdown of fan (safety)
* Blynk sends notification event

### **4. Occupancy Detection (IR Sensors)**

Two IR sensors detect direction of movement:

* Person entering: IR1 triggers before IR2 → `occupancy++`
* Person exiting: IR2 triggers before IR1 → `occupancy--`

Occupancy never goes below zero.

### **5. Auto vs Manual Mode**

* Auto mode runs full automation
* Manual mode lets user directly control fan & buzzer
* Switching modes updates Blynk and WebSocket dashboard

---

## 🤖 TinyML Integration (Rule-Based Placeholder)

Currently, TinyML block is rule-based but structured for future model deployment.

### Trigger Conditions:

* Flame detected → anomaly
* AQI > 900 → anomaly
* Temp > 80°C → anomaly

On anomaly:

* Buzzer ON
* Blynk event triggered
* Dashboard warning displayed

This allows easy replacement with an actual TFLite model later.

---

## 📱 Local Web Dashboard (ESP8266 Hosted)

Written in:

* HTML + CSS (mobile-optimized)
* JavaScript (WebSocket API)

### Features:

* Live temp/humidity/AQI
* Motion / flame / occupancy
* Auto toggle
* Fan/Buzzer/Servo controls
* Real-time log system

WebSocket ensures **instant UI updates** without refreshing.

---

## 📱 Blynk Mobile Dashboard

The Blynk app provides:

* Professional cloud dashboard
* History graph widgets
* LED widgets for flame/motion
* Switch controls for fan, buzzer, servo
* Auto-mode switch
* Notification alerts for fire/anomaly

---

## ⚙️ Power & Pin Considerations

### ⚠ Boot Pins

Some ESP8266 pins affect boot:

* D3 (GPIO0) → must be HIGH on boot
* D4 (GPIO2) → must be HIGH on boot
* D8 (GPIO15) → must be LOW on boot

Therefore:

* IR sensors & flame sensor **must be connected AFTER uploading**.

This prevents bootloader lock.

---

## 🛠 Reliability Features

* Watchdog-like timed tasks (via BlynkTimer)
* Non-blocking sensor logic
* Automatic reconnection attempts
* JSON optimization for low RAM
* Error handling for NaN from DHT11

---

## 📡 Memory & Performance

The code uses:

* IRAM for WebSocket + WiFi ISR code
* Flash for HTML dashboard
* Stack for sensor processing

Optimized to prevent:

* Heap fragmentation
* Blocking delays
* WiFi timeouts

---

## 📓 Summary

This project integrates:

* **IoT automation**
* **Real-time dashboards**
* **Cloud control**
* **Local control**
* **Sensor fusion**
* **TinyML anomaly logic**

It is a complete **Smart Room Automation System**, fully deployable and scalable.

