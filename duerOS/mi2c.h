/*
 * i2c.h
 *
 *  Created on: 2018Äê6ÔÂ9ÈÕ
 *      Author: work
 */

#ifndef TESTS_TARGET_RDA_BAIDU_DUEROS_DIRVER_MI2C_H_
#define TESTS_TARGET_RDA_BAIDU_DUEROS_DIRVER_MI2C_H_
typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;
typedef signed char         s8;
typedef signed short        s16;
typedef signed int          s32;
typedef signed long long    s64;

void mi2c_init(void);
void i2c_start(void);
void i2c_writeByte(unsigned char wrdata);
u8 i2c_checkAck(void);
void i2c_stop(void);

#endif /* TESTS_TARGET_RDA_BAIDU_DUEROS_DIRVER_MI2C_H_ */
