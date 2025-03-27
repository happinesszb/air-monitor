/***************************************************
  1.3TFT  ST7789 IPS SPI display.
  接线
 TFT_DC    8
 TFT_RST   9  
 TFT_SDA   11    
 TFT_SCL 13    
 GND 
 VCC   5V/3.3V 都行
 BLK不用接
 ****************************************************/

#include <Adafruit_GFX.h>    // Core graphics library by Adafruit
#include "Arduino_ST7789.h" // Hardware-specific library for ST7789 (with or without CS pin)
#include <SPI.h>
#include <SoftwareSerial.h>
#include <avr/wdt.h> // 看门狗库

// TFT显示屏配置
#define TFT_DC    8
#define TFT_RST   9 
#define TFT_MOSI  11   // for hardware SPI data pin (all of available pins)
#define TFT_SCLK  13   // for hardware SPI sclk pin (all of available pins)

// 传感器串口配置
#define RX_PIN 3    // 传感器TX接Arduino D3
#define TX_PIN 2    // 传感器RX接Arduino D2
SoftwareSerial sensorSerial(RX_PIN, TX_PIN);

// 协议相关定义
#define FRAME_HEADER 0x3C  // 帧头固定值
#define VERSION      0x02  // 协议版本

// 串口接收缓冲区
unsigned char uartBuffer[17];
unsigned char bufferIndex = 0;

// 传感器数据变量
unsigned short voc = 0;
unsigned short jq = 0; 
unsigned short pm25 = 0;
boolean dataUpdated = false;

Arduino_ST7789 tft = Arduino_ST7789(TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK); //for display without CS pin

float p = 3.1415926;

void setup(void) {
  wdt_disable();  // 禁用看门狗
  delay(100);     // 等待硬件稳定
  Serial.begin(115200);
  while (!Serial) {
    ; // 等待串口连接[1](@ref)
  }
  Serial.println("System Ready");
  sensorSerial.begin(9600); // 传感器串口
  
  Serial.println("Sensor and TFT Display Test Started");

  tft.init(240, 240);   // initialize a ST7789 chip, 240x240 pixels
  Serial.println("TFT Display Initialized");
  
  // 初始化屏幕
  tft.fillScreen(BLACK);
  
  // 显示标题
  tft.setTextColor(YELLOW);
  tft.setTextSize(2);
  tft.setCursor(20, 10);
  tft.println("Air monitor: ");
  
  // 显示标签
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 50);
  tft.println("VOC:");
  
  tft.setCursor(10, 100);
  tft.println("HCHO:");
  
  tft.setCursor(10, 150);
  tft.println("PM2.5:");
}

void loop() {
  wdt_reset(); // 喂狗防止重启
  // 读取传感器数据
  readSensorData();
  
  // 如果有新数据，更新显示
  if (dataUpdated) {
    updateDisplay();
    dataUpdated = false;
  }
}

// 读取传感器数据
void readSensorData() {
  if (sensorSerial.available()) {
    unsigned char incomingByte = sensorSerial.read();
    
    // 帧头检测（0x3C）
    if (bufferIndex == 0 && incomingByte == FRAME_HEADER) {
      uartBuffer[bufferIndex++] = incomingByte;
    } 
    else if (bufferIndex > 0) {
      uartBuffer[bufferIndex++] = incomingByte;
      
      // 完整接收一帧（17字节）
      if (bufferIndex >= 17) {
        bufferIndex = 0;
        
        // 校验和计算（前16字节和取低8位）
        unsigned char checksum = 0;
        for (int i=0; i<16; i++) checksum += uartBuffer[i];
        checksum &= 0xFF;  // 取低8位
        
        // 校验通过（帧头0x3C，版本0x02）
        if (checksum == uartBuffer[16] && uartBuffer[1] == VERSION) {
          // 解析数据
          voc = (uartBuffer[6] << 8) | uartBuffer[7];  // B7+B8
          jq = (uartBuffer[4] << 8) | uartBuffer[5];   // B5+B6
          pm25 = (uartBuffer[8] << 8) | uartBuffer[9]; // B9+B10
          
          // 串口输出
          Serial.print("VOC: ");
          Serial.print(voc);
          Serial.print(" ug/m³ | 甲醛: ");
          Serial.print(jq);
          Serial.print(" ug/m³ | PM2.5: ");
          Serial.print(pm25);
          Serial.println(" ug/m³");
          
          dataUpdated = true;  // 标记数据已更新
        } else {
          Serial.println("校验失败！");
        }
      }
    }
  }
}

// 更新显示
void updateDisplay() {
  // 清空数值区域
  tft.fillRect(100, 50, 140, 25, BLACK);
  tft.fillRect(100, 100, 140, 25, BLACK);
  tft.fillRect(100, 150, 140, 25, BLACK);
  
  // VOC标准阈值: 600 ug/m3
  // 显示VOC值
  if (voc > 600) {
    tft.setTextColor(RED); // 超标显示红色
  } else {
    tft.setTextColor(GREEN); // 正常显示绿色
  }
  tft.setTextSize(2);
  tft.setCursor(100, 50);
  tft.print(voc);
  tft.print(" ug/m3");
  
  // 甲醛标准阈值: 100 ug/m3
  // 显示甲醛值
  if (jq > 100) {
    tft.setTextColor(RED); // 超标显示红色
  } else {
    tft.setTextColor(GREEN); // 正常显示绿色
  }
  tft.setCursor(100, 100);
  tft.print(jq);
  tft.print(" ug/m3");
  
  // PM2.5标准阈值: 75 ug/m3
  // 显示PM2.5值
  if (pm25 > 75) {
    tft.setTextColor(RED); // 超标显示红色
  } else {
    tft.setTextColor(GREEN); // 正常显示绿色
  }
  tft.setCursor(100, 150);
  tft.print(pm25);
  tft.print(" ug/m3");
}

void testlines(uint16_t color) {
  tft.fillScreen(BLACK);
  for (int16_t x=0; x < tft.width(); x+=6) {
    tft.drawLine(0, 0, x, tft.height()-1, color);
  }
  for (int16_t y=0; y < tft.height(); y+=6) {
    tft.drawLine(0, 0, tft.width()-1, y, color);
  }

  tft.fillScreen(BLACK);
  for (int16_t x=0; x < tft.width(); x+=6) {
    tft.drawLine(tft.width()-1, 0, x, tft.height()-1, color);
  }
  for (int16_t y=0; y < tft.height(); y+=6) {
    tft.drawLine(tft.width()-1, 0, 0, y, color);
  }

  tft.fillScreen(BLACK);
  for (int16_t x=0; x < tft.width(); x+=6) {
    tft.drawLine(0, tft.height()-1, x, 0, color);
  }
  for (int16_t y=0; y < tft.height(); y+=6) {
    tft.drawLine(0, tft.height()-1, tft.width()-1, y, color);
  }

  tft.fillScreen(BLACK);
  for (int16_t x=0; x < tft.width(); x+=6) {
    tft.drawLine(tft.width()-1, tft.height()-1, x, 0, color);
  }
  for (int16_t y=0; y < tft.height(); y+=6) {
    tft.drawLine(tft.width()-1, tft.height()-1, 0, y, color);
  }
}

void testdrawtext(char *text, uint16_t color) {
  tft.setCursor(0, 0);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

void testfastlines(uint16_t color1, uint16_t color2) {
  tft.fillScreen(BLACK);
  for (int16_t y=0; y < tft.height(); y+=5) {
    tft.drawFastHLine(0, y, tft.width(), color1);
  }
  for (int16_t x=0; x < tft.width(); x+=5) {
    tft.drawFastVLine(x, 0, tft.height(), color2);
  }
}

void testdrawrects(uint16_t color) {
  tft.fillScreen(BLACK);
  for (int16_t x=0; x < tft.width(); x+=6) {
    tft.drawRect(tft.width()/2 -x/2, tft.height()/2 -x/2 , x, x, color);
  }
}

void testfillrects(uint16_t color1, uint16_t color2) {
  tft.fillScreen(BLACK);
  for (int16_t x=tft.width()-1; x > 6; x-=6) {
    tft.fillRect(tft.width()/2 -x/2, tft.height()/2 -x/2 , x, x, color1);
    tft.drawRect(tft.width()/2 -x/2, tft.height()/2 -x/2 , x, x, color2);
  }
}

void testfillcircles(uint8_t radius, uint16_t color) {
  for (int16_t x=radius; x < tft.width(); x+=radius*2) {
    for (int16_t y=radius; y < tft.height(); y+=radius*2) {
      tft.fillCircle(x, y, radius, color);
    }
  }
}

void testdrawcircles(uint8_t radius, uint16_t color) {
  for (int16_t x=0; x < tft.width()+radius; x+=radius*2) {
    for (int16_t y=0; y < tft.height()+radius; y+=radius*2) {
      tft.drawCircle(x, y, radius, color);
    }
  }
}

void testtriangles() {
  tft.fillScreen(BLACK);
  int color = 0xF800;
  int t;
  int w = tft.width()/2;
  int x = tft.height()-1;
  int y = 0;
  int z = tft.width();
  for(t = 0 ; t <= 15; t++) {
    tft.drawTriangle(w, y, y, x, z, x, color);
    x-=4;
    y+=4;
    z-=4;
    color+=100;
  }
}

void testroundrects() {
  tft.fillScreen(BLACK);
  int color = 100;
  int i;
  int t;
  for(t = 0 ; t <= 4; t+=1) {
    int x = 0;
    int y = 0;
    int w = tft.width()-2;
    int h = tft.height()-2;
    for(i = 0 ; i <= 16; i+=1) {
      tft.drawRoundRect(x, y, w, h, 5, color);
      x+=2;
      y+=3;
      w-=4;
      h-=6;
      color+=1100;
    }
    color+=100;
  }
}

void tftPrintTest() {
  tft.setTextWrap(false);
  tft.fillScreen(BLACK);
  tft.setCursor(0, 30);
  tft.setTextColor(RED);
  tft.setTextSize(1);
  tft.println("Hello World!");
  tft.setTextColor(YELLOW);
  tft.setTextSize(2);
  tft.println("Hello World!");
  tft.setTextColor(GREEN);
  tft.setTextSize(3);
  tft.println("Hello World!");
  tft.setTextColor(BLUE);
  tft.setTextSize(4);
  tft.print(1234.567);
  delay(1500);
  tft.setCursor(0, 0);
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);
  tft.setTextSize(0);
  tft.println("Hello World!");
  tft.setTextSize(1);
  tft.setTextColor(GREEN);
  tft.print(p, 6);
  tft.println(" Want pi?");
  tft.println(" ");
  tft.print(8675309, HEX); // print 8,675,309 out in HEX!
  tft.println(" Print HEX!");
  tft.println(" ");
  tft.setTextColor(WHITE);
  tft.println("Sketch has been");
  tft.println("running for: ");
  tft.setTextColor(MAGENTA);
  tft.print(millis() / 1000);
  tft.setTextColor(WHITE);
  tft.print(" seconds.");
}

void mediabuttons() {
  // play
  tft.fillScreen(BLACK);
  tft.fillRoundRect(25, 10, 78, 60, 8, WHITE);
  tft.fillTriangle(42, 20, 42, 60, 90, 40, RED);
  delay(500);
  // pause
  tft.fillRoundRect(25, 90, 78, 60, 8, WHITE);
  tft.fillRoundRect(39, 98, 20, 45, 5, GREEN);
  tft.fillRoundRect(69, 98, 20, 45, 5, GREEN);
  delay(500);
  // play color
  tft.fillTriangle(42, 20, 42, 60, 90, 40, BLUE);
  delay(50);
  // pause color
  tft.fillRoundRect(39, 98, 20, 45, 5, RED);
  tft.fillRoundRect(69, 98, 20, 45, 5, RED);
  // play color
  tft.fillTriangle(42, 20, 42, 60, 90, 40, GREEN);
}
