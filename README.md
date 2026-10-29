# DTMF decoder for PC / PlayStation emulator control - Maxihra

A USB keyboard controlled by DTMF tones.

An **MT8870** decoder converts tones (e.g. from a phone) into a 4-bit code, and a microcontroller with native USB HID turns them into key presses on a PC. 

Inspired by Maxihra, a 1990s TV show also broadcast in Slovakia, this project is designed mainly for playing PlayStation games in DuckStation (D-Pad, face buttons, L1/L2/R1/R2), while profiles P3 and P4 work as a plain numeric keyboard.


## Contents

- [Features](#features)
- [Versions](#versions)
- [Hardware](#hardware)
- [Wiring](#wiring)
- [Profiles and mapping](#profiles-and-mapping)
- [Configuration](#configuration)
- [Debug](#debug)
- [Known limitations](#known-limitations)
- [Repository structure](#repository-structure)

## Features

- Decodes all 16 DTMF symbols (`0-9`, `*`, `#`, `A-D`).
- 4 profiles, switched with tones `A`, `B`, `C`, `D`.
- Active profile indicated by the LED (number of blinks = profile number).
- Supports special keys (arrow keys) as well as regular characters.
- Debouncing of the StD signal.
- Non-blocking LED (state machine).
- Optional serial debug output (115200 baud).

## Versions

| | **pulse** | **hold** |
|---|---|---|
| Key press length | fixed 80 ms (`KEY_TIME`) | key stays pressed while the tone lasts |
| Release | after `KEY_TIME` expires | when there is no StD activity for 60 ms (`RELEASE_TIMEOUT`) |
| Repeated same tone | every tone = a new key press | tone is just held, nothing is resent |
| Best for | menus, single hits | movement in games (holding a direction) |

**Note on the hold version:** holding works only if the source actually keeps playing the tone while the button is pressed. Some phones play only a fixed ~100-150 ms tone when dialing.

## Hardware

| Part | Note |
|---|---|
| **Arduino Pro Micro** (ATmega32U4, 5 V / 16 MHz) | recommended; Leonardo / Micro work as well |
| **MT8870 module** | Q1-Q4 and StD outputs, 3.5 mm jack input; OE must be tied HIGH |
| 3.5 mm audio cable | from the phone / source to the module |

Boards without native USB (Uno, Nano) **do not work**, because `Keyboard.h` does not support them.

Boards with 3.3 V logic (Raspberry Pi Pico, ESP32-S3) are possible if you add a voltage divider or level shifter between the MT8870 (5 V) and the GPIO pins.

## Wiring

| MT8870 | Pro Micro |
|---|---|
| Q1 | D2 |
| Q2 | D3 |
| Q3 | D4 |
| Q4 | D5 |
| StD/StQ | D6 |
| VCC | 5 V |
| GND | GND |

The LED uses `LED_BUILTIN`. Pin numbers can be changed in the `CONFIGURATION` section of the code.

### MT8870 table (Q4 Q3 Q2 Q1)

The code is converted using the `mt8870Map` array:

| Raw (Q4Q3Q2Q1) | DTMF | Internal code |
|---|---|---|
| 0001-1001 | 1-9 | 1-9 |
| 1010 | 0 | 0 |
| 1011 | * | 10 |
| 1100 | # | 11 |
| 1101 | A | 12 |
| 1110 | B | 13 |
| 1111 | C | 14 |
| 0000 | D | 15 |

Profile switching: `A` = P1, `B` = P2, `C` = P3, `D` = P4.


## Profiles and mapping

DuckStation may not have these keys assigned by default. Set them in *Settings → Controllers → Keyboard bindings* according to the tables below.

### P1 (DuckStation)

| DTMF | PS button | Key |
|---|---|---|
| 0 | D-Pad Down | ↓ |
| 1 | L2 | `1` |
| 2 | Triangle | `a` |
| 3 | R2 | `3` |
| 4 | L1 | `q` |
| 5 | Circle | `z` |
| 6 | R1 | `e` |
| 7 | D-Pad Left | ← |
| 8 | D-Pad Up | ↑ |
| 9 | D-Pad Right | → |
| * | Cross | `x` |
| # | Square | `s` |

### P2 (DuckStation, alternative Maxihra layout)

Compared to P1: `1` = D-Pad Left, `2` = D-Pad Right, `9` = Cross, Triangle is on `7`, L2 is on `*`.

| DTMF | PS button | Key |
|---|---|---|
| 0 | D-Pad Down | ↓ |
| 1 | D-Pad Left | ← |
| 2 | D-Pad Right | → |
| 3 | R2 | `3` |
| 4 | L1 | `q` |
| 5 | Circle | `z` |
| 6 | R1 | `e` |
| 7 | Triangle | `a` |
| 8 | D-Pad Up | ↑ |
| 9 | Cross | `x` |
| * | L2 | `1` |
| # | Square | `s` |

### P3 and P4

Numeric keyboard: `0-9`, `*`, `#` are sent as the same characters. Tones A-D are used for profile switching, so they are not mapped.

### Custom profile

A profile is an array of 16 `KeyAction {key, special}` entries. For special keys (arrows, Enter, ...) set `special = true`, for characters `false`. An entry `{0,false}` means "do nothing".

```cpp
KeyAction profileP3[16] =
{
  {KEY_UP_ARROW, true},   // 0
  {'w', false},           // 1
  ...
};
```

## Configuration

| Constant | Default | Meaning |
|---|---|---|
| `DEBUG` | `true` | enables serial output |
| `PIN_Q1`-`PIN_Q4`, `PIN_STD` | 2-6 | MT8870 pins |
| `STD_FILTER` | 20 ms | minimum spacing between valid StD edges |
| `KEY_TIME` | 80 ms | key press length (**pulse** only) |
| `RELEASE_TIMEOUT` | 60 ms | end of tone without StD activity (**hold** only) |
| `LED_INTERVAL` | 120 ms | LED blink speed |

## Debug

With `DEBUG = true` the sketch waits for the serial port for up to 3 s, then prints:

```
MT8870 DTMF Keyboard
Active profile: P1
DTMF=5
Key: z
Profile: P2
```

The hold version additionally prints `Key DOWN` and `Key released`.

If you don't need the serial monitor, set `DEBUG = false` to skip the startup wait.

## Known limitations

- **Only one key at a time.** DTMF carries a single tone pair, so you cannot hold e.g. a direction and a button simultaneously. Combined codes (one tone = multiple keys) would be a solution.
- **Hold version:** when the same key is tapped very quickly, the second tap may be lost (state is reset only after `RELEASE_TIMEOUT`). Release is delayed by `RELEASE_TIMEOUT`.
- **Latency** over a call is 100 ms or more. A direct audio cable is much better.
- Too high a source volume can distort the signal and worsen detection.

### Incompatible call technologies

DTMF must arrive as a **real audio tone** (in-band) at the MT8870 audio input. Modern call technologies often transmit it as signaling (RFC 4733, telephone-event) rather than audio, so the tone may not appear on the output at all.

| Technology | Status |
|---|---|
| **VoLTE** | does not work |
| **Wi-Fi Calling** (VoWiFi) | does not work |

Recommendation: disable VoLTE (and Wi-Fi Calling) in the phone settings so the call goes over 2G/3G or CS fallback, or play tones directly from a generator/app over the audio cable without a call.

## Repository structure

```
.
├── pulse/
│   └── pulse.ino
├── hold/
│   └── hold.ino
└── README.md
```

## MIT license

 [LICENSE](LICENSE)
