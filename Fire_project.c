#include <stdio.h> // Thư viện để in thông báo và xử lí dữ liệu
#include "driver/ledc.h" // Thư viện điểu khiển PWM cho buzzer
#include "driver/gpio.h" // Thư viện điểu khiển GPIO
#include "driver/rtc_io.h"//Thư viện cho phép làm việc với chân GPIO
#include "esp_sleep.h" // Thư viện cho phép sử dụng chế độ sleep
#include "freertos/FreeRTOS.h" // Thư viện cho phép sử dụng FreeRTOS
#include "freertos/task.h" //Thư viện hỗ trợ tạo task
#include "esp_err.h" // Thư viện cho phép xử lý lỗi
#define BUZZER_PIN 18       // Chân kết nối với buzzer
#define LED_PIN 17          // Chân kết nối với LED
#define FIRE_SENSOR_PIN_AO 35  // Chân AO của cảm biến lửa (GPIO 35)
#define FIRE_SENSOR_PIN_DO 23  // Chân DO của cảm biến lửa (GPIO 23)
#define LEDC_TIMER LEDC_TIMER_0         // Timer 0 được sử dụng cho PWM
#define LEDC_MODE LEDC_LOW_SPEED_MODE   // Chế độ hoạt động PWM ở tốc độ thấp
#define LEDC_CHANNEL LEDC_CHANNEL_0     // Kênh PWM 0
#define LEDC_DUTY 128                   // Chu kỳ PWM (50% - nửa chu kỳ)
#define LEDC_FREQUENCY 500              // Tần số PWM khởi đầu (500Hz)
// Hàm khởi tạo PWM cho buzzer
void init_buzzer() {
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_8_BIT, // Độ phân giải 8-bit
        .freq_hz = LEDC_FREQUENCY,           // Tần số
        .speed_mode = LEDC_MODE,             // Chế độ PWM
        .timer_num = LEDC_TIMER              // Số timer
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .channel    = LEDC_CHANNEL,
        .duty       = 0,                     // Ban đầu không phát tín hiệu
        .gpio_num   = BUZZER_PIN,
        .speed_mode = LEDC_MODE,
        .timer_sel  = LEDC_TIMER
    };
    ledc_channel_config(&ledc_channel);
}

// Hàm khởi tạo LED và cảm biến lửa
void init_gpio() {
    esp_rom_gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);       // LED là output
    esp_rom_gpio_pad_select_gpio(FIRE_SENSOR_PIN_DO);
    gpio_set_direction(FIRE_SENSOR_PIN_DO, GPIO_MODE_INPUT); // Cảm biến lửa (DO) là input
    esp_rom_gpio_pad_select_gpio(FIRE_SENSOR_PIN_AO);
    gpio_set_direction(FIRE_SENSOR_PIN_AO, GPIO_MODE_INPUT); // Cảm biến lửa (AO) là input
}
// Hàm để chuyển ESP32 vào chế độ light sleep
void enter_light_sleep() {
    esp_sleep_enable_ext0_wakeup(FIRE_SENSOR_PIN_DO, 0); // Kích hoạt interrupt khi có tín hiệu từ cảm biến lửa (DO)
    esp_light_sleep_start(); // Bắt đầu chế độ light sleep
}

// Task phát còi báo động
void buzzer_task(void *param) {
    while (1) {
        if (fire_detected) {
            for (int freq = 500; freq <= 2000; freq += 50) {
                ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq);
                ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY);
                ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            for (int freq = 2000; freq >= 500; freq -= 50) {
                ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
        } else {
            ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0); // Tắt buzzer khi không phát hiện lửa
            ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Giảm tải CPU
    }
}

// Task nhấp nháy LED
void led_task(void *param) {
    while (1) {
        if (fire_detected) {
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(200)); // LED sáng trong 200ms
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(200)); // LED tắt trong 200ms
        } else {
            gpio_set_level(LED_PIN, 0); // Đảm bảo LED tắt
            vTaskDelay(pdMS_TO_TICKS(10)); // Giảm tải CPU
        }
    }
}
// Hàm đọc giá trị cảm biến lửa (chân AO)
int read_flame_sensor_ao() {
    int adc_value = adc1_get_raw(ADC1_CHANNEL_7); // Đọc giá trị từ cảm biến lửa (AO)
    printf("ADC Value: %d\n", adc_value); // In ra giá trị ADC
    return adc_value;
}

// Hàm đọc tín hiệu số từ cảm biến lửa (chân DO)
int read_flame_sensor_do() {
    return gpio_get_level(FIRE_SENSOR_PIN_DO); // Đọc giá trị tín hiệu số từ DO của cảm biến lửa
}
