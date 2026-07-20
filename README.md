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

The project uses [PlatformIO](https://platformio.org/) with the Arduino framework for the ESP32.

### Build & flash

```bash
# Install PlatformIO CLI if needed
pip install platformio

# Build
pio run

# Build and upload to connected ESP32
pio run --target upload

# Monitor serial output
pio device monitor
```

Or open the project folder in **VS Code** with the PlatformIO extension installed and use the PlatformIO toolbar buttons.

## How it works

1. The ESP32's I2S peripheral is clocked at ~13 MHz and drives the internal 8-bit DAC (GPIO 25) to produce a composite NTSC signal.
2. A FreeRTOS task pinned to core 0 continuously streams the front frame-buffer out as composite video.
3. The main loop (core 1) renders "Hello World!" centred on a 320×200 back-buffer using a 6×8 pixel monospace font, then swaps the buffers.

## Project structure

```
src/
  main.cpp            – Arduino setup/loop and draw routine
  CompositeOutput.h   – I2S/DAC NTSC/PAL composite signal generator
  CompositeGraphics.h – Double-buffered graphics primitives
  Font.h              – Bitmap font renderer
  Image.h             – Bitmap image renderer
  TriangleTree.h      – Sorted triangle rasteriser
  font6x8.h           – 6×8 pixel ASCII font data
platformio.ini        – PlatformIO project configuration
```
