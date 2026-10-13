/*
  ESP32-S3 + ST7789显示屏 + 八路巡线模块 + MPU6050 + 电压检测
  Version: v1.2.0
  兼容环境：Arduino IDE 1.8.19-DengFOC、ESP32S3 Dev Module

  ============================================================
  一、接线
  ============================================================

  八路巡线模块与MPU6050共用软件I2C总线：

  八路巡线 +       -> ESP32-S3 3V3
  八路巡线 -       -> ESP32-S3 GND
  八路巡线 D/SDA   -> ESP32-S3 GPIO8
  八路巡线 C/SCL   -> ESP32-S3 GPIO9

  MPU6050 VCC       -> ESP32-S3 3V3
  MPU6050 GND       -> ESP32-S3 GND
  MPU6050 SDA       -> ESP32-S3 GPIO8
  MPU6050 SCL       -> ESP32-S3 GPIO9
  MPU6050 AD0       -> GND

  八路巡线地址：0x27
  MPU6050地址：0x68

  ST7789显示屏使用独立SPI引脚：

  TFT CS            -> GPIO47
  TFT DC            -> GPIO21
  TFT RST           -> 不连接或使用模块自身复位
  TFT SDA/MOSI      -> GPIO20
  TFT SCL/SCK       -> GPIO19

  ============================================================
  二、屏幕显示内容
  ============================================================

  - CH1～CH8：八路巡线模块8个数字状态，只显示0或1
  - ACC X/Y/Z：MPU6050三个方向的加速度原始值
  - VOLTAGE：GPIO14测得并按5倍分压还原后的输入电压
  - 不显示巡线参考值、当前模拟值、温度和陀螺仪

  ============================================================
  三、需要安装的显示库
  ============================================================

  - Adafruit GFX Library
  - Adafruit ST7735 and ST7789 Library

  本程序不使用Wire库
  两个I2C模块统一使用GPIO8、GPIO9软件模拟I2C
*/

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// ============================================================
// ST7789显示屏设置
// ============================================================

const int8_t TFT_CS_PIN  = 47;
const int8_t TFT_DC_PIN  = 21;
const int8_t TFT_RST_PIN = -1;
const int8_t TFT_SDA_PIN = 20;
const int8_t TFT_SCL_PIN = 19;

const uint16_t TFT_NATIVE_WIDTH  = 240;
const uint16_t TFT_NATIVE_HEIGHT = 320;
const uint8_t TFT_ROTATION = 3;

Adafruit_ST7789 screen(
  TFT_CS_PIN,
  TFT_DC_PIN,
  TFT_SDA_PIN,
  TFT_SCL_PIN,
  TFT_RST_PIN
);


// ============================================================
// GPIO14 ADC电压检测设置
// ============================================================

const uint8_t VOLTAGE_SENSOR_PIN = 14;
const float ADC_REFERENCE_VOLTAGE = 3.3F;
const float VOLTAGE_DIVIDER_RATIO = 5.0F;
const float ADC_MAX_VALUE = 4095.0F;

// ============================================================
// 共享软件I2C总线设置
// ============================================================

const uint8_t SDA_PIN = 8;
const uint8_t SCL_PIN = 9;

const uint16_t I2C_DELAY_US = 10;
const uint32_t I2C_TIMEOUT_US = 5000;
const uint16_t READ_INTERVAL_MS = 100;

unsigned long lastReadTime = 0;
unsigned long lastMPURetryTime = 0;

// ============================================================
// 八路巡线模块设置
// ============================================================

const uint8_t LINE_SENSOR_ADDRESS = 0x27;
const uint8_t CMD_READ_LINE_SENSOR = 0x01;
const uint8_t LINE_SENSOR_DATA_LENGTH = 32;
const bool HIGH_VALUE_IS_ONE = true;

uint8_t lineSensorBuffer[LINE_SENSOR_DATA_LENGTH];

struct LineSensorData {
  uint8_t digital;
  uint16_t reference[8];
  uint16_t current[8];
};

// ============================================================
// MPU6050设置
// ============================================================

const uint8_t MPU6050_ADDRESS = 0x68;
const uint8_t REG_ACCEL_XOUT_H = 0x3B;
const uint8_t REG_PWR_MGMT_1   = 0x6B;
const uint8_t REG_WHO_AM_I     = 0x75;

struct MPU6050Data {
  int16_t accX;
  int16_t accY;
  int16_t accZ;

  int16_t temperatureRaw;
  float temperatureC;

  int16_t gyroX;
  int16_t gyroY;
  int16_t gyroZ;
};

bool mpuInitialized = false;

// ============================================================
// 函数声明
// ============================================================

void lineLow(uint8_t pin);
void lineRelease(uint8_t pin);
bool releaseClock();
void recoverI2CBus();
bool prepareI2CBus();
bool i2cStart();
bool i2cStop();
bool i2cWriteByte(uint8_t value);
bool i2cReadByte(uint8_t &value, bool sendAck);
void abortI2C();

bool probeI2CDevice(uint8_t deviceAddress);
bool i2cWriteData(uint8_t deviceAddress, const uint8_t *data, uint8_t length);
bool i2cReadData(uint8_t deviceAddress, uint8_t *data, uint8_t length);
bool i2cReadRegisters(uint8_t deviceAddress, uint8_t startRegister, uint8_t *data, uint8_t length);
bool i2cWriteRegister(uint8_t deviceAddress, uint8_t registerAddress, uint8_t value);
bool i2cReadRegister(uint8_t deviceAddress, uint8_t registerAddress, uint8_t &value);

bool readLineSensor(LineSensorData &data);
void parseLineSensorData(LineSensorData &data);
uint8_t calculateLineDigitalValue(const LineSensorData &data);

bool initializeMPU6050();
bool readMPU6050(MPU6050Data &data);
int16_t combineBytes(uint8_t highByte, uint8_t lowByte);

void drawStaticScreen();
void updateLineDisplay(const LineSensorData &data, bool readSuccess);
void updateMPUDisplay(const MPU6050Data &data, bool readSuccess);
float readInputVoltage(uint16_t &adcValue, float &pinVoltage);
void updateVoltageDisplay(float inputVoltage, bool readSuccess);
void printCompactSerial(
  const LineSensorData &lineData,
  bool lineSuccess,
  const MPU6050Data &mpuData,
  bool mpuSuccess,
  uint16_t adcValue,
  float inputVoltage
);

// ============================================================
// 软件I2C基础控制
// ============================================================

void lineLow(uint8_t pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}

void lineRelease(uint8_t pin) {
  pinMode(pin, INPUT_PULLUP);
}

bool releaseClock() {
  lineRelease(SCL_PIN);

  unsigned long startTime = micros();

  while (digitalRead(SCL_PIN) == LOW) {
    if (micros() - startTime >= I2C_TIMEOUT_US) {
      return false;
    }
  }

  delayMicroseconds(I2C_DELAY_US);
  return true;
}

void recoverI2CBus() {
  lineRelease(SDA_PIN);
  lineRelease(SCL_PIN);
  delayMicroseconds(50);

  for (uint8_t count = 0; count < 9; count++) {
    lineLow(SCL_PIN);
    delayMicroseconds(20);

    lineRelease(SCL_PIN);
    delayMicroseconds(20);
  }

  lineLow(SDA_PIN);
  delayMicroseconds(20);

  lineRelease(SCL_PIN);
  delayMicroseconds(20);

  lineRelease(SDA_PIN);
  delayMicroseconds(20);
}

bool prepareI2CBus() {
  lineRelease(SDA_PIN);
  lineRelease(SCL_PIN);
  delayMicroseconds(50);

  if (
    digitalRead(SDA_PIN) == HIGH &&
    digitalRead(SCL_PIN) == HIGH
  ) {
    return true;
  }

  recoverI2CBus();

  return
    digitalRead(SDA_PIN) == HIGH &&
    digitalRead(SCL_PIN) == HIGH;
}

bool i2cStart() {
  lineRelease(SDA_PIN);

  if (!releaseClock()) {
    return false;
  }

  if (digitalRead(SDA_PIN) == LOW) {
    return false;
  }

  lineLow(SDA_PIN);
  delayMicroseconds(I2C_DELAY_US);

  lineLow(SCL_PIN);
  delayMicroseconds(I2C_DELAY_US);

  return true;
}

bool i2cStop() {
  lineLow(SDA_PIN);
  delayMicroseconds(I2C_DELAY_US);

  if (!releaseClock()) {
    return false;
  }

  lineRelease(SDA_PIN);
  delayMicroseconds(I2C_DELAY_US);

  return digitalRead(SDA_PIN) == HIGH;
}

bool i2cWriteByte(uint8_t value) {
  for (int bitIndex = 7; bitIndex >= 0; bitIndex--) {
    lineLow(SCL_PIN);

    if (value & (1 << bitIndex)) {
      lineRelease(SDA_PIN);
    } else {
      lineLow(SDA_PIN);
    }

    delayMicroseconds(I2C_DELAY_US);

    if (!releaseClock()) {
      return false;
    }

    lineLow(SCL_PIN);
    delayMicroseconds(I2C_DELAY_US);
  }

  lineRelease(SDA_PIN);
  delayMicroseconds(I2C_DELAY_US);

  if (!releaseClock()) {
    return false;
  }

  bool acknowledged = digitalRead(SDA_PIN) == LOW;

  lineLow(SCL_PIN);
  delayMicroseconds(I2C_DELAY_US);

  lineRelease(SDA_PIN);
  return acknowledged;
}

bool i2cReadByte(uint8_t &value, bool sendAck) {
  value = 0;
  lineRelease(SDA_PIN);

  for (uint8_t bitIndex = 0; bitIndex < 8; bitIndex++) {
    lineLow(SCL_PIN);
    delayMicroseconds(I2C_DELAY_US);

    if (!releaseClock()) {
      return false;
    }

    value <<= 1;

    if (digitalRead(SDA_PIN) == HIGH) {
      value |= 1;
    }

    lineLow(SCL_PIN);
    delayMicroseconds(I2C_DELAY_US);
  }

  if (sendAck) {
    lineLow(SDA_PIN);
  } else {
    lineRelease(SDA_PIN);
  }

  delayMicroseconds(I2C_DELAY_US);

  if (!releaseClock()) {
    return false;
  }

  lineLow(SCL_PIN);
  delayMicroseconds(I2C_DELAY_US);

  lineRelease(SDA_PIN);
  return true;
}

void abortI2C() {
  i2cStop();
  recoverI2CBus();
}

// ============================================================
// 通用I2C通信函数
// ============================================================

bool probeI2CDevice(uint8_t deviceAddress) {
  if (!prepareI2CBus()) {
    return false;
  }

  if (!i2cStart()) {
    recoverI2CBus();
    return false;
  }

  uint8_t writeAddress = deviceAddress << 1;
  bool acknowledged = i2cWriteByte(writeAddress);

  i2cStop();
  return acknowledged;
}

bool i2cWriteData(uint8_t deviceAddress, const uint8_t *data, uint8_t length) {
  if (data == NULL || length == 0 || !prepareI2CBus()) {
    return false;
  }

  if (!i2cStart()) {
    recoverI2CBus();
    return false;
  }

  uint8_t writeAddress = deviceAddress << 1;

  if (!i2cWriteByte(writeAddress)) {
    abortI2C();
    return false;
  }

  for (uint8_t index = 0; index < length; index++) {
    if (!i2cWriteByte(data[index])) {
      abortI2C();
      return false;
    }
  }

  if (!i2cStop()) {
    recoverI2CBus();
    return false;
  }

  return true;
}

bool i2cReadData(uint8_t deviceAddress, uint8_t *data, uint8_t length) {
  if (data == NULL || length == 0 || !prepareI2CBus()) {
    return false;
  }

  if (!i2cStart()) {
    recoverI2CBus();
    return false;
  }

  uint8_t readAddress = (deviceAddress << 1) | 1;

  if (!i2cWriteByte(readAddress)) {
    abortI2C();
    return false;
  }

  for (uint8_t index = 0; index < length; index++) {
    bool sendAck = index < length - 1;

    if (!i2cReadByte(data[index], sendAck)) {
      abortI2C();
      return false;
    }
  }

  if (!i2cStop()) {
    recoverI2CBus();
    return false;
  }

  return true;
}

bool i2cReadRegisters(uint8_t deviceAddress, uint8_t startRegister, uint8_t *data, uint8_t length) {
  if (data == NULL || length == 0 || !prepareI2CBus()) {
    return false;
  }

  if (!i2cStart()) {
    recoverI2CBus();
    return false;
  }

  uint8_t writeAddress = deviceAddress << 1;

  if (!i2cWriteByte(writeAddress)) {
    abortI2C();
    return false;
  }

  if (!i2cWriteByte(startRegister)) {
    abortI2C();
    return false;
  }

  if (!i2cStart()) {
    abortI2C();
    return false;
  }

  uint8_t readAddress = (deviceAddress << 1) | 1;

  if (!i2cWriteByte(readAddress)) {
    abortI2C();
    return false;
  }

  for (uint8_t index = 0; index < length; index++) {
    bool sendAck = index < length - 1;

    if (!i2cReadByte(data[index], sendAck)) {
      abortI2C();
      return false;
    }
  }

  if (!i2cStop()) {
    recoverI2CBus();
    return false;
  }

  return true;
}

bool i2cWriteRegister(uint8_t deviceAddress, uint8_t registerAddress, uint8_t value) {
  uint8_t writeBuffer[2];

  writeBuffer[0] = registerAddress;
  writeBuffer[1] = value;

  return i2cWriteData(deviceAddress, writeBuffer, 2);
}

bool i2cReadRegister(uint8_t deviceAddress, uint8_t registerAddress, uint8_t &value) {
  return i2cReadRegisters(deviceAddress, registerAddress, &value, 1);
}

// ============================================================
// 八路巡线模块
// ============================================================

bool readLineSensor(LineSensorData &data) {
  uint8_t command = CMD_READ_LINE_SENSOR;

  if (!i2cWriteData(LINE_SENSOR_ADDRESS, &command, 1)) {
    return false;
  }

  delay(10);

  if (!i2cReadData(
        LINE_SENSOR_ADDRESS,
        lineSensorBuffer,
        LINE_SENSOR_DATA_LENGTH
      )) {
    return false;
  }

  parseLineSensorData(data);
  return true;
}

void parseLineSensorData(LineSensorData &data) {
  uint8_t position = 0;

  for (uint8_t index = 0; index < 8; index++) {
    uint8_t highByte = lineSensorBuffer[position++];
    uint8_t lowByte = lineSensorBuffer[position++];

    data.reference[index] =
      ((uint16_t)highByte << 8) |
      lowByte;
  }

  for (uint8_t index = 0; index < 8; index++) {
    uint8_t highByte = lineSensorBuffer[position++];
    uint8_t lowByte = lineSensorBuffer[position++];

    data.current[index] =
      ((uint16_t)highByte << 8) |
      lowByte;
  }

  data.digital = calculateLineDigitalValue(data);
}

uint8_t calculateLineDigitalValue(const LineSensorData &data) {
  uint8_t digitalValue = 0;

  for (uint8_t index = 0; index < 8; index++) {
    bool detected;

    if (HIGH_VALUE_IS_ONE) {
      detected =
        data.current[index] >=
        data.reference[index];
    } else {
      detected =
        data.current[index] <
        data.reference[index];
    }

    if (detected) {
      digitalValue |= (1 << index);
    }
  }

  return digitalValue;
}

// ============================================================
// MPU6050
// ============================================================

int16_t combineBytes(uint8_t highByte, uint8_t lowByte) {
  uint16_t value =
    ((uint16_t)highByte << 8) |
    lowByte;

  return (int16_t)value;
}

bool initializeMPU6050() {
  uint8_t whoAmI = 0;

  if (!probeI2CDevice(MPU6050_ADDRESS)) {
    return false;
  }

  if (!i2cReadRegister(
        MPU6050_ADDRESS,
        REG_WHO_AM_I,
        whoAmI
      )) {
    return false;
  }

  Serial.print("MPU6050 WHO_AM_I = 0x");

  if (whoAmI < 0x10) {
    Serial.print("0");
  }

  Serial.println(whoAmI, HEX);

  if (!i2cWriteRegister(
        MPU6050_ADDRESS,
        REG_PWR_MGMT_1,
        0x00
      )) {
    return false;
  }

  delay(100);
  return true;
}

bool readMPU6050(MPU6050Data &data) {
  uint8_t rawData[14];

  if (!i2cReadRegisters(
        MPU6050_ADDRESS,
        REG_ACCEL_XOUT_H,
        rawData,
        14
      )) {
    return false;
  }

  data.accX = combineBytes(rawData[0], rawData[1]);
  data.accY = combineBytes(rawData[2], rawData[3]);
  data.accZ = combineBytes(rawData[4], rawData[5]);

  data.temperatureRaw = combineBytes(rawData[6], rawData[7]);

  data.gyroX = combineBytes(rawData[8], rawData[9]);
  data.gyroY = combineBytes(rawData[10], rawData[11]);
  data.gyroZ = combineBytes(rawData[12], rawData[13]);

  data.temperatureC =
    data.temperatureRaw / 340.0F +
    36.53F;

  return true;
}

// ============================================================
// GPIO14 ADC电压读取
// ============================================================

float readInputVoltage(
  uint16_t &adcValue,
  float &pinVoltage
) {
  adcValue = analogRead(VOLTAGE_SENSOR_PIN);

  pinVoltage =
    adcValue / ADC_MAX_VALUE *
    ADC_REFERENCE_VOLTAGE;

  return
    pinVoltage *
    VOLTAGE_DIVIDER_RATIO;
}

// ============================================================
// ST7789显示
// ============================================================

void drawStaticScreen() {
  screen.fillScreen(ST77XX_BLACK);
  screen.setTextWrap(false);

  // 顶部总标题
  screen.setTextSize(2);
  screen.setTextColor(ST77XX_WHITE);
  screen.setCursor(8, 4);
  screen.print("LINE + MPU + VOLT");

  screen.drawFastHLine(0, 24, 320, ST77XX_BLUE);

  // 八路巡线区域
  screen.setTextSize(2);
  screen.setTextColor(ST77XX_CYAN);
  screen.setCursor(8, 29);
  screen.print("LINE SENSOR");

  for (uint8_t index = 0; index < 8; index++) {
    int16_t boxX = 6 + index * 39;

    screen.setTextSize(1);
    screen.setTextColor(ST77XX_WHITE);
    screen.setCursor(boxX + 7, 49);
    screen.print("CH");
    screen.print(index + 1);

    screen.drawRect(
      boxX,
      59,
      34,
      34,
      ST77XX_WHITE
    );
  }

  screen.drawFastHLine(0, 101, 320, ST77XX_BLUE);

  // MPU6050加速度区域
  screen.setTextSize(2);
  screen.setTextColor(ST77XX_MAGENTA);
  screen.setCursor(8, 107);
  screen.print("MPU6050 ACC RAW");

  screen.setTextSize(1);
  screen.setTextColor(ST77XX_YELLOW);

  screen.setCursor(8, 132);
  screen.print("X");

  screen.setCursor(110, 132);
  screen.print("Y");

  screen.setCursor(212, 132);
  screen.print("Z");

  screen.drawFastHLine(0, 174, 320, ST77XX_BLUE);

  // GPIO14电压区域
  screen.setTextSize(2);
  screen.setTextColor(ST77XX_GREEN);
  screen.setCursor(8, 181);
  screen.print("VOLTAGE G14");

  screen.setTextSize(1);
  screen.setTextColor(ST77XX_WHITE);
  screen.setCursor(8, 224);
  screen.print("INPUT AFTER x5 DIVIDER");
}

void updateLineDisplay(
  const LineSensorData &data,
  bool readSuccess
) {
  if (!readSuccess) {
    screen.fillRect(7, 60, 304, 32, ST77XX_BLACK);

    screen.setTextSize(2);
    screen.setTextColor(ST77XX_RED);
    screen.setCursor(114, 68);
    screen.print("ERROR");
    return;
  }

  for (uint8_t index = 0; index < 8; index++) {
    int16_t boxX = 6 + index * 39;
    uint8_t state =
      (data.digital >> index) & 0x01;

    screen.fillRect(
      boxX + 1,
      60,
      32,
      32,
      ST77XX_BLACK
    );

    screen.setTextSize(3);
    screen.setTextColor(
      state == 1 ?
      ST77XX_GREEN :
      ST77XX_RED
    );

    screen.setCursor(boxX + 9, 65);
    screen.print(state);
  }
}

void updateMPUDisplay(
  const MPU6050Data &data,
  bool readSuccess
) {
  // 三个加速度数值分别放在三个宽度约100像素的区域
  screen.fillRect(8, 143, 92, 24, ST77XX_BLACK);
  screen.fillRect(110, 143, 92, 24, ST77XX_BLACK);
  screen.fillRect(212, 143, 100, 24, ST77XX_BLACK);

  screen.setTextSize(2);

  if (!readSuccess) {
    screen.setTextColor(ST77XX_RED);
    screen.setCursor(106, 145);
    screen.print("MPU ERROR");
    return;
  }

  screen.setTextColor(ST77XX_WHITE);

  screen.setCursor(8, 145);
  screen.print(data.accX);

  screen.setCursor(110, 145);
  screen.print(data.accY);

  screen.setCursor(212, 145);
  screen.print(data.accZ);
}

void updateVoltageDisplay(
  float inputVoltage,
  bool readSuccess
) {
  // 只刷新电压数值区域，避免整个屏幕闪烁
  screen.fillRect(154, 180, 158, 40, ST77XX_BLACK);

  screen.setTextSize(3);

  if (!readSuccess) {
    screen.setTextColor(ST77XX_RED);
    screen.setCursor(172, 190);
    screen.print("ERROR");
    return;
  }

  screen.setTextColor(ST77XX_YELLOW);
  screen.setCursor(158, 190);
  screen.print(inputVoltage, 2);
  screen.print("V");
}

void printCompactSerial(
  const LineSensorData &lineData,
  bool lineSuccess,
  const MPU6050Data &mpuData,
  bool mpuSuccess,
  uint16_t adcValue,
  float inputVoltage
) {
  Serial.print("LINE=");

  if (lineSuccess) {
    for (uint8_t index = 0; index < 8; index++) {
      Serial.print(
        (lineData.digital >> index) &
        0x01
      );
    }
  } else {
    Serial.print("ERROR");
  }

  Serial.print(" | ACC=");

  if (mpuSuccess) {
    Serial.print("X=");
    Serial.print(mpuData.accX);

    Serial.print(" Y=");
    Serial.print(mpuData.accY);

    Serial.print(" Z=");
    Serial.print(mpuData.accZ);
  } else {
    Serial.print("ERROR");
  }

  Serial.print(" | ADC=");
  Serial.print(adcValue);

  Serial.print(" | VOLTAGE=");
  Serial.print(inputVoltage, 2);
  Serial.println("V");
}

// ============================================================
// Arduino初始化
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(1500);

  pinMode(VOLTAGE_SENSOR_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(
    VOLTAGE_SENSOR_PIN,
    ADC_11db
  );

  screen.init(
    TFT_NATIVE_WIDTH,
    TFT_NATIVE_HEIGHT
  );

  screen.setRotation(TFT_ROTATION);
  drawStaticScreen();

  lineRelease(SDA_PIN);
  lineRelease(SCL_PIN);
  recoverI2CBus();

  bool lineFound =
    probeI2CDevice(LINE_SENSOR_ADDRESS);

  mpuInitialized =
    initializeMPU6050();

  LineSensorData emptyLineData = {};
  MPU6050Data emptyMPUData = {};

  updateLineDisplay(
    emptyLineData,
    false
  );

  updateMPUDisplay(
    emptyMPUData,
    false
  );

  Serial.println();
  Serial.println("ESP32-S3 TFT + LINE + MPU6050");

  Serial.print("Line sensor 0x27: ");
  Serial.println(
    lineFound ?
    "FOUND" :
    "NOT FOUND"
  );

  Serial.print("MPU6050 0x68: ");
  Serial.println(
    mpuInitialized ?
    "READY" :
    "NOT READY"
  );
}

// ============================================================
// Arduino主循环
// ============================================================

void loop() {
  unsigned long currentTime = millis();

  if (
    currentTime - lastReadTime <
    READ_INTERVAL_MS
  ) {
    delay(2);
    return;
  }

  lastReadTime = currentTime;

  LineSensorData lineData = {};
  MPU6050Data mpuData = {};

  uint16_t adcValue = 0;
  float pinVoltage = 0.0F;
  float inputVoltage =
    readInputVoltage(
      adcValue,
      pinVoltage
    );

  // 先访问0x27，完成后STOP释放总线
  bool lineSuccess = readLineSensor(lineData);

  if (!lineSuccess) {
    recoverI2CBus();
  }

  // 再访问0x68，两次通信不会重叠
  if (
    !mpuInitialized &&
    currentTime - lastMPURetryTime >= 1000
  ) {
    lastMPURetryTime = currentTime;
    mpuInitialized = initializeMPU6050();
  }

  bool mpuSuccess = false;

  if (mpuInitialized) {
    mpuSuccess = readMPU6050(mpuData);
  }

  if (!mpuSuccess) {
    recoverI2CBus();

    if (
      currentTime - lastMPURetryTime >= 1000
    ) {
      lastMPURetryTime = currentTime;
      mpuInitialized = false;
    }
  }

  updateLineDisplay(
    lineData,
    lineSuccess
  );

  updateMPUDisplay(
    mpuData,
    mpuSuccess
  );

  updateVoltageDisplay(
    inputVoltage,
    true
  );

  printCompactSerial(
    lineData,
    lineSuccess,
    mpuData,
    mpuSuccess,
    adcValue,
    inputVoltage
  );
}
