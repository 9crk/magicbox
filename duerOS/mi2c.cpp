/*
 * i2c.c
 *
 *  Created on: 2018Äê6ÔÂ9ÈÕ
 *      Author: work
 */

#include "gpio_api.h"
#include "gpio_object.h"
#include "mbed.h"
#include "mi2c.h"
#include "wait_api.h"





gpio_t SDA,SCL;
void mi2c_init(void)
{

	gpio_init(&SDA, PC_9);
	gpio_dir(&SDA, PIN_OUTPUT);
	gpio_mode(&SDA, Repeater);
	gpio_write(&SDA, 1);
	gpio_init(&SCL, PA_0);
	gpio_dir(&SCL, PIN_OUTPUT);
	gpio_mode(&SCL, Repeater);
	gpio_write(&SCL, 1);
}

void i2c_start(void)
{
	gpio_write(&SCL, 1);
	gpio_write(&SDA, 1);
	wait_us(5);
	gpio_write(&SDA, 0);
	wait_us(5);
}

void i2c_writeByte(u8 wrdata)
{
    u8 dat;
    s8 i;
    wait_us(5);
    gpio_write(&SCL, 0);
    wait_us(5);

    for (i = 7; i >= 0; i--) {
        dat = (wrdata >> i)&1;
        gpio_write(&SDA, dat);
        wait_us(5);
        gpio_write(&SCL, 1);
        wait_us(5);

        if (i == 0) {
        	wait_us(5);
        }

        gpio_write(&SCL, 0);
        wait_us(5);
    }
}

u8 i2c_checkAck(void)
{
	gpio_dir(&SDA, PIN_INPUT);
	wait_us(5);
    //delay_us(1);
    gpio_write(&SCL, 0);
    wait_us(5);
    gpio_write(&SCL, 1);
    wait_us(5);
    gpio_write(&SCL, 0);
    wait_us(5);
    gpio_dir(&SDA, PIN_OUTPUT);
    wait_us(5);
    return 0;
}

void i2c_stop(void)
{

	wait_us(5);
    gpio_write(&SDA, 0);
    wait_us(5);
    gpio_write(&SCL, 1);
    wait_us(5);
    gpio_write(&SDA, 1);
    wait_us(5);
}
