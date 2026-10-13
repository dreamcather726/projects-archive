#include <Wire.h>//i2c库
#include <Adafruit_PWMServoDriver.h>//舵机库
#include <math.h>//数学库
#include <string.h>

//这是四足狗走直线的程序
#define pi 3.1415

#define L0 15.5 //髋关节长度
#define L1 120 //大腿长度
#define L2 120 //小腿长度

#define STEP 100 //定义一个行走周期的脉冲总数为100

#define PCA9685_ADDR 0x40
#define I2C_SDA 14
#define I2C_SCL 15
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDR);//定义9685

//右前腿舵机通道，1--髋关节舵机，2-大腿舵机，3-小腿舵机；
#define R_f_1  0
#define R_f_2  1
#define R_f_3  2

#define R_b_1  8
#define R_b_2  9
#define R_b_3  10

#define L_f_1  4
#define L_f_2  5
#define L_f_3  6

#define L_b_1  12
#define L_b_2  13
#define L_b_3  14

int ledPin1 = 3;
int ledPin2 = 4;
int ledPin3 = 5;

float startX = -25.0;
float startY = 145.0;

float endX = 10.0;
float endY = 115.0;

float h = 105.0; //h是腿抬起的最高点坐标
float Px[STEP];
float Py[STEP];

int angle_to_pluse(float angle);
void my_90();
void my_go(int j);

static bool manualOverride = false;
static bool stopRequested = false;

static bool read_serial_line(char* out, size_t out_len)
{
    static size_t idx = 0;
    while (Serial.available() > 0) {
        char c = (char)Serial.read();
        if (c == '\r') continue;
        if (c == '\n') {
            if (idx == 0) continue;
            out[idx] = '\0';
            idx = 0;
            return true;
        }
        if (idx < out_len - 1) {
            out[idx++] = c;
        }
    }
    return false;
}

static void send_ok(const char* msg)
{
    Serial.print("OK ");
    Serial.println(msg);
}

static void send_err(const char* msg)
{
    Serial.print("ERR ");
    Serial.println(msg);
}

static void set_servo_deg(int channel, float deg)
{
    if (deg < 0) deg = 0;
    if (deg > 180) deg = 180;
    float rad = deg * pi / 180.0;
    int pulse = angle_to_pluse(rad);
    if (pulse < 0) pulse = 0;
    if (pulse > 4095) pulse = 4095;
    pwm.setPWM(channel, 0, pulse);
}

static void set_servo_pulse(int channel, int pulse)
{
    if (pulse < 0) pulse = 0;
    if (pulse > 4095) pulse = 4095;
    pwm.setPWM(channel, 0, pulse);
}

static void handle_command(char* line)
{
    while (*line == ' ' || *line == '\t') line++;
    if (*line == '\0') return;

    char* cmd = strtok(line, " ,\t");
    if (!cmd) return;

    if ((cmd[0] == 'C' || cmd[0] == 'c') && (cmd[1] == 'H' || cmd[1] == 'h')) {
        char* colon = strchr(cmd, ':');
        if (colon) {
            *colon = '\0';
            int ch = atoi(cmd + 2);
            float deg = (float)atof(colon + 1);
            if (ch < 0 || ch > 15) {
                send_err("channel out of range");
                return;
            }
            manualOverride = true;
            stopRequested = true;
            set_servo_deg(ch, deg);
            send_ok("CH");
            return;
        }
    }

    if (strcmp(cmd, "PING") == 0) {
        Serial.println("PONG");
        return;
    }

    if (strcmp(cmd, "MANUAL") == 0) {
        manualOverride = true;
        stopRequested = true;
        send_ok("MANUAL");
        return;
    }

    if (strcmp(cmd, "STOP") == 0) {
        stopRequested = true;
        send_ok("STOP");
        return;
    }

    if (strcmp(cmd, "ZERO") == 0) {
        manualOverride = true;
        stopRequested = true;
        my_90();
        send_ok("ZERO");
        return;
    }

    if (strcmp(cmd, "AUTO") == 0) {
        char* steps_s = strtok(NULL, " ,\t");
        if (!steps_s) {
            send_err("AUTO needs steps");
            return;
        }
        int steps = atoi(steps_s);
        if (steps <= 0) steps = 1;
        manualOverride = false;
        stopRequested = false;
        my_go(steps);
        if (stopRequested || manualOverride) {
            send_ok("AUTO_STOPPED");
        } else {
            send_ok("AUTO_DONE");
        }
        return;
    }

    if (strcmp(cmd, "SET") == 0) {
        char* ch_s = strtok(NULL, " ,\t");
        char* deg_s = strtok(NULL, " ,\t");
        if (!ch_s || !deg_s) {
            send_err("SET needs channel deg");
            return;
        }
        int ch = atoi(ch_s);
        float deg = (float)atof(deg_s);
        if (ch < 0 || ch > 15) {
            send_err("channel out of range");
            return;
        }
        manualOverride = true;
        stopRequested = true;
        set_servo_deg(ch, deg);
        send_ok("SET");
        return;
    }

    if (strcmp(cmd, "PULSE") == 0) {
        char* ch_s = strtok(NULL, " ,\t");
        char* p_s = strtok(NULL, " ,\t");
        if (!ch_s || !p_s) {
            send_err("PULSE needs channel pulse");
            return;
        }
        int ch = atoi(ch_s);
        int pulse = atoi(p_s);
        if (ch < 0 || ch > 15) {
            send_err("channel out of range");
            return;
        }
        manualOverride = true;
        stopRequested = true;
        set_servo_pulse(ch, pulse);
        send_ok("PULSE");
        return;
    }

    send_err("unknown command");
}

static void process_serial()
{
    char line[96];
    while (read_serial_line(line, sizeof(line))) {
        handle_command(line);
    }
}

void make_path()//生成步态坐标
{
    //着地相,分配P/2个脉冲
    int j = 0;
    for (float i = startX; i < endX; i = i + (endX - startX) / (STEP / 2))
    {
        Px[j] = i;
        Py[j] = startY;
        j++;
        if (j >= STEP / 2) break;
    }
    //腾空相，分配P/2个脉冲
    j = STEP / 2;
    for (float i = endX; i > startX; i = i - (endX - startX) / (STEP / 2))
    {
        Px[j] = i;
        Py[j] = 4 * (startY - h) / pow(endX - startX, 2) * pow(i - (startX + endX) / 2, 2) + h;
        j++;
        if (j >= STEP) break;
    }
}

void setup() //初始化
{
    Serial.begin(115200);
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000);
    pwm.begin();
    pwm.setPWMFreq(50);  // Analog servos run at ~60 Hz updates
    //模拟伺服在50赫兹更新下运行

    make_path();



    //打印PX[150]PY[150]
    const bool debugPrintPath = false;
    if (debugPrintPath) {
        for (int i = 0; i < STEP; i++) {
            Serial.print("i = ");
            Serial.print(i);
            Serial.print(" : Px[i] = ");
            Serial.print(Px[i]);
            Serial.print(" , Py[i] = ");
            Serial.println(Py[i]);
        }
    }

    Serial.println("READY");
}


//存下500个路径点

//0度：0.5/20*4096 = 102
//45度：1/20*4096 = 1/20*4096 = 204 * 0.915 = 187
//90度：1.5/20*4096 = 1.s5/20*4096 = 306 * 0.915 = 280
//135度：2/20*4096 = 2/20*4096 = 408 * 0.915 = 373 
//180度：2.5/20*4096 = 2.5/20*4096 =510

//把角度值转化为4096对应的脉冲值
int angle_to_pluse(float angle)
{
    float pluse;
    pluse = 187.392 * (0.5 + 2 * angle / pi);
    return int(pluse);
}


//已以大腿舵机转动中心为原点建立直角坐标系，x轴向后，y轴向下，把足底坐标值转化为关节角度
//angle_2是小腿关节角度，angle_1是大腿关节角度
//右边两条腿逆解
float R_x_y_to_angle_1(float x, float y)//大腿
{
    float angle_1;
    float angle_2;
    angle_2 = acos((x * x + y * y - L1 * L1 - L2 * L2) / (2 * L1 * L2));
    double judge_expression = 4 * pow(x * (L1 + L2 * cos(angle_2)), 2) - 4 * (x * x + y * y) * (pow(x, 2) - pow(L2 * sin(angle_2), 2));
    angle_1 = acos(((x * (L1 + L2 * cos(angle_2))) + 0.5 * sqrt(judge_expression)) / (x * x + y * y));
    return angle_1 + pi / 6;
}

//小腿
float R_x_y_to_angle_2(float x, float y)
{
    float angle_1;
    float angle_2;
    angle_2 = acos((x * x + y * y - L1 * L1 - L2 * L2) / (2 * L1 * L2));
    return angle_2 - pi / 6;
}
//左边两条腿逆解
float L_x_y_to_angle_1(float x, float y)
{
    float right_angle = R_x_y_to_angle_1(x, y);
    // 转换为镜像角度：pi - angle
    return pi - right_angle;
}
//小腿
float L_x_y_to_angle_2(float x, float y)
{
    float right_angle = R_x_y_to_angle_2(x, y);
    // 转换为镜像角度：pi - angle
    return pi - right_angle;
}


//对角步态
void my_go(int j)
{
    while (j--) {
        process_serial();
        if (stopRequested || manualOverride) break;
        delay(3);//髋关节电机90度
        pwm.setPWM(R_f_1, 0, angle_to_pluse(pi / 2));
        pwm.setPWM(R_b_1, 0, angle_to_pluse(pi / 2));
        pwm.setPWM(L_f_1, 0, angle_to_pluse(pi / 2));
        pwm.setPWM(L_b_1, 0, angle_to_pluse(pi / 2));

        //对角的腿动作相同，且腾空相和着地相交替轮换
        for (int i = 0; i < STEP / 2; i++)
        {
            process_serial();
            if (stopRequested || manualOverride) return;
            /*右前*/
            //pwm.setPWM(R_f_1, 0, angle_to_pluse(pi/2));
            pwm.setPWM(R_f_2, 0, angle_to_pluse(R_x_y_to_angle_1(Px[i], Py[i])));//着地
            pwm.setPWM(R_f_3, 0, angle_to_pluse(R_x_y_to_angle_2(Px[i], Py[i]) - (pi / 2 - R_x_y_to_angle_1(Px[i], Py[i]))));//着地
            /*右后*/
            //pwm.setPWM(R_b_1, 0, angle_to_pluse(pi/2));
            pwm.setPWM(R_b_2, 0, angle_to_pluse(R_x_y_to_angle_1(Px[i + STEP / 2], Py[i + STEP / 2])));//腾空
            pwm.setPWM(R_b_3, 0, angle_to_pluse(R_x_y_to_angle_2(Px[i + STEP / 2], Py[i + STEP / 2]) - (pi / 2 - R_x_y_to_angle_1(Px[i + STEP / 2], Py[i + STEP / 2]))));//着地
            /*左前*/
            //pwm.setPWM(L_f_1, 0, angle_to_pluse(pi/2));
            pwm.setPWM(L_f_2, 0, angle_to_pluse(L_x_y_to_angle_1(Px[i + STEP / 2], Py[i + STEP / 2])));//腾空
            pwm.setPWM(L_f_3, 0, angle_to_pluse(L_x_y_to_angle_2(Px[i + STEP / 2], Py[i + STEP / 2]) - (pi / 2 - L_x_y_to_angle_1(Px[i + STEP / 2], Py[i + STEP / 2]))));//腾空
            /*左后*/
            //pwm.setPWM(L_b_1, 0, angle_to_pluse(pi/2));
            pwm.setPWM(L_b_2, 0, angle_to_pluse(L_x_y_to_angle_1(Px[i], Py[i])));//着地
            pwm.setPWM(L_b_3, 0, angle_to_pluse(L_x_y_to_angle_2(Px[i], Py[i]) - (pi / 2 - L_x_y_to_angle_1(Px[i], Py[i]))));//着地

            delay(1);
        }

        delay(3);
        for (int i = STEP / 2; i < STEP; i++)
        {
            process_serial();
            if (stopRequested || manualOverride) return;
            /*右前*/
            //pwm.setPWM(R_f_1, 0, angle_to_pluse(pi/2));
            pwm.setPWM(R_f_2, 0, angle_to_pluse(R_x_y_to_angle_1(Px[i], Py[i])));//腾空
            pwm.setPWM(R_f_3, 0, angle_to_pluse(R_x_y_to_angle_2(Px[i], Py[i]) - (pi / 2 - R_x_y_to_angle_1(Px[i], Py[i]))));//腾空
            /*右后*/
            //pwm.setPWM(R_b_1, 0, angle_to_pluse(pi/2));
            pwm.setPWM(R_b_2, 0, angle_to_pluse(R_x_y_to_angle_1(Px[i - STEP / 2], Py[i - STEP / 2])));//着地
            pwm.setPWM(R_b_3, 0, angle_to_pluse(R_x_y_to_angle_2(Px[i - STEP / 2], Py[i - STEP / 2]) - (pi / 2 - R_x_y_to_angle_1(Px[i - STEP / 2], Py[i - STEP / 2]))));//着地
            // /*左前*/
            //pwm.setPWM(L_f_1, 0, angle_to_pluse(pi/2));
            pwm.setPWM(L_f_2, 0, angle_to_pluse(L_x_y_to_angle_1(Px[i - STEP / 2], Py[i - STEP / 2])));//着地
            pwm.setPWM(L_f_3, 0, angle_to_pluse(L_x_y_to_angle_2(Px[i - STEP / 2], Py[i - STEP / 2]) - (pi / 2 - L_x_y_to_angle_1(Px[i - STEP / 2], Py[i - STEP / 2]))));//着地
            /*左后*/
            //pwm.setPWM(L_b_1, 0, angle_to_pluse(pi/2));
            pwm.setPWM(L_b_2, 0, angle_to_pluse(L_x_y_to_angle_1(Px[i], Py[i])));//腾空
            pwm.setPWM(L_b_3, 0, angle_to_pluse(L_x_y_to_angle_2(Px[i], Py[i]) - (pi / 2 - L_x_y_to_angle_1(Px[i], Py[i]))));//腾空

            delay(1);
        }
    }
    delay(3);
}


void my_90()//归零
{
    delay(3);
    /*右前*/
    pwm.setPWM(R_f_1, 0, angle_to_pluse(pi / 2));
    pwm.setPWM(R_f_2, 0, angle_to_pluse(pi / 2));//着地
    pwm.setPWM(R_f_3, 0, angle_to_pluse(pi / 2));//着地
    /*右后*/
    pwm.setPWM(R_b_1, 0, angle_to_pluse(pi / 2));
    pwm.setPWM(R_b_2, 0, angle_to_pluse(pi / 2));//腾空
    pwm.setPWM(R_b_3, 0, angle_to_pluse(pi / 2));//着地
    /*左前*/
    pwm.setPWM(L_f_1, 0, angle_to_pluse(pi / 2));
    pwm.setPWM(L_f_2, 0, angle_to_pluse(pi / 2));//腾空
    pwm.setPWM(L_f_3, 0, angle_to_pluse(pi / 2));//腾空
    /*左后*/
    pwm.setPWM(L_b_1, 0, angle_to_pluse(pi / 2));
    pwm.setPWM(L_b_2, 0, angle_to_pluse(pi / 2));//着地
    pwm.setPWM(L_b_3, 0, angle_to_pluse(pi / 2));//着地
    delay(3);
}


void loop() {

    process_serial();
   

    // my_90();//电机归零90度


}
