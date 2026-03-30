# Denver DVH-1245 IR Remote

Replacement IR remote for the **Denver DVH-1245 DVD player**, built with an **M5Stack Core ESP32** and **M5Stack IR Unit**.

The DVH-1245 has no publicly available IR code database (not in LIRC, irdb, or Flipper-IRDB). This project includes a **complete command map** obtained by brute-forcing all 256 NEC command bytes against the player.

## Why

The DVD player's front panel only has play/pause/stop — no way to navigate menus or change audio language without a remote. No replacement remote or IR code database exists for this model.

## Hardware

| Component      | Details                                    |
| -------------- | ------------------------------------------ |
| MCU            | M5Stack Core ESP32                         |
| IR Transmitter | M5Stack IR Unit on **Port A** (GPIO 21 TX) |
| Protocol       | NEC, address `0x00FF`                      |

## IR Protocol

Standard NEC with 8-bit address `0x00` (inverted sub-address `0xFF`). Full 32-bit code built as:

```
0x00FF0000 | (cmd << 8) | (~cmd & 0xFF)
```

Codes are sent **once** per button press — the DVD player handles repeat internally.

## Complete Command Map

Full brute-force scan of all 256 command bytes (0x00–0xFF). This is the only known public database for this player.

### Navigation

| Byte | Dec | Function   |
| ---- | --- | ---------- |
| 0x40 | 64  | UP         |
| 0x80 | 128 | DOWN       |
| 0x30 | 48  | LEFT       |
| 0xA0 | 160 | RIGHT      |
| 0x29 | 41  | OK / ENTER |

### Playback

| Byte | Dec | Function    | Notes                                       |
| ---- | --- | ----------- | ------------------------------------------- |
| 0xC0 | 192 | PLAY/PAUSE  |                                             |
| 0xC8 | 200 | STOP        | 1st = pre-stop (resumable), 2nd = full stop |
| 0x32 | 50  | RESUME      |                                             |
| 0x0E | 14  | EJECT/CLOSE |                                             |

### Volume

| Byte | Dec | Function |
| ---- | --- | -------- |
| 0x00 | 0   | MUTE     |
| 0xEA | 234 | VOL+     |
| 0xB2 | 178 | VOL-     |

### Other

| Byte | Dec | Function        |
| ---- | --- | --------------- |
| 0x20 | 32  | POWER           |
| 0x05 | 5   | SETUP/SETTINGS  |
| 0x0A | 10  | PROGRAM         |
| 0x90 | 144 | CVBS/RGB toggle |
| 0xD8 | 216 | DISC/USB select |

## Remote UI

The M5Stack's three buttons (A, B, C) provide a 3-page remote using modifier-key combos:

| Input      | Action            |
| ---------- | ----------------- |
| A / B / C  | Primary command   |
| Hold C + A | Secondary command |
| Hold C + B | Secondary command |
| Hold A + B | Previous page     |
| Hold A + C | Next page         |

Hold threshold: 400ms.

### Pages

**Page 1 — Navigate:** LEFT, RIGHT, OK, UP (C+A), DOWN (C+B)

**Page 2 — Playback:** PLAY/PAUSE, VOL+, VOL-, STOP (C+A), MUTE (C+B)

**Page 3 — Extras:** POWER, EJECT, SETTINGS

## Building & Uploading

Requires [PlatformIO](https://platformio.org/).

```bash
# Build
pio run

# Upload (replace port with yours)
pio run -t upload --upload-port /dev/cu.usbserial-XXXXXXXX
```

## Project Structure

```
IR.ino           Main source — 3-page remote
platformio.ini   PlatformIO config
```

## License

MIT
