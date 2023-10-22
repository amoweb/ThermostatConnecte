#ifndef LED_RGB_H
#define LED_RGB_H

#include "couleurs.h"

#include "driver/ledc.h"

void rgb_ledc_init(int fade);
void rgb_esp_off(void);
void rgb_esp_hsv(hsv_t *hsv);
void rgb_esp_rgb(rgb_t *rgb);
void rgb_esp_fade_off(uint32_t duration);
void rgb_esp_hsv_fade(hsv_t *hsv, uint32_t duration);
void rgb_esp_rgb_fade(rgb_t *rgb, uint32_t duration);

#define LEDC_TIMER                    LEDC_TIMER_0
// #define LEDC_MODE                     LEDC_LOW_SPEED_MODE
#define LEDC_MODE                     LEDC_HIGH_SPEED_MODE

#define LEDC_DUTY_RES                 LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_FREQUENCY                (5000) // Frequency in Hertz. Set frequency at 5 kHz
// #define LEDC_DUTY_RES                 LEDC_TIMER_12_BIT // Set duty resolution to 13 bits
// #define LEDC_FREQUENCY                (2500) // Frequency in Hertz. Set frequency at 5 kHz

#define LEDC_CHANNEL_RED              (LEDC_CHANNEL_0)
#define LEDC_CHANNEL_GREEN            (LEDC_CHANNEL_1)
#define LEDC_CHANNEL_BLUE             (LEDC_CHANNEL_2)

// Structure to store R, G, B channels
typedef struct {
    uint32_t red_channel;
    uint32_t green_channel;
    uint32_t blue_channel;
    int			 fade;
} rgb_channel_config_t;

// Define some colors R, G, B channel PWM duty cycles
#define RGB_TO_DUTY(x)  (x * (1 << LEDC_DUTY_RES) / 255)

#endif // LED_RGB_H