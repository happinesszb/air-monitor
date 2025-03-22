#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

#define RX_PIN 3
#define TX_PIN 2
SoftwareSerial sensorSerial(RX_PIN, TX_PIN);

// 数据结构
struct SensorData {
  unsigned short VOC;
  unsigned short JQ;
  unsigned short PM25;  // 新增PM2.5字段
} airData;

// 超标阈值（根据实际标准调整）
#define VOC_THRESHOLD 500    // VOC阈值(ug/m³)
#define JQ_THRESHOLD 80      // 甲醛阈值(ug/m³)
#define PM25_THRESHOLD 35    // PM2.5阈值(ug/m³)

unsigned char uartBuffer[17];
unsigned char bufferIndex = 0;

void setup() {
  Serial.begin(9600);
  sensorSerial.begin(9600);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED Failed");
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Air Quality Monitor");
  display.display();
  delay(1000);
}

void loop() {
  if (sensorSerial.available()) {
    unsigned char incomingByte = sensorSerial.read();
    
    if (bufferIndex == 0 && incomingByte == 0x3C) {
      uartBuffer[bufferIndex++] = incomingByte;
    } 
    else if (bufferIndex > 0) {
      uartBuffer[bufferIndex++] = incomingByte;
      
      if (bufferIndex >= 17) {
        bufferIndex = 0;
        unsigned char checksum = 0;
        for (int i = 0; i < 16; i++) checksum += uartBuffer[i];
        checksum &= 0xFF;
        
        if (checksum == uartBuffer[16] && uartBuffer[1] == 0x02) {
          // 解析数据
          airData.VOC = (uartBuffer[6] << 8) | uartBuffer[7];  // B7+B8
          airData.JQ = (uartBuffer[4] << 8) | uartBuffer[5];   // B5+B6
          airData.PM25 = (uartBuffer[8] << 8) | uartBuffer[9]; // B9+B10
          
          // OLED显示
          display.clearDisplay();
          display.setTextSize(1);
          
          // 显示VOC（带超标反色）
          display.setCursor(0, 0);
          display.print("VOC:");
          if (airData.VOC > VOC_THRESHOLD) display.invertDisplay(true);
          display.setTextSize(2);
          display.print(airData.VOC);
          display.setTextSize(1);
          display.print(" ug/m3");
          display.invertDisplay(false); // 恢复默认

          // 显示甲醛
          display.setCursor(0, 22);
          display.print("JQ:");
          if (airData.JQ > JQ_THRESHOLD) display.invertDisplay(true);
          display.setTextSize(2);
          display.print(airData.JQ);
          display.setTextSize(1);
          display.print(" ug/m3");
          display.invertDisplay(false);

          // 显示PM2.5
          display.setCursor(0, 44);
          display.print("PM2.5:");
          if (airData.PM25 > PM25_THRESHOLD) display.invertDisplay(true);
          display.setTextSize(2);
          display.print(airData.PM25);
          display.setTextSize(1);
          display.print(" ug/m3");
          display.invertDisplay(false);

          display.display();
        }
      }
    }
  }
}