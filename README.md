# 🔵 SchreinBluetoothManager

**Advanced Bluetooth Management Library for Arduino**  
*Robust connectivity with automatic retry mechanisms and smart state management for HC-05/HC-06 modules*

[![Arduino Library](https://img.shields.io/badge/Arduino_Library-✓-blue?logo=arduino)](https://www.arduino.cc/)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/Version-1.0.0-orange)](https://github.com/SchreikenLab/SchreinBluetoothManager/releases)
[![Documentation](https://img.shields.io/badge/Docs-📖-blue)](https://github.com/SchreikenLab/SchreinBluetoothManager/wiki)

## ✨ Features

| Feature | Description |
|---------|-------------|
| 🤖 **Dual Mode Operation** | Client (Master) & Server (Slave) modes |
| 🔄 **Smart Retry System** | Automatic retry for connections & data transmission |
| 📊 **Real-time State Management** | 5 connection states with callback support |
| 🎯 **Event-Driven Architecture** | Comprehensive callback system for all events |
| ⏰ **Exponential Backoff** | Intelligent retry timing algorithm |
| 📱 **Module Auto-Discovery** | Automatic MAC address, name, and PIN retrieval |
| ⚡ **Non-Blocking Design** | Fully asynchronous operation |
| 🔧 **HC-05/HC-06 Optimized** | Perfect for popular Bluetooth modules |

## 🚀 Quick Installation

### Arduino Library Manager
1. Open Arduino IDE
2. Go to **Sketch** → **Include Library** → **Manage Libraries...**
3. Search for "SchreinBluetoothManager"
4. Click **Install**

### Manual Installation
```bash
# Clone the repository
git clone https://github.com/SchreikenLab/SchreinBluetoothManager.git

# Move to Arduino libraries folder
mv SchreinBluetoothManager ~/Documents/Arduino/libraries/
