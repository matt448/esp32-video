# esp32-video

Composite video output project using an ESP32-WROOM-32, displaying **"Hello World!"** via the built-in DAC on GPIO 25.

Based on [bitluni's ESP32CompositeVideo](https://github.com/bitluni/ESP32CompositeVideo) (CC0 license).

---

## Hardware

| ESP32 pin | RCA connector |
|-----------|---------------|
| GPIO 25 (DAC1) | Centre pin (signal) |
| GND | Outer shield (ground) |

Connect the RCA connector to a TV or monitor's composite video (yellow) input.

## Software

The project uses [ESP-IDF](https://docs.espressif.com/projects/esp-idf/) and is intended to be opened with the Espressif ESP-IDF extension for VS Code.

### Build & flash

```powershell
# Set up the ESP-IDF environment for PowerShell.
& "$env:IDF_PATH\export.ps1"

# Configure the project for ESP32-WROOM-32.
idf.py set-target esp32

# Build, flash, and monitor a connected board.
idf.py build
idf.py -p PORT flash monitor
```

Open the project folder in **VS Code** with the Espressif ESP-IDF extension. Use **ESP-IDF: Build your project**, then **ESP-IDF: Flash your project** and **ESP-IDF: Monitor your device**.

## How it works

1. The ESP32's I2S peripheral is clocked at ~13 MHz and drives the internal 8-bit DAC (GPIO 25) to produce a composite NTSC signal.
2. A FreeRTOS task pinned to core 0 continuously streams the front frame-buffer out as composite video.
3. The main loop (core 1) renders "Hello World!" centred on a 320×200 back-buffer using a 6×8 pixel monospace font, then swaps the buffers.

## Project structure

```
src/                 ESP-IDF component
  CMakeLists.txt     – Component build configuration
  main.cpp            – ESP-IDF application entry point and draw routine
  CompositeOutput.h   – I2S/DAC NTSC/PAL composite signal generator
  CompositeGraphics.h – Double-buffered graphics primitives
  Font.h              – Bitmap font renderer
  Image.h             – Bitmap image renderer
  TriangleTree.h      – Sorted triangle rasteriser
  font6x8.h           – 6×8 pixel ASCII font data
CMakeLists.txt        – ESP-IDF project configuration
sdkconfig.defaults    – ESP32 target and CPU frequency defaults
```
