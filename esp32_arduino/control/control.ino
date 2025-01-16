#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"
#include "SD.h"
#include "FS.h"

/*
#include <HardwareSerial.h>
HardwareSerial SerialPort(2); // use UART2
*/

#include <string>
#include <unistd.h>

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
int current_file_index = 0;

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
  audio.setVolume(10); // 0...21
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

  // 初始化串口2
  Serial2.begin(115200, SERIAL_8N1, 16, 17); // 初始化 Serial1 串口，波特率为 115200，RX 引脚为 16，TX 引脚为 17

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
      Serial.printf("get audio list success,music count:%d\r\n", file_name_list.size());
    }
  }
}

void play_all_music()
{
  if (file_name_list.size() > 0)
  {
    if (current_file_index >= file_name_list.size())
    {
      current_file_index = 0; // 循环播放所有音乐
    }
    Serial.printf("play music file:%s \r\n", file_name_list[current_file_index].c_str());
    std::string music_name = file_name_list[current_file_index];
    music_name = "/" + music_name;
    audio.connecttoFS(SD, music_name.c_str());
    current_file_index++;
  }
}

void play_selected_music(int index)
{
  if (index >= 0 && index < file_name_list.size())
  {
    Serial.printf("play music file:%s \r\n", file_name_list[index].c_str());
    std::string music_name = file_name_list[index];
    music_name = "/" + music_name;
    audio.connecttoFS(SD, music_name.c_str());
  }
}

// 语音识别解析
void tts_sovle()
{
  if (Serial2.available())
  {
    char data = Serial2.read(); // 暂时只接受字符信息
    Serial.printf("get data(%s) from tts \r\n", data);
    if (data == '-1')
    {
      play_all_music();
    }
    else if (data > 0)
    {
      play_selected_music(std::stoi(std::string(1, data)));
    }
  }
}

void loop()
{
  audio.loop();

  if (audio.isRunning() == false)
  {
    usleep(10000);
    if (file_name_list.size() > 0 && audio.isRunning() == false)
    {
      if (current_file_index != 0)
      {
        Serial.println("play eos to file");
        Serial2.println("play eos to file");
      }
      if (current_file_index >= file_name_list.size())
      {
        current_file_index = 0; // 循环播放所有音乐
      }
      Serial.printf("play music file:%s \r\n", file_name_list[current_file_index].c_str());
      Serial2.printf("play music file:%s \r\n", file_name_list[current_file_index].c_str());
      std::string music_name = file_name_list[current_file_index];
      music_name = "/" + music_name;
      audio.connecttoFS(SD, music_name.c_str());
      current_file_index++;
    }
  }
  if (Serial2.available())
  {
    char number = Serial2.read();
    Serial.println(number);
    if (number == '0')
    {

      Serial.println("aaaaaaaaaaaa");
      Serial2.write("7");
      Serial.println("7w");
    }
    if (number == '1')
    {

      Serial.println("bbbbbbbbbbb");
      Serial2.write("6");
      Serial.println("6w");
    }
  }
}
