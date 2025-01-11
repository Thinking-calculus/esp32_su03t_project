#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"
#include "SD.h"
#include "FS.h"

#include <string>

// sd卡相关引脚
#define SD_CS 5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18

// 功放I2S相关引脚
#define I2S_DOUT 13
#define I2S_BCLK 12
#define I2S_LRC 14

#define FUN_OK 1
#define FUN_FAIL 0

// 音乐列表
#define MAX_FILES 100
#define MAX_FILENAME_LENGTH 50

// audio 全局变量
Audio audio;
std::vector<std::string> file_name_list;

// sd 卡初始化
int sd_init(int SD_CS_INIT, int SPI_SCK_INIT, int SPI_MISO_INIT, int SPI_MOSI_INIT)
{
  pinMode(SD_CS_INIT, OUTPUT);
  digitalWrite(SD_CS_INIT, HIGH);
  SPI.begin(SPI_SCK_INIT, SPI_MISO_INIT, SPI_MOSI_INIT);
  SD.begin(SD_CS_INIT);

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE)
  {
    Serial.println("No SD card attached");
    return FUN_FAIL;
  }
  else
  {
    Serial.println("SD card attached");
    return FUN_OK;
  }
}

// 功放初始化
void audio_init(int I2S_DOUT_INIT, int I2S_BCLK_INIT, int I2S_LRC_INIT)
{
  audio.setPinout(I2S_BCLK_INIT, I2S_LRC_INIT, I2S_DOUT_INIT);
  audio.setVolume(4); // 0...21
}

// 获取音频文件列表
int get_audio_list()
{
  // 打开根目录
  File root = SD.open("/");
  if (!root)
  {
    Serial.println("Failed to open directory");
    return FUN_FAIL;
  }
  if (!root.isDirectory())
  {
    Serial.println("Not a directory");
    root.close();
    return FUN_FAIL;
  }

  File file = root.openNextFile();

  while (file)
  {
    // 筛选出mp3文件
    std::string file_name(file.name());
    if (file_name.ends_with(".mp3") && file.size() > 0)
    {
      Serial.printf("get mp3 file name:%s,file size %d\n", file.name(), file.size());
      file_name_list.push_back(file_name);
    }
    file = root.openNextFile();
  }
  root.close();
  return FUN_OK;
}

void setup()
{
  // 串口初始化
  Serial.begin(115200);
  while (!Serial)
  {
    delay(10);
  }

  // 初始化功放
  audio_init(I2S_DOUT, I2S_BCLK, I2S_LRC);

  // sd 卡初始化
  if (sd_init(SD_CS, SPI_SCK, SPI_MISO, SPI_MOSI) == FUN_OK)
  {

    // 获取sd卡音频文件列表
    if (get_audio_list() != FUN_OK)
    {
      Serial.println("get audio list fail");
    }
    else
    {
      Serial.printf("get audio list success,music count:%d", file_name_list.size());
    }
  }

  if (file_name_list.size() > 0)
  {
    std::string first = file_name_list[0];
    first="/"+first;
    audio.connecttoFS(SD,first.c_str());
  }
}

void loop()
{
  audio.loop();
  if (audio.isRunning() == false)
  {
    Serial.println("play eos to file");
  }
}
