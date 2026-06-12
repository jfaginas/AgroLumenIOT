#ifndef AGROLUMEN_WEB_H
#define AGROLUMEN_WEB_H

#include <Arduino.h>

/**
 * @brief Inicializa el hardware Wi-Fi y prepara la configuración de Blynk.
 * Diseñado para arrancar de forma segura.
 */
void agroWeb_begin();

/**
 * @brief Mantiene viva la pila de conexión de Blynk de forma no bloqueante.
 * Debe llamarse frecuentemente en el loop principal.
 */
void agroWeb_handle();

/**
 * @brief Publica los datos actuales de radiación y DLI acumulado a la nube.
 * @param ppfd Valor instantáneo de PPFD (µmol/m²/s).
 * @param dli Valor acumulado de DLI diario (mol/m²/d).
 */
void agroWeb_publish(float ppfd, float dli);

#endif // AGROLUMEN_WEB_H