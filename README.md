# BC250 Auto-Off Controller

Automatic ATX power controller for the **AMD BC-250** using a **LOLIN/WEMOS ESP32-S2 Mini**, a **2N7000 MOSFET**, and an **FSP500-30AS** power supply.

The goal is simple: start the PSU from a momentary button, let the BC-250 boot normally, and automatically switch the PSU off after the BC-250 completes a normal operating-system shutdown.

## Features

- Safe boot state: PS_ON control starts LOW, so there is no automatic power-on.
- Short press on GPIO5 starts and holds the ATX PSU on.
- Waits for the BC-250 state signal before arming automatic shutdown.
- BC-250 ON is confirmed only after GPIO9 stays HIGH for 500 ms.
- After the controller is armed, GPIO9 LOW continuously for 3 seconds switches the PSU off.
- If GPIO9 returns HIGH before 3 seconds, shutdown is cancelled.
- Holding the momentary button for 5 seconds while powered on forces the PSU off.
- The startup button press cannot accidentally become the 5-second force-off command.
- Uses millis()-based timing and button debounce rather than long blocking delays.

## Hardware

- AMD BC-250 16 GB
- FSP500-30AS ATX PSU
- LOLIN/WEMOS ESP32-S2 Mini
- 2N7000 N-channel MOSFET
- 100 ohm resistor
- Momentary push button
- Hookup wire

## Wiring

| Connection | Destination |
|---|---|
| FSP 5V standby | ESP32 VBUS / 5V |
| FSP GND | ESP32 GND |
| BC-250 TPMS1 pin 9 | ESP32 GPIO9 |
| ESP32 GPIO7 | 100 ohm resistor -> 2N7000 Gate |
| 2N7000 Source | GND |
| 2N7000 Drain | FSP PS_ON |
| ESP32 GPIO5 | Momentary button -> GND |

### 2N7000 orientation

With the flat face / lettering facing you, verify the pinout of your actual part before soldering. The controller wiring used for this build is:

- Source -> GND
- Gate <- 100 ohm <- GPIO7
- Drain -> PS_ON

## State sequence

1. **POWER_OFF**: GPIO7 LOW.
2. Short press GPIO5.
3. GPIO7 HIGH turns the PSU on and the controller enters **WAITING_FOR_BC250**.
4. GPIO9 may remain LOW while the BC-250 has not started. This does **not** turn the PSU off.
5. GPIO9 HIGH continuously for 500 ms enters **BC250_RUNNING** and arms AUTO-OFF.
6. When the operating system shuts the BC-250 down, GPIO9 becomes LOW.
7. If GPIO9 remains LOW for 3 seconds, GPIO7 goes LOW and the PSU turns off.
8. Holding GPIO5 for 5 seconds while the PSU is being held on performs a manual force-off.

## Important safety notes

- **Never connect 5 V to the ESP32 3V3 pin.**
- PS_ON connects to the **2N7000 Drain**, not directly to an ESP32 GPIO.
- GPIO7 drives the MOSFET Gate through the 100 ohm resistor.
- All grounds must share a common reference.
- Remove any GPIO0-to-GND bootloader jumper before normal operation.
- Do not use a latching switch that permanently grounds PS_ON with this controller.
- Verify voltages and your specific hardware pinout before connecting the BC-250.

## ESP32-S2 build information

Tested configuration:

- Board: LOLIN S2 Mini
- Arduino FQBN: `esp32:esp32:lolin_s2_mini`
- ESP32 Arduino core: 3.3.12
- Normal USB VID/PID observed: `303A:80C2`
- ROM bootloader VID/PID observed: `303A:0002`

## ROM bootloader recovery

If the BOOT button is unavailable, the ESP32-S2 ROM bootloader can be entered by holding GPIO0 to GND during reset. Remove the GPIO0-GND connection before attempting a normal boot.

## Firmware

The Arduino sketch is in:

`BC250_Controller/BC250_Controller.ino`

## Legal / redistribution

This repository is intended to contain only original project documentation and source code that may be redistributed. It does **not** include cracked software, license keys, proprietary firmware, copyrighted third-party binaries, or other pirated material.

AMD, ESP32, LOLIN/WEMOS, and FSP product names and trademarks belong to their respective owners. This is an independent community project and is not affiliated with or endorsed by those companies.

## Support

If this project helped you, you can support the work here:

[☕ Buy Me a Coffee](https://buymeacoffee.com/oversoul)

## Author

Created by [Oversoul04](https://github.com/Oversoul04).
