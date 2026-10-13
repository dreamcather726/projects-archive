#include "lineSensor.h"

uint8_t sensorBuf[DATA_LEN];

// IO底层操作
static void lineLow(uint8_t pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}
static void lineRelease(uint8_t pin) {
  pinMode(pin, INPUT_PULLUP);
}

static bool releaseClock() {
  lineRelease(SCL_PIN);
  unsigned long startTime = micros();
  while (digitalRead(SCL_PIN) == LOW) {
    if (micros() - startTime >= I2C_TIMEOUT_US) return false;
  }
  delayMicroseconds(I2C_DELAY_US);
  return true;
}

void recoverI2CBus() {
  lineRelease(SDA_PIN);
  lineRelease(SCL_PIN);
  delayMicroseconds(50);
  for (uint8_t i = 0; i < 9; i++) {
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

static bool prepareI2CBus() {
  lineRelease(SDA_PIN);
  lineRelease(SCL_PIN);
  delayMicroseconds(50);
  if (digitalRead(SDA_PIN) && digitalRead(SCL_PIN)) return true;
  recoverI2CBus();
  return digitalRead(SDA_PIN) && digitalRead(SCL_PIN);
}

static bool i2cStart() {
  lineRelease(SDA_PIN);
  if (!releaseClock() || !digitalRead(SDA_PIN)) return false;
  lineLow(SDA_PIN);
  delayMicroseconds(I2C_DELAY_US);
  lineLow(SCL_PIN);
  delayMicroseconds(I2C_DELAY_US);
  return true;
}

static bool i2cStop() {
  lineLow(SDA_PIN);
  delayMicroseconds(I2C_DELAY_US);
  if (!releaseClock()) return false;
  lineRelease(SDA_PIN);
  delayMicroseconds(I2C_DELAY_US);
  return digitalRead(SDA_PIN);
}

static bool i2cWriteByte(uint8_t val) {
  for (int i = 7; i >= 0; i--) {
    lineLow(SCL_PIN);
    if (val & (1 << i)) lineRelease(SDA_PIN);
    else lineLow(SDA_PIN);
    delayMicroseconds(I2C_DELAY_US);
    if (!releaseClock()) return false;
    lineLow(SCL_PIN);
    delayMicroseconds(I2C_DELAY_US);
  }
  lineRelease(SDA_PIN);
  if (!releaseClock()) return false;
  bool ack = (digitalRead(SDA_PIN) == LOW);
  lineLow(SCL_PIN);
  lineRelease(SDA_PIN);
  return ack;
}

static bool i2cReadByte(uint8_t &val, bool sendAck) {
  val = 0;
  lineRelease(SDA_PIN);
  for (uint8_t i = 0; i < 8; i++) {
    lineLow(SCL_PIN);
    delayMicroseconds(I2C_DELAY_US);
    if (!releaseClock()) return false;
    val = val << 1;
    if (digitalRead(SDA_PIN)) val |= 1;
    lineLow(SCL_PIN);
    delayMicroseconds(I2C_DELAY_US);
  }
  sendAck ? lineLow(SDA_PIN) : lineRelease(SDA_PIN);
  delayMicroseconds(I2C_DELAY_US);
  if (!releaseClock()) return false;
  lineLow(SCL_PIN);
  lineRelease(SDA_PIN);
  return true;
}

static void abortI2C() {
  i2cStop();
  recoverI2CBus();
}

static bool i2cWriteData(uint8_t devAddr, const uint8_t *buf, uint8_t len) {
  if (!prepareI2CBus() || !i2cStart()) {
    recoverI2CBus();
    return false;
  }
  if (!i2cWriteByte(devAddr << 1)) {
    abortI2C();
    return false;
  }
  for (uint8_t i = 0; i < len; i++) {
    if (!i2cWriteByte(buf[i])) {
      abortI2C();
      return false;
    }
  }
  i2cStop();
  return true;
}

static bool i2cReadData(uint8_t devAddr, uint8_t *buf, uint8_t len) {
  if (!prepareI2CBus() || !i2cStart()) {
    recoverI2CBus();
    return false;
  }
  if (!i2cWriteByte((devAddr << 1) | 1)) {
    abortI2C();
    return false;
  }
  for (uint8_t i = 0; i < len; i++) {
    if (!i2cReadByte(buf[i], i != len - 1)) {
      abortI2C();
      return false;
    }
  }
  i2cStop();
  return true;
}

bool sensorProbe() {
  if (!prepareI2CBus()) return false;
  if (!i2cStart()) {
    recoverI2CBus();
    return false;
  }
  bool ack = i2cWriteByte(LINE_ADDR << 1);
  i2cStop();
  return ack;
}

bool readLineSensor(uint16_t adcBuf[8], uint8_t stateBuf[8]) {
  uint8_t cmd = CMD_READ;
  if (!i2cWriteData(LINE_ADDR, &cmd, 1)) return false;
  delay(10);
  if (!i2cReadData(LINE_ADDR, sensorBuf, DATA_LEN)) return false;

  for (uint8_t i = 0; i < 8; i++) {
    adcBuf[i] = sensorBuf[16 + i * 2] * 256 + sensorBuf[16 + i * 2 + 1];
    if (HIGH_IS_ONE) {
      stateBuf[i] = adcBuf[i] >= THRESHOLD ? 1 : 0;
    } else {
      stateBuf[i] = adcBuf[i] < THRESHOLD ? 1 : 0;
    }
  }
  return true;
}