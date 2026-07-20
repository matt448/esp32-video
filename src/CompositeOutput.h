// Based on bitluni's ESP32CompositeVideo (CC0)
// https://github.com/bitluni/ESP32CompositeVideo
#pragma once
#include <assert.h>
#include <stdlib.h>

#include "driver/dac_continuous.h"
#include "esp_check.h"
#include "soc/i2s_reg.h"
#include "soc/soc.h"

typedef struct
{
  float lineMicros;
  float syncMicros;
  float blankEndMicros;
  float backMicros;
  float shortVSyncMicros;
  float overscanLeftMicros;
  float overscanRightMicros;
  float syncVolts;
  float blankVolts;
  float blackVolts;
  float whiteVolts;
  short lines;
  short linesFirstTop;
  short linesOverscanTop;
  short linesOverscanBottom;
  float imageAspect;
} TechProperties;

const TechProperties PALProperties = {
  .lineMicros = 64,
  .syncMicros = 4.7,
  .blankEndMicros = 10.4,
  .backMicros = 1.65,
  .shortVSyncMicros = 2.35,
  .overscanLeftMicros = 1.6875,
  .overscanRightMicros = 1.6875,
  .syncVolts = -0.3,
  .blankVolts = 0.0,
  .blackVolts = 0.005,
  .whiteVolts = 0.7,
  .lines = 625,
  .linesFirstTop = 23,
  .linesOverscanTop = 9,
  .linesOverscanBottom = 9,
  .imageAspect = 4./3.
};

const TechProperties NTSCProperties = {
  .lineMicros = 63.492,
  .syncMicros = 4.7,
  .blankEndMicros = 9.2,
  .backMicros = 1.5,
  .shortVSyncMicros = 2.3,
  .overscanLeftMicros = 0,
  .overscanRightMicros = 0,
  .syncVolts = -0.286,
  .blankVolts = 0.0,
  .blackVolts = 0.05,
  .whiteVolts = 0.714,
  .lines = 525,
  .linesFirstTop = 20,
  .linesOverscanTop = 6,
  .linesOverscanBottom = 9,
  .imageAspect = 4./3.
};

class CompositeOutput
{
  public:
  int samplesLine;
  int samplesSync;
  int samplesBlank;
  int samplesBack;
  int samplesActive;
  int samplesBlackLeft;
  int samplesBlackRight;

  int samplesVSyncShort;
  int samplesVSyncLong;

  char levelSync;
  char levelBlank;
  char levelBlack;
  char levelWhite;
  char grayValues;

  int targetXres;
  int targetYres;
  int targetYresEven;
  int targetYresOdd;

  int linesEven;
  int linesOdd;
  int linesEvenActive;
  int linesOddActive;
  int linesEvenVisible;
  int linesOddVisible;
  int linesEvenBlankTop;
  int linesEvenBlankBottom;
  int linesOddBlankTop;
  int linesOddBlankBottom;

  float pixelAspect;

  unsigned char *line;
  dac_continuous_handle_t dac;

  enum Mode
  {
    PAL,
    NTSC
  };

  const TechProperties &properties;

  CompositeOutput(Mode mode, int xres, int yres, double Vcc = 3.3)
    :properties((mode == NTSC) ? NTSCProperties : PALProperties)
  {
    int linesSyncTop = 5;
    int linesSyncBottom = 3;

    linesOdd = properties.lines / 2;
    linesEven = properties.lines - linesOdd;
    linesEvenActive = linesEven - properties.linesFirstTop - linesSyncBottom;
    linesOddActive = linesOdd - properties.linesFirstTop - linesSyncBottom;
    linesEvenVisible = linesEvenActive - properties.linesOverscanTop - properties.linesOverscanBottom;
    linesOddVisible = linesOddActive - properties.linesOverscanTop - properties.linesOverscanBottom;

    targetYresOdd = (yres / 2 < linesOddVisible) ? yres / 2 : linesOddVisible;
    targetYresEven = (yres - targetYresOdd < linesEvenVisible) ? yres - targetYresOdd : linesEvenVisible;
    targetYres = targetYresEven + targetYresOdd;

    linesEvenBlankTop = properties.linesFirstTop - linesSyncTop + properties.linesOverscanTop + (linesEvenVisible - targetYresEven) / 2;
    linesEvenBlankBottom = linesEven - linesEvenBlankTop - targetYresEven - linesSyncBottom;
    linesOddBlankTop = linesEvenBlankTop;
    linesOddBlankBottom = linesOdd - linesOddBlankTop - targetYresOdd - linesSyncBottom;

    double samplesPerSecond = 160000000.0 / 3.0 / 2.0 / 2.0;
    double samplesPerMicro = samplesPerSecond * 0.000001;
    samplesLine = (int)(samplesPerMicro * properties.lineMicros + 1.5) & ~1;
    samplesSync = samplesPerMicro * properties.syncMicros + 0.5;
    samplesBlank = samplesPerMicro * (properties.blankEndMicros - properties.syncMicros + properties.overscanLeftMicros) + 0.5;
    samplesBack = samplesPerMicro * (properties.backMicros + properties.overscanRightMicros) + 0.5;
    samplesActive = samplesLine - samplesSync - samplesBlank - samplesBack;

    targetXres = xres < samplesActive ? xres : samplesActive;

    samplesVSyncShort = samplesPerMicro * properties.shortVSyncMicros + 0.5;
    samplesBlackLeft = (samplesActive - targetXres) / 2;
    samplesBlackRight = samplesActive - targetXres - samplesBlackLeft;
    double dacPerVolt = 255.0 / Vcc;
    levelSync = 0;
    levelBlank = (properties.blankVolts - properties.syncVolts) * dacPerVolt + 0.5;
    levelBlack = (properties.blackVolts - properties.syncVolts) * dacPerVolt + 0.5;
    levelWhite = (properties.whiteVolts - properties.syncVolts) * dacPerVolt + 0.5;
    grayValues = levelWhite - levelBlack + 1;

    pixelAspect = (float(samplesActive) / (linesEvenVisible + linesOddVisible)) / properties.imageAspect;
  }

  void init()
  {
    line = (unsigned char*)malloc(samplesLine);
    assert(line);
    dac_continuous_config_t dacConfig = {
      .chan_mask = DAC_CHANNEL_MASK_CH0,
      .desc_num = 2,
      .buf_size = (size_t)samplesLine,
      .freq_hz = 1000000,
      .offset = 0,
      .clk_src = DAC_DIGI_CLK_SRC_DEFAULT,
      .chan_mode = DAC_CHANNEL_MODE_SIMUL
    };

    ESP_ERROR_CHECK(dac_continuous_new_channels(&dacConfig, &dac));
    ESP_ERROR_CHECK(dac_continuous_enable(dac));

    // DAC continuous mode owns I2S0 DMA but limits its public clock API to
    // 2.5 MHz. Composite video needs the original 13.33 MHz I2S0 divider.
    SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_A_V, 1, I2S_CLKM_DIV_A_S);
    SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_B_V, 1, I2S_CLKM_DIV_B_S);
    SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_NUM_V, 2, I2S_CLKM_DIV_NUM_S);
    SET_PERI_REG_BITS(I2S_SAMPLE_RATE_CONF_REG(0), I2S_TX_BCK_DIV_NUM_V, 2, I2S_TX_BCK_DIV_NUM_S);
  }

  void sendLine()
  {
    size_t bytes_written = 0;
    size_t bytes_to_write = samplesLine;
    size_t cursor = 0;
    while(bytes_to_write > 0)
    {
      ESP_ERROR_CHECK(dac_continuous_write(dac, line + cursor, bytes_to_write, &bytes_written, -1));
      bytes_to_write -= bytes_written;
      cursor += bytes_written;
    }
  }

  inline void fillValues(int &i, unsigned char value, int count)
  {
    for(int j = 0; j < count; j++)
      line[i++] = value;
  }

  void fillLine(char *pixels)
  {
    int i = 0;
    fillValues(i, levelSync, samplesSync);
    fillValues(i, levelBlank, samplesBlank);
    fillValues(i, levelBlack, samplesBlackLeft);
    for(int x = 0; x < targetXres / 2; x++)
    {
      unsigned char pix = levelBlack + pixels[x];
      line[i++] = pix;
      line[i++] = pix;
    }
    fillValues(i, levelBlack, samplesBlackRight);
    fillValues(i, levelBlank, samplesBack);
  }

  void fillLong(int &i)
  {
    fillValues(i, levelSync, samplesLine / 2 - samplesVSyncShort);
    fillValues(i, levelBlank, samplesVSyncShort);
  }

  void fillShort(int &i)
  {
    fillValues(i, levelSync, samplesVSyncShort);
    fillValues(i, levelBlank, samplesLine / 2 - samplesVSyncShort);
  }

  void fillBlank()
  {
    int i = 0;
    fillValues(i, levelSync, samplesSync);
    fillValues(i, levelBlank, samplesBlank);
    fillValues(i, levelBlack, samplesActive);
    fillValues(i, levelBlank, samplesBack);
  }

  void fillHalfBlank(int &i)
  {
    fillValues(i, levelSync, samplesSync);
    fillValues(i, levelBlank, samplesLine / 2 - samplesSync);
  }

  void sendFrameHalfResolution(char ***frame)
  {
    // Even half-frame
    int i = 0;
    fillLong(i); fillLong(i);
    sendLine(); sendLine();
    i = 0;
    fillLong(i); fillShort(i);
    sendLine();
    i = 0;
    fillShort(i); fillShort(i);
    sendLine(); sendLine();
    fillBlank();
    for(int y = 0; y < linesEvenBlankTop; y++)
      sendLine();
    for(int y = 0; y < targetYresEven; y++)
    {
      char *pixels = (*frame)[y];
      fillLine(pixels);
      sendLine();
    }
    fillBlank();
    for(int y = 0; y < linesEvenBlankBottom; y++)
      sendLine();
    i = 0;
    fillShort(i); fillShort(i);
    sendLine(); sendLine();
    i = 0;
    fillShort(i);
    // Odd half-frame
    fillLong(i);
    sendLine();
    i = 0;
    fillLong(i); fillLong(i);
    sendLine(); sendLine();
    i = 0;
    fillShort(i); fillShort(i);
    sendLine(); sendLine();
    i = 0;
    fillShort(i); fillValues(i, levelBlank, samplesLine / 2);
    sendLine();

    fillBlank();
    for(int y = 0; y < linesOddBlankTop; y++)
      sendLine();
    for(int y = 0; y < targetYresOdd; y++)
    {
      char *pixels = (*frame)[y];
      fillLine(pixels);
      sendLine();
    }
    fillBlank();
    for(int y = 0; y < linesOddBlankBottom; y++)
      sendLine();
    i = 0;
    fillHalfBlank(i); fillShort(i);
    sendLine();
    i = 0;
    fillShort(i); fillShort(i);
    sendLine(); sendLine();
  }
};
