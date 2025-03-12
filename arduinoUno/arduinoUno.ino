#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// 定义OLED屏幕参数
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C // I2C地址（0x3C或0x3D）
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

// 软串口配置
#define RX_PIN 3
#define TX_PIN 2
SoftwareSerial sensorSerial(RX_PIN, TX_PIN);

// 数据结构
struct SensorData {
  unsigned short VOC;
  unsigned short JQ;  // 新增甲醛字段
} airData;

// 串口接收缓冲区
unsigned char uartBuffer[17];
unsigned char bufferIndex = 0;

void setup() {
  Serial.begin(9600);
  sensorSerial.begin(9600);
  
  // 初始化OLED屏幕
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED Failed");
    while (1); // 卡死，检查硬件连接
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
    
    // 帧头检测
    if (bufferIndex == 0 && incomingByte == 0x3C) {
      uartBuffer[bufferIndex++] = incomingByte;
    } 
    else if (bufferIndex > 0) {
      uartBuffer[bufferIndex++] = incomingByte;
      
      // 完整接收一帧（17字节）
      if (bufferIndex >= 17) {
        bufferIndex = 0;
        
        // 校验和计算
        unsigned char checksum = 0;
        for (int i = 0; i < 16; i++) checksum += uartBuffer[i];
        checksum &= 0xFF;
        
        // 校验通过
        if (checksum == uartBuffer[16] && uartBuffer[1] == 0x02) {
          // 解析VOC和甲醛数据
          airData.VOC = (uartBuffer[6] << 8) | uartBuffer[7];  // B7+B8
          airData.JQ = (uartBuffer[4] << 8) | uartBuffer[5];   // B5+B6
          
          // 串口输出
          Serial.print("VOC: ");
          Serial.print(airData.VOC);
          Serial.print(" ug/m³ | JQ: ");
          Serial.print(airData.JQ);
          Serial.println(" ug/m³");
          
          // OLED显示
          display.clearDisplay();
          display.setTextSize(1);
          display.setCursor(0, 0);
          display.print("VOC: ");
          display.setTextSize(2);  // 放大数值
          display.print(airData.VOC);
          display.setTextSize(1);
          display.print(" ug/m³");
          
          // 显示甲醛数据
          display.setCursor(0, 30);  // 下移30像素
          display.setTextSize(1);
          display.print("JQ: ");
          display.setTextSize(2);
          display.print(airData.JQ);
          display.setTextSize(1);
          display.print(" ug/m³");
          
          display.display();
        }
      }
    }
  }
}