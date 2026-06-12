#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =================================================================
// CREDENCIALES BLYNK IoT & WI-FI
// =================================================================
#define BLYNK_TEMPLATE_ID   "TU_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "TU_TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN    "TU_AUTH_TOKEN"
#define WIFI_SSID           "TU_SSID"
#define WIFI_PASS           "TU_PASS"

// =================================================================
// ASIGNACIÓN DE PINES ESP32 (Módulo de 30 pines)
// =================================================================
// Bus I2C Compartido (OLED, DS3231, AT24C32, AS7341)
#define PIN_I2C_SDA         21  
#define PIN_I2C_SCL         22  

// Encoder Rotativo (Módulo AgroLumen_Input)
#define PIN_ENCODER_CLK     18
#define PIN_ENCODER_DT      19
#define PIN_ENCODER_SW      23

// Periféricos de Control y Alarma
#define PIN_PWM_LED         13
#define PIN_BUZZER          25

// =================================================================
// TEMPORIZACIONES Y CRONÓMETROS (en milisegundos)
// =================================================================
const uint32_t CONTROL_INTERVAL_MS = 2000;    // Tick de la FSM local
const uint32_t WEB_UPLOAD_INTERVAL_MS = 900000; // Envío a Blynk (15 minutos)

#endif // CONFIG_H