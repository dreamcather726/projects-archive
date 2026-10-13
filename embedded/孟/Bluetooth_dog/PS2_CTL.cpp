#include "PS2_CTL.hpp"
#include <string.h>
#define USB_TX      34
#define Serial_PS2 Serial2

PS2_CTL::PS2_CTL(void)
{
  rec_flag = false;
  has_keyvalue = false;
  memset(&keyvalue, 0, sizeof(keyvalue));
  memset(recbuff, 0, sizeof(recbuff));
}

void PS2_CTL::init(void)
{
  // Serial 用于电脑串口监视器调试，Serial2 用于接收 PS2 手柄模块数据。
  Serial.begin(115200);
  Serial_PS2.begin(9600 ,SERIAL_8N1 , USB_TX , -1);
}

void PS2_CTL::PS2_Task(void)
{
  receive_msg();
}

bool PS2_CTL::getKeyValue(PS2KeyValueObjectTypeDef *latest_keyvalue)
{
  // 外部控制器通过这个函数拿到“最近一次完整解析成功”的手柄状态。
  if (!has_keyvalue || latest_keyvalue == NULL)
  {
    return false;
    }

    memcpy(latest_keyvalue, &keyvalue, sizeof(keyvalue));
    has_keyvalue = false;
    return true;
}

void PS2_CTL::clear_rec(void)
{
    // 清空串口接收缓冲区，通常用于重新同步 PS2 数据流。
    while (Serial_PS2.available()>0)
    {
        Serial_PS2.read();
    }
}

void PS2_CTL::receive_msg(void)
{
  static uint8_t step = 0;
  static uint8_t index = 0;
  // 手柄推动摇杆后，模式特征字节会变化，所以这里保存最近一次识别出的模式。
  static uint8_t current_mode = PS2_COORDINATE_MODE;
  while (Serial_PS2.available()>0) {
    switch(step){
      case 0:{
        // 第 1 个帧头：PS2 数据帧以 0x55 0x55 开始。
        if(Serial_PS2.read() == FRAME_HEADER)
        {
          step++;
        }
      }break;
      case 1:{
        // 第 2 个帧头：连续两个 0x55 才认为后面是有效数据区。
        if(Serial_PS2.read() == FRAME_HEADER)
        {
          index = 0;
          step++;
        }else{
          step = 0;
        }
      }break;
      case 2:{
        // 帧头后面固定读取 10 个字节，保存到 recbuff[0] ~ recbuff[9]。
        recbuff[index++] = Serial_PS2.read();
        if(index > 9)
        {
          rec_flag = true;
          step = 0;
        }
      }break;
      default:{
        step = 0;
      }break;
    }

    if(rec_flag)
    {
      // 每收到一帧新数据，先清空上一次按键状态，再重新解析当前帧。
      memset(&keyvalue, 0, sizeof(keyvalue));

      // 这两个特征用于判断当前 PS2 模块处于单舵机模式还是坐标模式。
      if(recbuff[9] == 0x80 && recbuff[6] == 0x80 &&
          recbuff[7] == 0x80 && recbuff[8] == 0x80)
      {
        current_mode = PS2_SINGLE_SERVO_MODE;
      }
      else if(recbuff[9] == 0x7F && recbuff[6] == 0x7F &&
              recbuff[7] == 0x7F && recbuff[8] == 0x7F)
      {
        current_mode = PS2_COORDINATE_MODE;
      }
      keyvalue.mode = current_mode;

      // recbuff[3] 保存右侧功能键和肩键，每一位代表一个按键状态。
      keyvalue.bit_triangle = 	recbuff[3] & 0xFF;
      keyvalue.bit_circle = 	(recbuff[3] & 0xFF) >> 1;
      keyvalue.bit_cross = 		(recbuff[3] & 0xFF) >> 2;
      keyvalue.bit_square = 	(recbuff[3] & 0xFF) >> 3;
      keyvalue.bit_l1 =  		(recbuff[3] & 0xFF) >> 4;
      keyvalue.bit_r1 =  		(recbuff[3] & 0xFF) >> 5;
      keyvalue.bit_l2 =  		(recbuff[3] & 0xFF) >> 6;
      keyvalue.bit_r2 =  		(recbuff[3] & 0xFF) >> 7;
      // recbuff[4] 保存 Select、Start 和左右摇杆按下状态。
      keyvalue.bit_select = 	recbuff[4] & 0xFF;
      keyvalue.bit_start = 		(recbuff[4] & 0xFF) >> 1;
      keyvalue.bit_leftjoystick_press =  (recbuff[4] & 0xFF) >> 2;
      keyvalue.bit_rightjoystick_press = (recbuff[4] & 0xFF) >> 3;

      switch(keyvalue.mode){
        case PS2_SINGLE_SERVO_MODE:
          // 单舵机模式下，用 recbuff[6] 和 recbuff[7] 判断方向键方向。
          keyvalue.bit_left = recbuff[6] == 0x00 ? 1 : 0;
          keyvalue.bit_right = recbuff[6] == 0xFF ? 1 : 0;
          keyvalue.bit_up = recbuff[7] == 0x00 ? 1 : 0;
          keyvalue.bit_down = recbuff[7] == 0xFF ? 1 : 0;
          break;
        
        case PS2_COORDINATE_MODE:
          // 坐标模式下，recbuff[6] ~ recbuff[9] 是左右摇杆的 X/Y 原始值。
          keyvalue.left_joystick_x = recbuff[6];
          keyvalue.left_joystick_y = recbuff[7];
          keyvalue.right_joystick_x = recbuff[8];
          keyvalue.right_joystick_y = recbuff[9];
          // recbuff[5] 用 0x00 ~ 0x07 表示方向键八方向，0x0F 表示未按方向键。
          switch(recbuff[5])
          {
            case 0x00:
              keyvalue.bit_up = 1;
              break;
            
            case 0x01:
              keyvalue.bit_up = 1;
              keyvalue.bit_right = 1;
              break;
            
            case 0x02:
              keyvalue.bit_right = 1;
              break;
            
            case 0x03:
              keyvalue.bit_down = 1;
              keyvalue.bit_right = 1;
              break;
            
            case 0x04:
              keyvalue.bit_down = 1;
              break;
            
            case 0x05:
              keyvalue.bit_down = 1;
              keyvalue.bit_left = 1;
              break;
            
            case 0x06:
              keyvalue.bit_left = 1;
              break;
            
            case 0x07:
              keyvalue.bit_up = 1;
              keyvalue.bit_left = 1;
              break;
            
            case 0x0F:
              keyvalue.bit_up = 0;
              keyvalue.bit_down = 0;
              keyvalue.bit_left = 0;
              keyvalue.bit_right = 0;
              break;
          }
          break;
      }
      // 标记已经有一份可供机械狗控制器读取的手柄状态。
      has_keyvalue = true;
      rec_flag = false;
      break;
    }
  }
}
