
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"

#include "i2c.h"
#include "wm8789.h"


static void i2c_task_example(void *arg)
{
    uint8_t sensor_data[14];
    uint8_t who_am_i, i;
    double Temp;
    static uint32_t error_count = 0;
    int ret;

    i2c_init();
    WM8978_Init();

    while (1) {
        who_am_i = 0;
        i2c_read(WM8978_ADDR, &who_am_i, 1);
        printf("who am i 1 = %02x\n",who_am_i);
        
        who_am_i = WM8978_Read_Reg(WM8978_ADDR);
        printf("who am i 2 = %02x\n",who_am_i);

        xTaskDelay(1000/portTICK_RATE_MS);
    }
    i2c_exit();
}

void app_main(void)
{
    //start i2c task
    xTaskCreate(i2c_task_example, "i2c_task_example", 2048, NULL, 10, NULL);
}