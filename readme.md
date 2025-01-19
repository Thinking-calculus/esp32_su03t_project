# su-03t 和 esp32 

## esp devkit v1 
### 硬件设备
esp32 devkit v1 
sd卡读取模块
5w 扬声器
MAX98357 I2S 音频放大模块

su-03t
咪头麦克风
5w扬声器

### 连接表
|esp32 devkit v1|sd 读卡模块|MAX98357 功放I2S模块|su-03t|
|---|---|---|---|
|D5|SD_CS||||
|D23|SPI_MOSI||
|D19|SPI_MISO||
|D18|SPI_SCK|||
|D13||I2S_DOUT||
|D12||I2S_BCLK||
|D14||I2S_LRC||
||VCC(5V)|VIN(5V)|VCC(5V)|
||GND|GND|GND||
|RX2(D16)|||B7||
|TX2(D17)|||B6||

su-03t 需要单独连接对应的喇叭和咪头(红正黑负)
MAX98357 需要单独连接对应喇叭
ESP32 需要单独外接micro usb 作为供电和 serial1 的通信

连接效果如图
![连接示例](https://github.com/Thinking-calculus/esp32_su03t_project/blob/main/%E8%BF%9E%E6%8E%A5%E5%9B%BE.jpg?raw=true)

### 软件
arduino库: [audio编解码库(ESP32-audioI2S Public)](https://github.com/schreibfaul1/ESP32-audioI2S/)
arduino 主控代码: esp32_arduino\control\control.ino
su-03t 语音模块配置文件: su_03t_产品配置.json (导入后选择最新的编译即可),编译配置可以[参考](https://blog.csdn.net/m0_61327546/article/details/145244286)

