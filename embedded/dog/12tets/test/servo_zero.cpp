#include "dog_control.h"

// 实机最终零位，单位 us；数组顺序为髋关节、大腿、小腿。
// 修改网页零位并确认机械姿态后，可用新的数值更新这个文件。
int centerTrimUs[LEG_COUNT][JOINT_COUNT] = {
  {-140, 70, -10},   // 右前 RF：CH4 / CH5 / CH6
  {-225, 265, 350},  // 左前 LF：CH0 / CH1 / CH2
  {-85, -100, -170}, // 右后 RB：CH12 / CH13 / CH14
  {-1000, -60, 120}  // 左后 LB：CH8 / CH9 / CH10
};
