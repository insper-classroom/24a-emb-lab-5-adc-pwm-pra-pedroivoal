/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"

#include <math.h>
#include <stdlib.h>

#define FILTER_SIZE 5
#define portMAX_DELAY 50

const int X_PIN = 27;
const int Y_PIN = 26;

QueueHandle_t xQueueAdc;

typedef struct adc {
    // x=0, y=1
    int axis;
    int val;
} adc_t;


int converter(int val) {
    val -= 2048;
    val /= 8;
    if (val < 30 && val > -30) 
        val = 0;
    return val;
}


void adc_x_task(void *p) {
    adc_init();
    adc_gpio_init(X_PIN);
    
    adc_t adc_x;
    adc_x.axis = 0;
    int delay = 500;
    while (1)
    {
        adc_select_input(1);
        adc_x.val = converter(adc_read());
        xQueueSend(xQueueAdc, &adc_x, 1 );
        printf("X: %d\n", adc_x.val);
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
}


void adc_y_task(void *p) {
    adc_init();
    adc_gpio_init(Y_PIN);
    
    adc_t adc_y;
    adc_y.axis = 1;
    int delay = 500;
    while (1)
    {
        adc_select_input(0);
        adc_y.val = converter(adc_read());
        xQueueSend(xQueueAdc, &adc_y, 1 );
        printf("Y: %d\n", adc_y.val);
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
}

void write_package(adc_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF ;

    uart_putc_raw(uart0, data.axis);
    uart_putc_raw(uart0, lsb);
    uart_putc_raw(uart0, msb);
    uart_putc_raw(uart0, -1);
}

void uart_task(void *p) {
    adc_t data;

    while (1) {
        if (xQueueReceive(xQueueAdc, &data, portMAX_DELAY)) {
            // write_package(data);
        } else {}
    }
}

int main() {
    stdio_init_all();

    xQueueAdc = xQueueCreate(32, sizeof(adc_t));

    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);
    xTaskCreate(adc_x_task, "adc_x_task", 511, NULL, 1, NULL);
    xTaskCreate(adc_y_task, "adc_y_task", 511, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
