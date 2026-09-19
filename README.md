# Pineappley-Watch
A digital watch with a transparent OLED display driven by a ESP32C3 on my custom PCB.
<img width="1500" height="1044" alt="15" src="https://github.com/user-attachments/assets/8f5f3e02-68de-4e60-8ae9-ae6ce5799f58" />

# Features
## Hardware
- Onboard Lithium battery charger
- USB-C port for charging, Flashing and powering
- Charging switching circuit to switch from battery to USB power when charging
- 3.3V Regulator circuit
- 12V Boost Circuit
- ESP32C3-Wroom-02 Module
- Transparent 1.51 inch OLED display
- 2 Buttons for control
## Software
- 9 Separate watch faces
- Bluetooth connectivity with phone through Gadgetbridge
- Deep Sleep, press to wake
- Weather
- Games
- Ability to set time
- My class timetable
- Live notifications while awake

# PCB
This project has my first 4 layer, 2 sided PCB. I chose this stack to make it as compact as possible keeping my voltage bus, charger, microcontroller all in a small package.
## Charger and regulator
<img width="1439" height="1251" alt="6" src="https://github.com/user-attachments/assets/7e4181f0-e60d-4413-abe6-61d44c93a77a" />

## Real time Clock
<img width="1393" height="1000" alt="8" src="https://github.com/user-attachments/assets/da36ffb9-79a7-4fb6-9f1e-3cc083b7e60e" />

## 12V Boost
<img width="1500" height="1429" alt="7" src="https://github.com/user-attachments/assets/73f77dd9-76ea-4ce2-a25a-50133d87927d" />

## ESP and Peripherals
<img width="1500" height="1379" alt="9" src="https://github.com/user-attachments/assets/d4008d8f-5143-43ab-9d00-373128f49316" />

## Final VS Debug Board
<img width="1500" height="1000" alt="10" src="https://github.com/user-attachments/assets/f19a450a-2cef-4c94-b028-165a76ded716" />

# Firmware Architecture
The firmware is built on the platformIO plugin for VSCode, written in arduino C++.
The Watch stays on 24/7 to keep counting time but Deep Sleeps after 90 seconds of inactivity to save battery power to hold out even for a few days.

# Showcase
Here is a little showcase GIF of the firmware features I’m hosting on my server, too big to keep in the README
https://cloud.webplanet.ie/nextcloud/index.php/s/4JbjHRAqcWJgZ6F
