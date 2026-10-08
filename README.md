# 🏠 Adam's Room: Smart Room Control System

A DIY smart room system built with an **Arduino Nano** and a **web app I built myself**. The light, the TV and a buzzer alarm can all be controlled **three ways**: from the app over Bluetooth, from an IR remote, or with physical push buttons.

🌐 **Live app:** [Open the app](https://YOUR-USERNAME.github.io/YOUR-REPO-NAME/) (Chrome on Android or PC)

🎥 **Demo video:** [Watch on LinkedIn](LINK-TO-YOUR-POST)

<!-- Add a photo or GIF of the setup here -->
<!-- ![Setup](images/setup.jpg) -->

---

## ✨ Features

| Feature | App | IR Remote | Push Button |
|---|:---:|:---:|:---:|
| Light on / off | ✅ | ✅ | ✅ |
| Light timer (turn on/off after a set time) | ✅ | | |
| TV power on / off | ✅ | ✅ | ✅ |
| TV volume up / down (hold to keep changing) | ✅ | ✅ | |
| TV mute | | ✅ | ✅ |
| Alarm (buzzer rings after a set time) | ✅ | | |
| Buzzer on / off | ✅ | ✅ | ✅ |
| Buzzer test tone | ✅ | ✅ | |

The app also **stays in sync**: if the light is switched with the remote or a button, the Arduino reports the new state back over Bluetooth and the app updates.

---

## ⚙️ How It Works

```
   📱 Web App ──(Bluetooth)──┐
                             │
   🎛️ IR Remote ──(IR)───────┼──►  Arduino Nano  ──►  💡 Light
                             │                    ──►  📺 Samsung TV (via IR transmitter)
   🔘 Push Buttons ──────────┘                    ──►  🔔 Buzzer
```

1. **App → Arduino:** the web app connects over Bluetooth (Web Bluetooth API) and sends text commands like `1`, `OFF:600` or `TV:UP`.
2. **Remote → Arduino:** the IR receiver decodes the remote's buttons, and the Arduino maps each one to an action.
3. **Arduino → TV:** the IR transmitter sends Samsung IR codes to control the TV's power, volume and mute.
4. **Arduino → App:** state changes are sent back (e.g. `STATE:1`, `BUZALARM:RINGING`) so the app always shows the real state.

All timing (light timer, alarm, button debouncing) is handled with `millis()`, so nothing blocks and the system stays responsive.

---

## 🧰 Hardware

- Arduino Nano
- Bluetooth Low Energy module (HM-10 / compatible, service `FFE0`)
- IR receiver
- IR transmitter (IR LED)
- Buzzer
- 4 push buttons
- Light (connected to D7)
- IR remote

## 🔌 Wiring

| Component | Arduino Pin |
|---|---|
| Bluetooth module TX | D10 |
| Bluetooth module RX | D11 |
| IR receiver OUT | D2 |
| IR transmitter | D3 |
| Buzzer | D5 |
| Light | D7 |
| Light button | D4 |
| TV mute button | D8 |
| TV power button | A1 |
| Buzzer on/off button | A2 |

All buttons connect between the pin and **GND** (the code uses the internal pull-up resistors).

---

## 📡 Bluetooth Commands

| Command | Action |
|---|---|
| `1` / `0` | Light on / off |
| `ON:<seconds>` | Turn light on after a delay |
| `OFF:<seconds>` | Turn light off after a delay |
| `STATE?` | Ask for the current light state |
| `BUZ:1` / `BUZ:0` | Buzzer on / off |
| `BUZ:TEST` | Play a test tone |
| `BUZ:ALARM:<seconds>` | Set the alarm |
| `BUZ:ALARM:STOP` | Stop the alarm |
| `TV:OC` | TV power on / off |
| `TV:UP` / `TV:DOWN` | Volume up / down |
| `TV:MUTE` | Mute |
| `TV:HOLD:UP` / `TV:HOLD:DOWN` / `TV:HOLD:STOP` | Keep changing the volume while the button is held |

---

## 🚀 Getting Started

### Arduino
1. Install the **IRremote** library (v4 or newer) from the Arduino Library Manager.
2. Open `arduino/smart_room/smart_room.ino`.
3. Select **Arduino Nano** as the board and upload.

### App
1. Open the live app link above in **Chrome** (Android or PC).
2. Tap **Connect device** and choose your Bluetooth module.
3. Start controlling the room.

> Web Bluetooth isn't supported on iPhone (Safari), so use an Android phone or a PC.

---

## 📁 Project Structure

```
├── index.html                    # Web app
├── arduino/
│   └── smart_room/
│       └── smart_room.ino        # Arduino code
├── images/                       # Photos of the setup
└── README.md
```

---

## 👤 Author

**Adam Ahmed**, Communications & Electronics Engineering student at AASTMT
[LinkedIn](https://www.linkedin.com/in/adam-ahmed-733072413/)
