#ifndef AGROLUMEN_OLED_H
#define AGROLUMEN_OLED_H

#include <Arduino.h>
#include "../AgroLumen_Storage/AgroLumen_Storage.h"

// Definición de las opciones del menú alineadas con la FSM original
namespace AgroMenu {
    enum Option : uint8_t {
        MANUAL_DLI   = 0,
        ADJUST_CLOCK = 1,
        VIEW_HISTORY = 2,
        EXIT         = 3,
        PRESET_1     = 4, // Opciones para presets de plantas
        PRESET_2     = 5
    };
}

/**
 * @brief Inicializa el display SSD1306 en la dirección I2C correspondiente.
 */
void agroOLED_begin();

/**
 * @brief Renderiza la pantalla principal de monitoreo en tiempo real.
 * @param accumulatedDLI Valor del DLI integrado en la jornada actual.
 * @param targetDLI Valor de DLI objetivo configurado por el usuario.
 * @param pwmPercentage Porcentaje actual de atenuación de la lámpara LED (0-100).
 */
// void agroOLED_showMonitoringScreen(float accumulatedDLI, float targetDLI, uint8_t pwmPercentage);
// void agroOLED_showMonitoringScreen(float accumulatedDLI, float targetDLI, uint8_t pwmPercentage, float ppfdActual);
// void agroOLED_showMonitoringScreen(float dliActual, float dliObjetivo, float ppfdActual, uint8_t hora, uint8_t minuto);
void agroOLED_showMonitoringScreen(float dliActual, float dliObjetivo, float ppfdActual, uint8_t hora, uint8_t minuto, uint8_t dia, uint8_t mes, uint16_t anio);


/**
 * @brief Dibuja la pantalla del menú de navegación.
 */
void agroOLED_showMenuScreen();

/**
 * @brief Mueve el cursor de selección del menú hacia arriba o hacia abajo.
 * @param direction 1 para bajar, -1 para subir.
 */
void agroOLED_moveMenuCursor(int8_t direction);

/**
 * @brief Obtiene la opción que el usuario tiene seleccionada actualmente con el cursor.
 * @return Opción del enum AgroMenu::Option.
 */
uint8_t agroOLED_getSelectedMenuOption();

/**
 * @brief Muestra la interfaz de configuración manual del DLI objetivo.
 * @param currentTargetDLI Valor actual que se está editando.
 */
void agroOLED_showManualConfigScreen(float currentTargetDLI);

/**
 * @brief Actualiza dinámicamente el valor numérico en la pantalla de configuración manual.
 */
void agroOLED_updateManualConfigValue(float currentTargetDLI);

/**
 * @brief Muestra la configuración de reloj dividida por sub-pasos (Horas / Minutos).
 * @param hour Hora editada (0-23).
 * @param minute Minuto editado (0-59).
 * @param subStep 0 para resaltar horas, 1 para resaltar minutos.
 */
void agroOLED_showClockConfigScreen(uint8_t hour, uint8_t minute, uint8_t subStep);

/**
 * @brief Muestra la pantalla de alerta/alarma cuando se excede el DLI máximo solar.
 */
void agroOLED_blinkAlarmScreen();

// Nueva función para renderizar el historial con scroll
void agroOLED_showHistoryScreen(AgroRegistroDLI* historial, uint8_t scroll);

#endif // AGROLUMEN_OLED_H