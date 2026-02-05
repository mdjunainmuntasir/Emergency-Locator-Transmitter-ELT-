# Emergency-Locator-Transmitter-ELT-
#Overview

This project implements an end-to-end IoT emergency location tracking system using ESP32 microcontrollers, LoRa (915 MHz), and GPS. The system is designed for low-power operation and real-time cloud connectivity, enabling reliable location transmission during emergency events.

#System Architecture

Briefly describe how the system is divided into two main parts:
Transmitter node (ESP32 + GPS + LoRa)
Receiver gateway (ESP32 + LoRa + Wi-Fi + Cloud)
Include a block diagram image here if available.

#Features

Low-power embedded firmware using ESP32 deep sleep
Hardware interrupt-based emergency activation
MOSFET-based GPS power control
Long-range LoRa wireless communication (915 MHz)
Binary payload transmission for efficiency
Wi-Fi gateway with REST API cloud integration
Real-time location visualization on cloud dashboards

#Hardware Components

ESP32 microcontroller (Transmitter and Receiver)
LoRa module (915 MHz)
GPS module
Emergency push button
P-MOSFET for power gating
Battery power supply
Software Stack

#Programming language: C++ (Arduino framework)

#Wireless protocols: LoRa, Wi-Fi

#Cloud platforms: Adafruit IO, ThingSpeak

#Communication protocols: SPI, UART, HTTP REST APIs

#Operating Modes

Deep Sleep (ARM mode): system idle with ultra-low power consumption
Emergency Mode: GPS powered on and location transmitted periodically
Gateway Mode: LoRa reception and cloud data forwarding

#Data Flow

Device remains in deep sleep until emergency button is pressed
GPS module is powered on and acquires location data
Location data is transmitted via LoRa every few seconds
Receiver gateway forwards data to cloud platforms via Wi-Fi
Live location is displayed on a cloud dashboard
