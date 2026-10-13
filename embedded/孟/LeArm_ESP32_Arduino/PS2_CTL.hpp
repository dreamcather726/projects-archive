#ifndef __PS2_PORTING_H__
#define __PS2_PORTING_H__
#include <Arduino.h>
#include "stdint.h"

#define FRAME_HEADER						0x55
#define PS2_PACKET_LENGTH					0x0A
// #define PS2_FRAME_LENGTH					  12
#define MAX_PS2_RB_BUFFER_LENGTH			  	  64

typedef enum
{
	PS2_SINGLE_SERVO_MODE = 1,
	PS2_COORDINATE_MODE
}PS2ModeStatusTypeDef;

typedef enum
{
	UNPACK_FINISH = 0,
	UNPACK_START
}PackStatusTypeDef;

#pragma pack()
typedef struct
{
	uint8_t mode;

	uint8_t left_joystick_x;
	uint8_t left_joystick_y;
	uint8_t right_joystick_x;
	uint8_t right_joystick_y;
	
	uint8_t bit_up : 						1;
	uint8_t bit_down : 						1;
	uint8_t bit_left : 						1;
	uint8_t bit_right : 					1;
	
	uint8_t bit_start : 				1;
	uint8_t bit_select : 				1;
	
	uint8_t bit_cross : 				1;
	uint8_t bit_circle : 				1;
	uint8_t bit_square : 				1;
	uint8_t bit_triangle : 				1;
	
	uint8_t bit_l1 : 					1;
	uint8_t bit_l2 : 					1;
	uint8_t bit_r1 : 					1;
	uint8_t bit_r2 : 					1;

	uint8_t bit_leftjoystick_press : 	1;
	uint8_t bit_rightjoystick_press : 	1;

}PS2KeyValueObjectTypeDef;


class PS2_CTL
{
    public:
        PS2_CTL(void);
        void init(void);
        void clear_rec(void);
        void PS2_Task(void);
        bool getKeyValue(PS2KeyValueObjectTypeDef *latest_keyvalue);
    private:
        bool rec_flag;
        bool has_keyvalue;
		PS2KeyValueObjectTypeDef keyvalue;
		uint8_t recbuff[20];
        void receive_msg(void);
};


#endif
