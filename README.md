<div align="center">
🔵 SchreinBluetoothManager
Advanced Bluetooth Management Library for Arduino
Robust connectivity with automatic retry mechanisms and smart state management

https://img.shields.io/badge/Arduino-Compatible-blue?logo=arduino
https://img.shields.io/badge/License-MIT-green.svg
https://img.shields.io/badge/Version-1.0.0-orange
https://img.shields.io/badge/Docs-Wiki-blue

</div>
✨ Features
Feature	Description
🤖 Dual Mode	Client (Master) & Server (Slave) operation
🔄 Smart Retry	Automatic retry system for connections & data
📊 State Management	Real-time connection state tracking
🎯 Event-Driven	Comprehensive callback system
⏰ Exponential Backoff	Intelligent retry timing algorithm
📱 Module Discovery	Auto-detection of MAC, name, and PIN
⚡ Non-Blocking	Fully asynchronous operation
🔧 HC-05/HC-06	Optimized for popular Bluetooth modules
🚀 Quick Start
Installation
Using Arduino Library Manager:

Open Arduino IDE

Sketch → Include Library → Manage Libraries...

Search for "SchreinBluetoothManager"

Click Install

Manual Installation:

bash
cd ~/Documents/Arduino/libraries/
git clone https://github.com/yourusername/SchreinBluetoothManager.git
Basic Usage
cpp
#include <SchreinBluetoothManager.h>

SoftwareSerial btSerial(10, 11);
SchreinBluetoothManager bt(btSerial, SchreinBluetoothManager::Mode::SERVER);

void setup() {
  bt.begin();
  bt.onConnect([]() { Serial.println("Connected!"); });
  bt.onDataReceived([](String data) { Serial.println("Received: " + data); });
}

void loop() {
  bt.update();
}
📋 Examples
🖥️ Bluetooth Server Example
https://img.shields.io/badge/View_Code-Example_1-8A2BE2

cpp
#include <SoftwareSerial.h>
#include <SchreinBluetoothManager.h>

SoftwareSerial btSerial(10, 11);
SchreinBluetoothManager bt(btSerial, SchreinBluetoothManager::Mode::SERVER);

void setup() {
  Serial.begin(9600);
  btSerial.begin(9600);
  
  bt.begin();
  bt.onConnect([]() {
    Serial.println("📱 Device connected!");
    Serial.println("Connected device: " + bt.getConnectedDeviceAddress());
  });
  
  bt.onDataReceived([](String data) {
    Serial.println("📨 Received: " + data);
    bt.sendRawData("Echo: " + data);
  });
  
  Serial.println("🔵 Server Started - Name: " + bt.getModuleName());
}

void loop() {
  bt.update();
}
📱 Bluetooth Client Example
https://img.shields.io/badge/View_Code-Example_2-8A2BE2

cpp
#include <SoftwareSerial.h>
#include <SchreinBluetoothManager.h>

SoftwareSerial btSerial(10, 11);
SchreinBluetoothManager bt(btSerial, SchreinBluetoothManager::Mode::CLIENT);

void setup() {
  Serial.begin(9600);
  btSerial.begin(9600);
  
  // Configure retry settings
  SchreinBluetoothManager::RetryConfig config;
  config.maxConnectionRetries = 5;
  config.connectionRetryDelay = 2000;
  bt.configureRetry(config);
  
  bt.begin();
  bt.connect("12:34:56:78:90:AB"); // Replace with server address
}

void loop() {
  bt.update();
  
  if (bt.isConnected() && Serial.available()) {
    String message = Serial.readString();
    bt.sendRawDataWithRetry(message);
  }
}
🔌 Hardware Setup
Wiring Diagram
bash
Arduino Uno/Nano    HC-05/HC-06 Module
───────────────────────────────────────
5V                 → VCC
GND                → GND
Digital Pin 10 (RX)→ TX
Digital Pin 11 (TX)→ RX (with voltage divider*)
* Use voltage divider for 5V→3.3V conversion on RX line

📚 API Reference
Core Methods
Method	Description
begin()	Initialize Bluetooth module
connect(address)	Connect to remote device
sendRawData(data)	Send data to connected device
update()	Main update function (call in loop)
getConnectionState()	Get current connection state
Callbacks
cpp
// Connection events
bt.onConnect([]() { /* Connected */ });
bt.onDisconnect([]() { /* Disconnected */ });

// Data events  
bt.onDataReceived([](String data) { /* Data received */ });

// Error handling
bt.onError([](String error) { /* Error occurred */ });

// Retry events
bt.onRetryAttempt([](uint8 attempt, uint8 max) { /* Retry started */ });
⚙️ Configuration
Retry Configuration
cpp
SchreinBluetoothManager::RetryConfig config;
config.enableConnectionRetry = true;
config.maxConnectionRetries = 5;
config.connectionRetryDelay = 2000;
config.useExponentialBackoff = true;
config.backoffMultiplier = 2.0;
config.maxBackoffDelay = 30000;

bt.configureRetry(config);
🐛 Troubleshooting
Common Issues
Module not responding

Check wiring and power supply

Verify baud rate settings

Connection failures

Ensure devices are paired

Confirm MAC address format

Data corruption

Add voltage divider for 5V→3.3V conversion

Check for electrical noise

Debug Mode
Enable serial debugging for detailed logs:

cpp
Serial.begin(9600);
while (!Serial); // Wait for serial connection
// Debug messages will appear here
🤝 Contributing
We welcome contributions! Please see our contributing guidelines:

Fork the repository

Create a feature branch (git checkout -b feature/amazing-feature)

Commit changes (git commit -m 'Add amazing feature')

Push to branch (git push origin feature/amazing-feature)

Open a Pull Request

📜 License
This project is licensed under the MIT License - see the LICENSE file for details.

📞 Support
📖 Documentation Wiki

🐛 Report Issues

💬 Discussions

📧 Email: your.email@example.com

🙏 Acknowledgments
Built for the Arduino community

Compatible with HC-05/HC-06 modules

Inspired by real-world IoT connectivity challenges

<div align="center">
Made with ❤️ for the Arduino community

⭐ Star this repo if you find it useful!

https://img.shields.io/github/stars/yourusername/SchreinBluetoothManager?style=social

</div>
