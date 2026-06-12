#ifndef AGROLUMEN_INPUT_H
#define AGROLUMEN_INPUT_H

#include <Arduino.h>

/**
 * @brief Inicializa los pines del encoder (CLK, DT, SW) con sus resistencias de Pull-Up.
 */
void agroInput_begin();

/**
 * @brief Realiza el escaneo (polling) del estado del encoder y el pulsador.
 * Debe ejecutarse en cada iteración del loop principal a máxima velocidad.
 */
void agroInput_update();

/**
 * @brief Verifica si el botón del encoder fue presionado (detecta el flanco de bajada).
 * @return true si el botón fue pulsado de forma efectiva, false en caso contrario.
 */
bool agroInput_isButtonPressed();

/**
 * @brief Verifica si el encoder ha girado en este ciclo.
 * @return true si hubo movimiento, false si está estático.
 */
bool agroInput_hasTurned();

/**
 * @brief Obtiene la dirección del último giro detectado.
 * @return 1 para giro horario (incrementar), -1 para antihorario (decrementar), 0 si no se movió.
 */
int8_t agroInput_getTurnDirection();

#endif // AGROLUMEN_INPUT_H