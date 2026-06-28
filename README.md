# Project_QCC744M_AbhiRam
A full-stack embedded integration project for the QCC744M EVK. This repository contains driver implementations for I2C displays (OLED/LCD) and sensors (DHT11/MQ-135), alongside a self-healing SPI Master-Slave communication protocol. Features automatic RTC recovery.
# QCC744M Integrated Environmental Station & SPI Bridge

![Language](https://img.shields.io/badge/Language-C-00599C.svg)
![Platform](https://img.shields.io/badge/Platform-QCC744M%20EVK-orange)
![Protocol](https://img.shields.io/badge/Protocol-SPI%20%2B%20I2C-green)

A comprehensive "Bare Metal" style embedded project for the QCC744M (BL616) EVK. This system integrates environmental sensing, precise timekeeping, dual-display outputs, and inter-board communication into a single, robust application.

## 📋 Features

* **Multi-Sensor Fusion:** Reads Temperature, Humidity (DHT11), and Air Quality/CO2 (MQ-135).
* **Timekeeping:** Interfaces with DS1307 RTC. Includes an **Auto-Recovery** feature that sets the time to the compiler timestamp if the RTC battery fails or resets.
* **Visual Output:** * Drives a **1.3" OLED** for detailed status text.
* **Telemetry (SPI Bridge):** Acts as an SPI Master, transmitting formatted data packets to a secondary Slave board for remote monitoring.

## 🛠 Hardware Configuration

### Pinout Map (Master Board)

| Component | Pin Name | GPIO Pin | Bus / Function |
| :--- | :--- | :--- | :--- |
| **SPI Interface** | **CS (Chip Select)** | **GPIO 0** | Software Controlled |
| | **SCK (Clock)** | **GPIO 1** | SPI Function 4 |
| | **MISO (RX)** | **GPIO 2** | SPI Function 4 |
| | **MOSI (TX)** | **GPIO 3** | SPI Function 4 |
| **Sensors** | **DHT11 Data** | **GPIO 10** | 1-Wire Protocol |
| | **MQ-135 Aout** | **GPIO 19** | ADC Channel 1 |
| **I2C Bus** | **SDA** | **GPIO 11** | I2C0 |
| | **SCL** | **GPIO 14** | I2C0 |
| **I2C Devices** | OLED Display | Addr `0x3C` | On I2C Bus |
| | DS1307 RTC | Addr `0x68` | On I2C Bus |
| | 16x2 LCD | Addr `0x27` | On I2C Bus |

### System Diagram
<img width="1536" height="823" alt="image" src="https://github.com/user-attachments/assets/e0cd7303-24c5-4ab1-8ece-0af423c4a5ab" />
