#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
e4
#define PI_F 3.1415f

#define PCA9685_ADDR 0x40
#define I2C_SDA 14
#define I2C_SCL 15

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDR);

static int angle_to_pulse_rad(float angle_rad)
{
    float pulse = 187.392f * (0.5f + 2.0f * angle_rad / PI_F);
    if (pulse < 0.0f) pulse = 0.0f;
    if (pulse > 4095.0f) pulse = 4095.0f;
    return (int)pulse;
}

static int deg_to_pulse(float deg)
{
    if (deg < 0.0f) deg = 0.0f;
    if (deg > 180.0f) deg = 180.0f;
    float rad = deg * PI_F / 180.0f;
    return angle_to_pulse_rad(rad);
}

static void set_all_channels_deg(float deg)
{
    int pulse = deg_to_pulse(deg);
    for (int ch = 0; ch < 16; ch++) {
        pwm.setPWM(ch, 0, pulse);
    }
}

void setup()
{
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000);
    pwm.begin();
    pwm.setPWMFreq(50);
}

void loop()
{
    static bool pos90 = true;
    set_all_channels_deg(pos90 ? 90.0f : 135.0f);
    pos90 = !pos90;
    delay(1000);
}
