#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <driver/adc.h>
#include <driver/ledc.h>
#include <freertos/timers.h>
#include <esp_log.h>
//PARA DHT22
#include <esp_system.h>
#include <rom/ets_sys.h>
#include <nvs_flash.h>
#include <sdkconfig.h>
#include <DHT22.h>
//variables rainmaker
/*
char deviceName_1[] = "Switch1";
char deviceName_2[] = "Switch2";
char deviceName_3[] = "Switch3";
char deviceName_4[] = "Switch4";
 * //The framework provides some standard device types like switch, lightbulb, fan, temperature sensor.
static Switch my_switch1(deviceName_1, &RelayPin1);
static Switch my_switch2(deviceName_2, &RelayPin2);
static Switch my_switch3(deviceName_3, &RelayPin3);
static Switch my_switch4(deviceName_4, &RelayPin4);
static Switch my_switch5(deviceName_5, &RelayPin5);
static Switch my_switch6(deviceName_6, &RelayPin6);
static Switch my_switch7(deviceName_7, &RelayPin7);
static Switch my_switch8(deviceName_8, &RelayPin8);
static TemperatureSensor temperature("Temperature");
static TemperatureSensor humidity("Humidity");
static TemperatureSensor ldr("LDR");
 * */
//variables
static const char *tag = "main";
uint8_t led_level = 0;
TimerHandle_t xTimers;
int interval = 500; // aqui son milisegundos pero solo acepta ticks "pulsos" del esp32 para eso usar pdMS_to_tick
int timerId = 1;
int potenciometro_R;
int potenciometro_G;
int potenciometro_B;
int dutyR;
int dutyG;
int dutyB;
//pines de salida LED
#define ledR 33
#define ledG 25
#define ledB 26
//pines de salida RELAY
#define RelayPin1 23
#define RelayPin2 22
#define RelayPin3 21
#define RelayPin4 19
//pines de entrada switch
#define  SwitchPin1 13
#define  SwitchPin2 12
#define  SwitchPin3 14
#define  SwitchPin4 27
//pines de entrada ADC 34,35,32,  hacia abajo
adc1_channel_t potenciometroB=ADC1_CHANNEL_6;
adc1_channel_t potenciometroG=ADC1_CHANNEL_7;
adc1_channel_t potenciometroR=ADC1_CHANNEL_4;

esp_err_t init_hardware(void);
esp_err_t set_timer(void);
esp_err_t set_pwm(void);
esp_err_t set_pwm_duty(void);
esp_err_t init_led(void);
//funciones repetidas en el tiempo
void vTimerCallback(TimerHandle_t pxTimer)
{
	init_hardware();

	potenciometro_B=adc1_get_raw(potenciometroB);
	potenciometro_G=adc1_get_raw(potenciometroG);
	potenciometro_R=adc1_get_raw(potenciometroR);
	dutyR = potenciometro_R;
	dutyG = potenciometro_G;
	dutyB = potenciometro_B;
	printf("ADC: %d| %d| %d \n",potenciometro_R,potenciometro_G,potenciometro_B);
	//printf("sw1: %d| sw2: %d| sw3: %d| sw4: %d \n",SwitchPin1,SwitchPin2,SwitchPin3,SwitchPin4);
    set_pwm_duty();
}

void DHT_task(void *pvParameter)
{
	//TOMADO DE : https://github.com/gosouth/DHT22/blob/master/main/DHT_main.c
	setDHTgpio( 16 );
	printf( "Starting DHT Task\n\n");

	while(1) {
		int ret = readDHT();
		errorHandler(ret);
		//printf("Hum: %.1f % \n| Tmp: %.1f C°\n",getHumidity() ,getTemperature());
		printf("Hum= %.1f  \n",getHumidity());
		printf("Tmp= %.1f \n",getTemperature());
		// -- wait at least 2 sec before reading again ------------
		// The interval of whole process must be beyond 2 seconds !!
		vTaskDelay( 2000 / portTICK_RATE_MS );
	}
}

void app_main(void)
{
    init_led();
    set_pwm();
    set_timer();
    nvs_flash_init();
    xTaskCreate( &DHT_task, "DHT_task", 2048, NULL, 5, NULL );
}

esp_err_t init_hardware(void){
	adc1_config_width(ADC_WIDTH_BIT_10);
	adc1_config_channel_atten(potenciometroB,ADC_ATTEN_DB_11);
	adc1_config_channel_atten(potenciometroG,ADC_ATTEN_DB_11);
	adc1_config_channel_atten(potenciometroR,ADC_ATTEN_DB_11);
    return ESP_OK;
}

esp_err_t set_timer(void)
{
    ESP_LOGI(tag, "configuración del timer");
    xTimers = xTimerCreate("Timer",                   // Just a text name, not used by the kernel.
                           (pdMS_TO_TICKS(interval)), // The timer period in ticks.
                           pdTRUE,                    // The timers will auto-reload themselves when they expire.
                           (void *)timerId,           // Assign each timer a unique id equal to its array index.
                           vTimerCallback             // Each timer calls the same callback when it expires.
    );

    if (xTimers == NULL)
    {
        // The timer was not created.
        ESP_LOGE(tag, "The timer was not created.");
    }
    else
    {
        if (xTimerStart(xTimers, 0) != pdPASS)
        {
            // The timer could not be set into the Active state.
            ESP_LOGE(tag, "The timer could not be set into the Active state.");
        }
    }

    return ESP_OK;
}

esp_err_t set_pwm(void)
{
    ledc_channel_config_t channelConfigR = {0};
    channelConfigR.gpio_num = 33;
    channelConfigR.speed_mode = LEDC_HIGH_SPEED_MODE;
    channelConfigR.channel = LEDC_CHANNEL_0;
    channelConfigR.intr_type = LEDC_INTR_DISABLE;
    channelConfigR.timer_sel = LEDC_TIMER_0;
    channelConfigR.duty = 0;

    ledc_channel_config_t channelConfigG = {0};
    channelConfigG.gpio_num = 25;
    channelConfigG.speed_mode = LEDC_HIGH_SPEED_MODE;
    channelConfigG.channel = LEDC_CHANNEL_1;
    channelConfigG.intr_type = LEDC_INTR_DISABLE;
    channelConfigG.timer_sel = LEDC_TIMER_0;
    channelConfigG.duty = 0;

    ledc_channel_config_t channelConfigB = {0};
    channelConfigB.gpio_num = 26;
    channelConfigB.speed_mode = LEDC_HIGH_SPEED_MODE;
    channelConfigB.channel = LEDC_CHANNEL_2;
    channelConfigB.intr_type = LEDC_INTR_DISABLE;
    channelConfigB.timer_sel = LEDC_TIMER_0;
    channelConfigB.duty = 0;

    ledc_channel_config(&channelConfigR);
    ledc_channel_config(&channelConfigG);
    ledc_channel_config(&channelConfigB);

    ledc_timer_config_t timerConfig = {0};
    timerConfig.speed_mode = LEDC_INTR_DISABLE;
    timerConfig.duty_resolution = LEDC_TIMER_10_BIT;
    // timerConfig.bit_num = LEDC_TIMER_10_BIT;
    timerConfig.timer_num = LEDC_TIMER_0;
    timerConfig.freq_hz = 20000;

    ledc_timer_config(&timerConfig);

    return ESP_OK;
}

esp_err_t set_pwm_duty(void)
{
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, dutyR);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, dutyG);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, dutyB);

    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2);
    return ESP_OK;
}

esp_err_t init_led(void)
{
    gpio_reset_pin(ledR);
    gpio_set_direction(ledR, GPIO_MODE_OUTPUT);
    gpio_reset_pin(ledG);
    gpio_set_direction(ledG, GPIO_MODE_OUTPUT);
    gpio_reset_pin(ledB);
    gpio_set_direction(ledB, GPIO_MODE_OUTPUT);
    //switches
    gpio_reset_pin(SwitchPin1);
    gpio_set_direction(SwitchPin1, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SwitchPin1, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(SwitchPin1, GPIO_INTR_ANYEDGE);
   //*** gpio_install_isr_service(13);
   //***gpio_isr_handler_add(SwitchPin1, main_isr_handler, NULL)
    gpio_reset_pin(SwitchPin2);
    gpio_set_direction(SwitchPin2, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SwitchPin2, GPIO_PULLUP_ONLY);
    gpio_reset_pin(SwitchPin3);
    gpio_set_direction(SwitchPin3, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SwitchPin3, GPIO_PULLUP_ONLY);
    gpio_reset_pin(SwitchPin4);
    gpio_set_direction(SwitchPin4, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SwitchPin4, GPIO_PULLUP_ONLY);

     return ESP_OK;
}

