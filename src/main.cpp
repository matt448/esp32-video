// ESP32 Composite Video - Hello World
// Outputs "Hello World!" text over NTSC composite video on pin 25.
//
// Hardware:
//   - ESP32-WROOM-32
//   - Connect GPIO 25 (DAC1) to the centre pin of a composite video RCA connector
//   - Connect GND to the outer shield of the RCA connector
//
// Based on bitluni's ESP32CompositeVideo (CC0)
// https://github.com/bitluni/ESP32CompositeVideo

#include "esp_pm.h"
#include "CompositeGraphics.h"
#include "CompositeOutput.h"
#include "font6x8.h"

// NTSC resolution: max half-res is 324x224, use a comfortable 320x200
const int XRES = 320;
const int YRES = 200;

// Graphics double-buffer at the selected resolution
CompositeGraphics graphics(XRES, YRES);

// Composite output in NTSC mode, feeding the DAC at twice the resolution so
// the centering logic in CompositeOutput places the image correctly.
CompositeOutput composite(CompositeOutput::NTSC, XRES * 2, YRES * 2);

// 6x8 monospace font
Font<CompositeGraphics> font(6, 8, font6x8::pixels);

// Core 0 task: continuously send the graphics front-buffer as a composite frame
void compositeCore(void *data)
{
  while (true)
  {
    composite.sendFrameHalfResolution(&graphics.frame);
  }
}

void draw()
{
  // Clear the back-buffer to black (value 0)
  graphics.begin(0);

  // Centre "Hello World!" on screen
  const char *message = "Hello World!";
  const int charW = 6;
  const int charH = 8;
  int msgLen = 0;
  while (message[msgLen]) msgLen++;
  int textX = (XRES - msgLen * charW) / 2;
  int textY = (YRES - charH) / 2;

  graphics.setTextColor(50); // white-ish on the 0–54 grayscale
  graphics.setCursor(textX, textY);
  graphics.print(message);

  // Swap back-buffer to front so compositeCore picks it up
  graphics.end();
}

void setup()
{
  // Lock CPU at maximum frequency for reliable composite timing
  esp_pm_lock_handle_t powerManagementLock;
  esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "compositeCorePerformanceLock", &powerManagementLock);
  esp_pm_lock_acquire(powerManagementLock);

  // Initialise I2S/DAC composite output and graphics buffers
  composite.init();
  graphics.init();
  graphics.setFont(font);

  // Run the composite output on core 0; rendering runs on core 1 (loop())
  xTaskCreatePinnedToCore(compositeCore, "compositeCoreTask", 1024, NULL, 1, NULL, 0);
}

void loop()
{
  draw();
}
