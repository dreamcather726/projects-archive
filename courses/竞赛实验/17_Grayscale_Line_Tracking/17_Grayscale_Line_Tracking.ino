const uint8_t LEFT_GRAY_PIN = 1;
const uint8_t CENTER_GRAY_PIN = 2;
const uint8_t RIGHT_GRAY_PIN = 3;

const uint16_t BLACK_LINE_THRESHOLD = 1800;

void setup() {
    Serial.begin(115200);
}

void loop() {
    bool leftOnLine = isBlackLineDetected(LEFT_GRAY_PIN);
    bool centerOnLine = isBlackLineDetected(CENTER_GRAY_PIN);
    bool rightOnLine = isBlackLineDetected(RIGHT_GRAY_PIN);

    uint8_t lineState = encodeLineState(leftOnLine, centerOnLine, rightOnLine);

    Serial.print("state=");
    Serial.print(lineState, BIN);
    Serial.print(" action=");
    Serial.println(getTrackAction(lineState));

    delay(100);
}

bool isBlackLineDetected(uint8_t sensorPin) {
    return analogRead(sensorPin) < BLACK_LINE_THRESHOLD;
}

uint8_t encodeLineState(bool leftOnLine, bool centerOnLine, bool rightOnLine) {
    uint8_t lineState = 0;

    if (leftOnLine) {
        lineState |= 0b100;
    }

    if (centerOnLine) {
        lineState |= 0b010;
    }

    if (rightOnLine) {
        lineState |= 0b001;
    }

    return lineState;
}

const char *getTrackAction(uint8_t lineState) {
    if (lineState == 0b010) {
        return "go straight";
    }

    if (lineState == 0b100 || lineState == 0b110) {
        return "turn left";
    }

    if (lineState == 0b001 || lineState == 0b011) {
        return "turn right";
    }

    if (lineState == 0b000) {
        return "search line";
    }

    return "slow down";
}
