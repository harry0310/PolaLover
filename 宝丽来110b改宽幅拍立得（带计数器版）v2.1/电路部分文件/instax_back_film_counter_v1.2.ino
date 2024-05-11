#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <Math.h>
//#include "Prescaler.h"


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// 剩(0) 余(1) ：(2)
static const unsigned char PROGMEM word_sheng[] =
{

  0x07, 0x83, 0x7C, 0x03, 0x0C, 0x03, 0xFF, 0xC3, 0x3F, 0x1B, 0x3F, 0xDB, 0xFF, 0x9B, 0x3F, 0xDB,
  0x7F, 0xDB, 0xFD, 0xDB, 0x1E, 0x1B, 0x3F, 0x1B, 0x6D, 0x83, 0xCC, 0xC3, 0x0C, 0x0F, 0x0C, 0x06,//剩0
};

static const unsigned char PROGMEM word_yu[] =
{

  0x01, 0x80, 0x01, 0x80, 0x03, 0xC0, 0x06, 0x60, 0x0C, 0x30, 0x18, 0x18, 0x3F, 0xFC, 0xE1, 0x87,
  0x01, 0x80, 0x3F, 0xFC, 0x01, 0x80, 0x19, 0x98, 0x19, 0x8C, 0x31, 0x86, 0x67, 0x86, 0x03, 0x00,//余1
};

static const unsigned char PROGMEM word_mao[] =
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x38, 0x00, 0x38, 0x00, 0x00, 0x00, 0x38, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00,//：2
};

boolean DEBUG_ON=false;//开启debug log

int Pm = 10; //剩余相纸数量
int Tm = 0; //快门次数
int PmPos = 1;
int TmPos = 2; //存储位置

int button1Pin = 5;
int button2Pin = 6;

int button1State = 0;
int button2State = 0;

unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

int buttonState1;             // the current reading from the input pin
int lastButtonState1 = LOW;   // the previous reading from the input pin
unsigned long lastDebounceTime1 = 0;  // the last time the output pin was toggled

float vcc;

long step = 0;

int opening=1;

long shutterCountDelay = 0;

/***************************************
* 常数 时钟频率 参考电流
* CLOCK_PRESCALER_1 16MHZ 7.8mA
* CLOCK_PRESCALER_2 8MHZ 5.4mA
* CLOCK_PRESCALER_4 4MHZ 4.0mA
* CLOCK_PRESCALER_8 2MHZ 3.2mA
* CLOCK_PRESCALER_16 1MHZ 2.6mA
* CLOCK_PRESCALER_32 500KHZ 2.3mA
* CLOCK_PRESCALER_64 250KHZ 2.2mA
* CLOCK_PRESCALER_128 125KHZ 2.1mA
* CLOCK_PRESCALER_256 62.5KHZ 2.1mA
*****************************************/
void setup() {
  Serial.begin(9600);
  pinMode(button1Pin, INPUT);
  pinMode(button2Pin, INPUT);
  digitalWrite(13, LOW);   //Turn the LED off
  
//  setClockPrescaler(CLOCK_PRESCALER_16);

  Wire.begin();

  //相纸数量初始化
  Pm = EEPROM.read(PmPos); //剩余相纸数量
  if (DEBUG_ON)Serial.print("读取内存，剩余相纸：");
  if (DEBUG_ON)Serial.println(Pm);
  if (Pm < 0) {
    Pm = 0;
  } else if (Pm > 11) {
    Pm = 11;
  }
  if (DEBUG_ON)Serial.print("修正，剩余相纸：");
  if (DEBUG_ON)Serial.println(Pm);

  Tm = EEPROM.read(TmPos); //总计数
  if (Tm < 0) {
    Tm = 0;
  }
  if (DEBUG_ON)Serial.print("读取内存，快门总数：");
  if (DEBUG_ON)Serial.println(Tm);
  //电压
  vcc = readVcc() * 0.001f + 1.05f;
  //绘制
  initLed();
}

void initLed() {
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    if (DEBUG_ON)Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000);
  display.clearDisplay(); // Clear the buffer

  drawInfo();
}

void drawInfo() {
  display.clearDisplay();
  //剩余相纸
  int startX = 30;
  int startY = 45;
  display.drawBitmap(0 + startX, startY, word_sheng, 16, 16, 1);
  display.drawBitmap(18 + startX, startY, word_yu, 16, 16, 1);
  display.drawBitmap(36 + startX, startY, word_mao, 16, 16, 1);

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.cp437(true);

  display.setCursor(46 + startX, startY);
  if (Pm < 0) {
    Pm = 0;
  } else if (Pm > 11) {
    Pm = 11;
  }
  if(Pm==11){
    if(opening==1){
      display.print("+");
    }else{
      display.print("-");
    }
  }else{
    display.print(Pm);
  }
  if (DEBUG_ON)Serial.print("绘制相纸：");
  if (DEBUG_ON)Serial.println(Pm);
  //电池
  startX = 28;
  startY = 24;
  //显示电压
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.cp437(true);
  display.setCursor(startX + 2, startY + 3);
  if (shutterCountDelay > 0) {
    //    display.print("st: ");
    display.print(Tm);
  } else {
    display.print(vcc);
    display.print("v");
  }
  //电池框
  display.drawRect(startX + 50, startY + 3, 15, 9, SSD1306_WHITE);
  display.drawRect(startX + 65, startY + 5, 2, 5, SSD1306_WHITE);
  //电量
  if (vcc > 5.8f) {
    display.fillRect(startX + 52, startY + 5, 3, 5, SSD1306_WHITE);
  }
  if (vcc > 5.9f) {
    display.fillRect(startX + 56, startY + 5, 3, 5, SSD1306_WHITE);
  }
  if (vcc > 6.0f) {
    display.fillRect(startX + 60, startY + 5, 3, 5, SSD1306_WHITE);
  }

  display.display();
}

void loop() {
  //出片(每次出片完毕之前延迟检测)
  if(shutterCountDelay<=0){
    button1State = getButtonState1(button1Pin);
  }

  int lastStatus=opening;
  //开仓门
  opening = digitalRead(button2Pin);
  if(lastStatus!=opening){
       drawInfo();
  }
  if (DEBUG_ON)Serial.print("btn2 = ");
  if (DEBUG_ON)Serial.println(opening);
  if (opening == 1) { //=1表示开门
    if (Pm != 11) {
      Pm = 11;
      EEPROM.write(PmPos, Pm);
      if (DEBUG_ON)Serial.println("相纸归位");
      if (DEBUG_ON)Serial.print("存储并读取，相纸=");
      if (DEBUG_ON)Serial.println(EEPROM.read(PmPos));
      drawInfo();
    }
  }
  //电压
  step++;
  shutterCountDelay--;
  if (shutterCountDelay == 0) {
    vcc = readVcc() * 0.001f + 1.05f;
    drawInfo();
  }

  delay(10);
}

long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1125300L / result; // Back-calculate AVcc in mV
  return result;
}

long microsecondsToCentimeters(long microseconds) {
  return microseconds / 29 / 2;
}

//按快门
int getButtonState1(int buttonPin) {
  // read the state of the switch into a local variable:
  int reading = digitalRead(buttonPin);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState1) {
    // reset the debouncing timer
    lastDebounceTime1 = millis();
  }

  if ((millis() - lastDebounceTime1) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState1) {
      buttonState1 = reading;
      //change state
      if (button1State == HIGH) {
        // 相纸-1
        Pm--;
        if (DEBUG_ON)Serial.print("按快门，相纸=");
        if (DEBUG_ON)Serial.println(Pm);
        EEPROM.write(PmPos, Pm);
        if (DEBUG_ON)Serial.print("存储并读取，相纸=");
        if (DEBUG_ON)Serial.println(EEPROM.read(PmPos));
        //快门+1
        Tm++;
        EEPROM.write(TmPos, Tm);
        if (DEBUG_ON)Serial.print("存储并读取，总数=");
        if (DEBUG_ON)Serial.println(EEPROM.read(TmPos));
        //出片延时
        shutterCountDelay = 600;
        //        drawInfo();
      }
    }
  }

  // save the reading. Next time through the loop, it'll be the lastButtonState1:
  lastButtonState1 = reading;
  //返回按钮状态
  return reading;
}
