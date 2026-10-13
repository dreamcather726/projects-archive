HardwareSerial softSerial(1);

const uint32_t USB_SERIAL_BAUD_RATE = 115200;
const uint32_t SOFT_SERIAL_BAUD_RATE = 9600;

const uint8_t SOFT_SERIAL_RX_PIN = 18;
const uint8_t SOFT_SERIAL_TX_PIN = 17;

const unsigned long STATUS_INTERVAL_MS = 1000;

unsigned long lastStatusTimeMs = 0;

void setup() {
    Serial.begin(USB_SERIAL_BAUD_RATE);
    softSerial.begin(SOFT_SERIAL_BAUD_RATE, SERIAL_8N1, SOFT_SERIAL_RX_PIN, SOFT_SERIAL_TX_PIN);

    Serial.println("串口通信已初始化，等待电脑发送数据...");
    Serial.println("请在串口监视器中输入文字或数字，按回车发送。");
    Serial.println("软串口已初始化，可连接外部串口模块测试。");
}

void loop() {
    handleUsbSerialInput();
    handleSoftSerialInput();
    printStatusMessage();
}

void handleUsbSerialInput() {
    while (Serial.available() > 0) {
        char receivedChar = Serial.read();

        Serial.print("电脑串口已收到：");
        Serial.println(receivedChar);

        softSerial.print("from usb: ");
        softSerial.println(receivedChar);
    }
}

void handleSoftSerialInput() {
    while (softSerial.available() > 0) {
        char receivedChar = softSerial.read();

        Serial.print("软串口已收到：");
        Serial.println(receivedChar);

        softSerial.print("echo: ");
        softSerial.println(receivedChar);
    }
}

void printStatusMessage() {
    unsigned long currentTimeMs = millis();
    if (currentTimeMs - lastStatusTimeMs < STATUS_INTERVAL_MS) {
        return;
    }

    Serial.println("串口正常工作，等待数据...");
    lastStatusTimeMs = currentTimeMs;
}
