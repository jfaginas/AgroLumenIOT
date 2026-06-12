#ifndef AGROLUMEN_SENSOR_H
#define AGROLUMEN_SENSOR_H

#include <Arduino.h>

/**
 * @brief Inicializa el sensor multiespectral AS7341 y configura sus ganancias básicas.
 */
void agroSensor_begin();

/**
 * @brief Realiza la lectura de los canales espectrales, calcula el PPFD instantáneo
 * y acumula el DLI en base al tiempo transcurrido.
 * @param segundosTranscurridos Tiempo delta desde la última lectura para la integral.
 */
void agroSensor_procesarMuestreo(uint32_t segundosTranscurridos);

/**
 * @brief Obtiene el último valor calculado de PPFD.
 * @return Densidad de flujo de fotones fotosintéticos en umol/m2/s.
 */
float agroSensor_getPPFD();

/**
 * @brief Obtiene el acumulado diario de DLI.
 * @return Integral de luz diaria en mol/m2/d.
 */
float agroSensor_getDLI();

/**
 * @brief Resetea a cero el contador de DLI acumulado (Llamado típicamente a la medianoche).
 */
void agroSensor_resetDLI();

#endif // AGROLUMEN_SENSOR_H