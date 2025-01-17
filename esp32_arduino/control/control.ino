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

// 串口通信相关
#define START_BYTE 0x01
#define END_BYTE 0x10

// 音乐列表
#define MAX_FILES 100
#define MAX_FILENAME_LENGTH 50

// audio 全局变量
Audio audio;
std::vector<std::string> file_name_list;
int current_file_index = 0;

// 播放流程控制变量
bool receiving = false;
std::vector<uint8_t> buffer;
int music_loop = 0; // -1:顺序播放 1:逆序循环 2:单曲循环

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

// void play_all_music()
// {
//   if (file_name_list.size() > 0)
//   {
//     if (current_file_index >= file_name_list.size())
//     {
//       current_file_index = 0; // 循环播放所有音乐
//     }
//     Serial.printf("play music file:%s \r\n", file_name_list[current_file_index].c_str());
//     std::string music_name = file_name_list[current_file_index];
//     music_name = "/" + music_name;
//     audio.connecttoFS(SD, music_name.c_str());
//     current_file_index++;
//   }
// }

// void play_selected_music(int index)
// {
//   if (index >= 0 && index < file_name_list.size())
//   {
//     Serial.printf("play music file:%s \r\n", file_name_list[index].c_str());
//     std::string music_name = file_name_list[index];
//     music_name = "/" + music_name;
//     audio.connecttoFS(SD, music_name.c_str());
//   }
// }

// // 语音识别解析
// void tts_sovle()
// {
//   if (Serial2.available())
//   {
//     char data = Serial2.read(); // 暂时只接受字符信息
//     Serial.printf("get data(%s) from tts \r\n", data);
//     if (data == '-1')
//     {
//       play_all_music();
//     }
//     else if (data > 0)
//     {
//       // play_selected_music(std::stoi(std::string(1, data)));
//     }
//   }
// }

void handleData(const std::vector<uint8_t> &data)
{
  // // 处理接收到的数据
  Serial.print("Received data: ");
  for (uint8_t byte : data)
  {
    Serial.printf("%02X ", byte);
  }
  Serial.println();

  // 判断接收到的数据
  uint8_t expectedData1[] = {0x00, 0x00};  // 顺序播放
  uint8_t expectedData2[] = {0x00, 0x02};  // 逆序播放
  uint8_t expectedData3[] = {0x00, 0x03};  // 下一首
  uint8_t expectedData4[] = {0x00, 0x04};  // 上一首
  uint8_t expectedData5[] = {0x00, 0x05};  // 大声一点
  uint8_t expectedData6[] = {0x00, 0x06};  // 小声一点
  uint8_t expectedData7[] = {0x00, 0x07};  // 最大声/最大音量
  uint8_t expectedData8[] = {0x00, 0x08};  // 最小声/最小音量
  uint8_t expectedData9[] = {0x00, 0x09};  // 暂停播放
  uint8_t expectedData10[] = {0x00, 0x0A}; // 恢复播放
  uint8_t expectedData11[] = {0x00, 0x0B}; // 结束播放
  uint8_t expectedData12[] = {0x00, 0x0C}; // 开始播放
  uint8_t expectedData13[] = {0x00, 0x0D}; // 单曲循环
  uint8_t expectedData14[] = {0x00, 0x0E}; // 今天xx天气怎样
  uint8_t expectedData15[] = {0x00, 0x0F}; // 今天气温是多少

  if (data.size() == sizeof(expectedData1) && memcmp(data.data(), expectedData1, sizeof(expectedData1)) == 0)
  {
    music_loop = -1;
  }
  else if (data.size() == sizeof(expectedData2) && memcmp(data.data(), expectedData2, sizeof(expectedData2)) == 0)
  {
    music_loop = 1;
  }
  else if (data.size() == sizeof(expectedData3) && memcmp(data.data(), expectedData3, sizeof(expectedData3)) == 0)
  {
    if (audio.isRunning() == true)
    {
      usleep(100);
      if (audio.isRunning() == true)
      {
        audio.stopSong();
        if (current_file_index < file_name_list.size() - 1)
        {
          current_file_index++; // 循环播放下一首音乐
        }
        else
        {
          current_file_index = 0;
        }
        std::string music_name = file_name_list[current_file_index];
        music_name = "/" + music_name;
        audio.connecttoFS(SD, music_name.c_str());
        Serial.println("play next music");
      }
    }
  }
  else if (data.size() == sizeof(expectedData4) && memcmp(data.data(), expectedData4, sizeof(expectedData4)) == 0)
  {
    if (audio.isRunning() == true)
    {
      usleep(100);
      if (audio.isRunning() == true)
      {
        audio.stopSong();
        current_file_index = current_file_index - 1; // 播放上一首音乐
        if (current_file_index < 0)
        {
          current_file_index = file_name_list.size() - 1;
        }
        std::string music_name = file_name_list[current_file_index];
        music_name = "/" + music_name;
        audio.connecttoFS(SD, music_name.c_str());
        Serial.println("play previous music");
      }
    }
  }
  else if (data.size() == sizeof(expectedData5) && memcmp(data.data(), expectedData5, sizeof(expectedData5)) == 0)
  {
  }
  else if (data.size() == sizeof(expectedData6) && memcmp(data.data(), expectedData6, sizeof(expectedData6)) == 0)
  {
  }
  else if (data.size() == sizeof(expectedData7) && memcmp(data.data(), expectedData7, sizeof(expectedData7)) == 0)
  {
  }
  else if (data.size() == sizeof(expectedData8) && memcmp(data.data(), expectedData8, sizeof(expectedData8)) == 0)
  {
  }
  else if (data.size() == sizeof(expectedData9) && memcmp(data.data(), expectedData9, sizeof(expectedData9)) == 0)
  {
    if (audio.isRunning() == true)
    {
      usleep(100);
      if (audio.isRunning() == true)
      {
        if (audio.pauseResume() == true)
        {
          Serial.println("pause music");
        }
      }
    }
  }
  else if (data.size() == sizeof(expectedData10) && memcmp(data.data(), expectedData10, sizeof(expectedData10)) == 0)
  {
    if (audio.isRunning() == false)
    {
      usleep(100);
      if (audio.isRunning() == false)
      {
        if (audio.pauseResume() == true)
        {
          Serial.println("resume music");
        }
      }
    }
  }
  else if (data.size() == sizeof(expectedData11) && memcmp(data.data(), expectedData11, sizeof(expectedData11)) == 0)
  {
    if (audio.isRunning() == true)
    {
      usleep(100);
      if (audio.isRunning() == true)
      {
        audio.stopSong();
        Serial.println("stop music");
      }
    }
  }
  else if (data.size() == sizeof(expectedData12) && memcmp(data.data(), expectedData12, sizeof(expectedData12)) == 0)
  {
    if (audio.isRunning() == false)
    {
      usleep(100);
      if (file_name_list.size() > 0 && audio.isRunning() == false)
      {
        if (current_file_index >= file_name_list.size())
        {
          current_file_index = 0; // 从头开始播放
        }
        Serial.printf("play music file:%s \r\n", file_name_list[current_file_index].c_str());
        std::string music_name = file_name_list[current_file_index];
        music_name = "/" + music_name;
        audio.connecttoFS(SD, music_name.c_str());
      }
    }
  }
  else if (data.size() == sizeof(expectedData13) && memcmp(data.data(), expectedData13, sizeof(expectedData13)) == 0)
  {
    music_loop = 2;
  }
  else if (data.size() == sizeof(expectedData14) && memcmp(data.data(), expectedData14, sizeof(expectedData14)) == 0)
  {
  }
  else if (data.size() == sizeof(expectedData15) && memcmp(data.data(), expectedData15, sizeof(expectedData15)) == 0)
  {
  }
  else
  {
    Serial.println("Unknown data");
  }
}

void loop()
{
  audio.loop();

  if (audio.isRunning() == false)
  {
    usleep(100);
    if (file_name_list.size() > 0 && audio.isRunning() == false && music_loop != 0)
    {
      if (current_file_index != 0)
      {
        Serial.println("play eos to file");
      }
      if (current_file_index >= file_name_list.size())
      {
        current_file_index = 0; // 循环播放所有音乐
      }

      if (music_loop == 1)
      {
        current_file_index = current_file_index - 1; // 逆序播放
        if (current_file_index < 0)
        {
          current_file_index = file_name_list.size() - 1;
        }
        Serial.println("play previous music loop");
      }
      else if(music_loop == -1)
      {
        current_file_index++;
        Serial.println("play next music loop");
      }else if(music_loop == 2)
      {
        Serial.println("single loop");
        // 单曲循环
      }

      Serial.printf("play music file:%s \r\n", file_name_list[current_file_index].c_str());
      std::string music_name = file_name_list[current_file_index];
      music_name = "/" + music_name;
      audio.connecttoFS(SD, music_name.c_str());
    }
  }

  if (Serial2.available())
  {
    uint8_t data = Serial2.read();
    // Serial.printf("[%d]Received data: %02X\n", __LINE__, data);
    if (data == START_BYTE)
    {
      receiving = true;
      buffer.clear();
    }
    else if (data == END_BYTE)
    {
      receiving = false;
      if (!buffer.empty())
      {
        handleData(buffer);
      }
    }
    else if (receiving)
    {
      buffer.push_back(data);
    }
  }
}
