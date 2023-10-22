#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"


#include "led_rgb.h"
#include "couleurs.h"

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
// Define RGB LEDs IOs
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#ifndef CONFIG_GPIO_LED_R
#warning	"CONFIG_GPIO_LED_R non defini => 33 par defaut"
#define LEDC_RED_IO         33	
#else
#define LEDC_RED_IO					(CONFIG_GPIO_LED_R)
#endif

#ifndef CONFIG_GPIO_LED_G
#warning	"CONFIG_GPIO_LED_G non defini => 25 par defaut"
#define LEDC_GREEN_IO				25
#else
#define LEDC_GREEN_IO				(CONFIG_GPIO_LED_G)
#endif

#ifndef CONFIG_GPIO_LED_B
#warning	"CONFIG_GPIO_LED_B non defini => 26 par defaut"
#define LEDC_BLUE_IO					26     	
#else
#define LEDC_BLUE_IO					(CONFIG_GPIO_LED_B)
#endif

static void rgb_esp_duty(rgb_t *rgb);
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  constantes */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
// static const char *TAG = "Led_rgb";

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  variables locales */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
static rgb_channel_config_t rgb_channels = {
    .red_channel = LEDC_CHANNEL_RED,
    .green_channel = LEDC_CHANNEL_GREEN,
    .blue_channel = LEDC_CHANNEL_BLUE,
    .fade = 0
};

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
// Fonctions pour ESP32
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*==========================================================*/
/* init PWM progressif avec correction gamma */ /* 	
	nécessite ledc_fade_func_install(0) */
/*==========================================================*/
static void rgb_set_linear_fade(rgb_t *rgb, uint32_t duration)
{
	ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_MODE, rgb_channels.red_channel, rgb->r, duration));
	ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_MODE, rgb_channels.green_channel, rgb->g, duration));
	ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_MODE, rgb_channels.blue_channel, rgb->b, duration));
}

/*==========================================================*/
/* démarre PWM progressif */ /* 	
	nécessite ledc_fade_func_install(0) */
/*==========================================================*/
static void rgb_fade_start(void)
{
	ESP_ERROR_CHECK(ledc_fade_start(LEDC_MODE, rgb_channels.red_channel, LEDC_FADE_NO_WAIT));
	ESP_ERROR_CHECK(ledc_fade_start(LEDC_MODE, rgb_channels.green_channel, LEDC_FADE_NO_WAIT));
	ESP_ERROR_CHECK(ledc_fade_start(LEDC_MODE, rgb_channels.blue_channel, LEDC_FADE_NO_WAIT));
}

/*==========================================================*/
/* mise à jour PWM progressive */ /* 	
	nécessite ledc_fade_func_install(0) */
/*==========================================================*/
static void rgb_esp_fade_and_start(rgb_t *rgb, uint32_t duration)
{
	if(rgb_channels.fade) {
		rgb_set_linear_fade(rgb, duration);
		rgb_fade_start();
		vTaskDelay(pdMS_TO_TICKS(duration+5));
	}
	else {
		rgb_esp_duty(rgb);
	}
}

/*==========================================================*/
/* mise à jour progressive du format rgb vers PWM  */ /*
	avec correction gamma 
	nécessite ledc_fade_func_install(0) */
/*==========================================================*/
void rgb_esp_rgb_fade(rgb_t *rgb, uint32_t duration)
{		
	correction_gamma_RGB(rgb, LEDC_DUTY_RES);
	rgb_esp_fade_and_start(rgb, duration);
}

/*==========================================================*/
/* mise à jour progressive du format hsv vers PWM  */ /*
	avec correction gamma 
	nécessite ledc_fade_func_install(0) */
/*==========================================================*/
void rgb_esp_hsv_fade(hsv_t *hsv, uint32_t duration)
{		
	rgb_t rgb;
	hsv_en_rgb_corrige(hsv, LEDC_DUTY_RES, &rgb);
	rgb_esp_fade_and_start(&rgb, duration);
}

//*==========================================================*/
/* éteint progressivement */ /* 
	nécessite ledc_fade_func_install(0)  */
/*==========================================================*/
void rgb_esp_fade_off(uint32_t duration)
{	
	rgb_t rgb = {OFF_RGB};
	rgb_esp_fade_and_start(&rgb, duration);
	vTaskDelay(pdMS_TO_TICKS(100));
}


/*==========================================================*/
/* mise à jour immédiate PWM avec RGB affecté de la résolution */ /* 
		 */
/*==========================================================*/
static void rgb_esp_duty(rgb_t *rgb) {
	ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, rgb_channels.red_channel, rgb->r));
	ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, rgb_channels.green_channel, rgb->g));
	ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, rgb_channels.blue_channel, rgb->b));
	ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, rgb_channels.red_channel));
	ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, rgb_channels.green_channel));
	ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, rgb_channels.blue_channel));
}

/*==========================================================*/
/* mise à jour immédiate du format rgb vers PWM */ /* 
	avec correction gamma 	 */
/*==========================================================*/
void rgb_esp_rgb(rgb_t *rgb)
{		
	correction_gamma_RGB(rgb, LEDC_DUTY_RES);
	rgb_esp_duty(rgb);
}

/*==========================================================*/
/* mise à jour immédiate du format hsv vers PWM */ /* 
		avec correction gamma */
/*==========================================================*/
void rgb_esp_hsv(hsv_t *hsv)
{		
	rgb_t rgb;
	hsv_en_rgb_corrige(hsv, LEDC_DUTY_RES, &rgb);
	rgb_esp_duty(&rgb);
}

/*==========================================================*/
/* éteint immédiatement */ /*  */
/*==========================================================*/
void rgb_esp_off(void)
{
	rgb_t rgb = {OFF_RGB};
	rgb_esp_duty(&rgb);
}

/*==========================================================*/
/* init timer, canaux et pin pour PWM */ /*  */
/*==========================================================*/
void rgb_ledc_init(int fade)
{
	// Prepare and then apply the LEDC PWM timer configuration
	ledc_timer_config_t ledc_timer = {
			.speed_mode       = LEDC_MODE,
			.timer_num        = LEDC_TIMER,
			.duty_resolution  = LEDC_DUTY_RES,
			.freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
			.clk_cfg          = LEDC_AUTO_CLK
	};
	ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

	// Prepare and then apply the LEDC PWM configuration to the six channels
	ledc_channel_config_t ledc_channel = {
			.speed_mode     = LEDC_MODE,
			.timer_sel      = LEDC_TIMER,
			.intr_type      = LEDC_INTR_DISABLE,
			.duty           = 0, // Set initial duty to 0%
			.hpoint         = 0
	};
	ledc_channel.channel = LEDC_CHANNEL_RED;
	ledc_channel.gpio_num = LEDC_RED_IO;
	ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
	ledc_channel.channel = LEDC_CHANNEL_GREEN;
	ledc_channel.gpio_num = LEDC_GREEN_IO;
	ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
	ledc_channel.channel = LEDC_CHANNEL_BLUE;
	ledc_channel.gpio_num = LEDC_BLUE_IO;
	ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

	if(fade) {
		ledc_fade_func_install(0);
		rgb_channels.fade = fade;
	}
	return;
}

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
// Fonctions pour driver P9813
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*==========================================================*/
/* date du format hsv vers data P9813 */ /* 
		avec correction gamma */
/*==========================================================*/
void rgb_format_P9813_hsv(hsv_t *hsv, unsigned char *bufP9813) {
	rgb_t rgb;
	hsv_en_rgb_corrige(hsv, 8, &rgb);
	// Start by sending a byte with the format "1 1 /B7 /B6 /G7 /G6 /R7 /R6"
	bufP9813[0] = 0b11000000;
	if ((rgb.b & 0x80) == 0) {
			bufP9813[0] |= 0b00100000;
	}
	if ((rgb.b & 0x40) == 0) {
			bufP9813[0] |= 0b00010000;
	}
	if ((rgb.g & 0x80) == 0) {
			bufP9813[0] |= 0b00001000;
	}
	if ((rgb.g & 0x40) == 0) {
			bufP9813[0] |= 0b00000100;
	}
	if ((rgb.r & 0x80) == 0) {
			bufP9813[0] |= 0b00000010;
	}
	if ((rgb.r & 0x40) == 0) {
			bufP9813[0] |= 0b00000001;
	}
	
	bufP9813[1] = (unsigned char)rgb.b;
	bufP9813[2] = (unsigned char)rgb.g;
	bufP9813[3] = (unsigned char)rgb.r;
}

#if(0)
/*==========================================================*/
/* test */ /*  */
/*==========================================================*/
void led_test(void)
{
    // Light up RGB LEDs to GREEN in 500ms
// 		ESP_LOGI(TAG, "Light up RGB LEDs to GREEN in 500ms");
//     rgb_set_fade_and_start_correct(GREEN , 500);
//     vTaskDelay(pdMS_TO_TICKS(1000));

    // Dim to OFF in 2000ms
// 		ESP_LOGI(TAG, "Dim to OFF in 2000ms");
//     rgb_set_fade_and_start_correct(OFF_RGB, 2000);
//     vTaskDelay(pdMS_TO_TICKS(1000));

    // Set RGB LEDs to BLUEISH_PURPLE color
		ESP_LOGI(TAG, "Set RGB LEDs to BLUEISH_PURPLE color");
    rgb_set_immediat(BLUEISH_PURPLE);
    vTaskDelay(pdMS_TO_TICKS(2000));
// 

// 	rgb_set_immediat(26, 17, 0);
// 	vTaskDelay(pdMS_TO_TICKS(2000));
// 	
// 	rgb_set_fade_immediat_correct(26, 17, 0);
// 
    // Dim to OFF in 5000ms
// 		ESP_LOGI(TAG, "Dim to OFF in 5000ms");
//     rgb_set_fade_and_start_correct(OFF_RGB, 5000);
//     vTaskDelay(pdMS_TO_TICKS(1000));
//     
// 
//     // Light up to ORANGE in 5000ms
// 		ESP_LOGI(TAG, "Light up to ORANGE in 5000ms");
//     rgb_set_fade_and_start_correct(ORANGE, 5000);
//     vTaskDelay(pdMS_TO_TICKS(1000));
// 
//     // Fade color to ROSE PINK in 5000ms
// 		ESP_LOGI(TAG, "Fade color to ROSE PINK in 5000ms");
//     rgb_set_fade_and_start_correct(ROSE, 5000);
//     vTaskDelay(pdMS_TO_TICKS(1000));

    // Cycle through color spectrum
//     while (1) {
		ESP_LOGI(TAG, "YELLOW 3000ms");
        rgb_set_fade_and_start_correct(YELLOW, 3000);
//         vTaskDelay(pdMS_TO_TICKS(5));
// 
		ESP_LOGI(TAG, "GREEN 3000ms");
        rgb_set_fade_and_start_correct(GREEN, 3000);
//         vTaskDelay(pdMS_TO_TICKS(5));
// 
		ESP_LOGI(TAG, "CYAN 3000ms");
        rgb_set_fade_and_start_correct(CYAN, 3000);
//         vTaskDelay(pdMS_TO_TICKS(5));
// 
		ESP_LOGI(TAG, "BLUE 3000ms");
        rgb_set_fade_and_start_correct(BLUE, 3000);
//         vTaskDelay(pdMS_TO_TICKS(5));
// 
		ESP_LOGI(TAG, "MAGENTA 3000ms");
       rgb_set_fade_and_start_correct(MAGENTA, 3000);
//         vTaskDelay(pdMS_TO_TI;CKS(5));

		ESP_LOGI(TAG, "RED 3000ms");
        rgb_set_fade_and_start_correct(RED, 3000);
//         vTaskDelay(pdMS_TO_TICKS(5));
        
		ESP_LOGW(TAG, "fin du test");

//     }
}
#endif

