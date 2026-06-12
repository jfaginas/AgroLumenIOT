/**
 * @file AgroLumen_Input.cpp
 * @brief Implementación por polling no bloqueante del encoder rotativo.
 */

#include "AgroLumen_Input.h"
#include "../../src/Config.h" // Enlace a los pines centralizados

// Variables de estado internas del Encoder (Encapsuladas en el módulo)
static int estadoUltimoCLK;
static bool huboMovimiento = false;
static int8_t direccionGiro = 0;

// Variables de estado internas del Pulsador con Debounce por software
static bool botonPresionadoEfectivo = false;
static bool ultimoEstadoBoton = HIGH;
static unsigned long cronoDebounceBoton = 0;
const unsigned long TIEMPO_DEBOUNCE_MS = 50; // Tiempo de filtrado de ruido

void agroInput_begin() {
    // Configuración de pines físicos usando las constantes de Config.h
    pinMode(PIN_ENCODER_CLK, INPUT_PULLUP);
    pinMode(PIN_ENCODER_DT,  INPUT_PULLUP);
    pinMode(PIN_ENCODER_SW,  INPUT_PULLUP);

    // Leer el estado inicial del pin CLK para tener una línea de base
    estadoUltimoCLK = digitalRead(PIN_ENCODER_CLK);
}

void agroInput_update() {
    // -----------------------------------------------------------------
    // 1. POLLING DEL GIRO (ENCODER)
    // -----------------------------------------------------------------
    huboMovimiento = false;
    direccionGiro = 0;

    int estadoActualCLK = digitalRead(PIN_ENCODER_CLK);

    // Detectamos si el pin CLK cambió de estado (Flanco de subida o bajada)
    if (estadoActualCLK != estadoUltimoCLK) {
        // Para evitar lecturas dobles por paso mecánico, evaluamos solo un flanco (ej. LOW)
        if (estadoActualCLK == LOW) {
            // Si el estado de DT es diferente a CLK, el giro es horario
            if (digitalRead(PIN_ENCODER_DT) != estadoActualCLK) {
                direccionGiro = 1;  // Sentido Horario (Incrementar)
            } else {
                direccionGiro = -1; // Sentido Antihorario (Decrementar)
            }
            huboMovimiento = true;
        }
    }
    estadoUltimoCLK = estadoActualCLK; // Guardamos el estado para el próximo ciclo

/*     // -----------------------------------------------------------------
    // 2. POLLING DEL PULSADOR (SW) CON DEBOUNCE ASÍNCRONO
    // -----------------------------------------------------------------
    botonPresionadoEfectivo = false;
    int lecturaBoton = digitalRead(PIN_ENCODER_SW);

    // Si el estado físico cambió (por ruido o pulsación real), reseteamos el cronómetro
    if (lecturaBoton != ultimoEstadoBoton) {
        cronoDebounceBoton = millis();
    }

    // Si pasó más tiempo que el umbral de debounce, validamos el estado estable
    if ((millis() - cronoDebounceBoton) > TIEMPO_DEBOUNCE_MS) {
        // Detectamos el flanco de bajada (de HIGH a LOW) que indica que fue presionado
        if (lecturaBoton == LOW && ultimoEstadoBoton == HIGH) {
            botonPresionadoEfectivo = true;
        }
    }
    
    ultimoEstadoBoton = lecturaBoton; */

    // -----------------------------------------------------------------
    // 2. POLLING DEL PULSADOR (SW) CON DEBOUNCE ULTRA-ESTABLE
    // -----------------------------------------------------------------
    botonPresionadoEfectivo = false;
    int lecturaBoton = digitalRead(PIN_ENCODER_SW);

    // Si el estado físico cambió (por ruido o pulsación real), guardamos el tiempo
    if (lecturaBoton != ultimoEstadoBoton) {
        cronoDebounceBoton = millis();
        ultimoEstadoBoton = lecturaBoton;
    }

    // Solo si el estado se mantiene estable durante el tiempo de debounce
    if ((millis() - cronoDebounceBoton) > TIEMPO_DEBOUNCE_MS) {
        // Usamos una variable estática para recordar el último estado ESTABLE validado
        static int estadoEstableAnterior = HIGH;
        
        // Detectamos el flanco de bajada real: antes estaba en HIGH (suelto) y ahora está en LOW (presionado)
        if (lecturaBoton == LOW && estadoEstableAnterior == HIGH) {
            botonPresionadoEfectivo = true;
            Serial.println(F("[INPUT] ¡Pulsación única validada por software!"));
        }
        
        estadoEstableAnterior = lecturaBoton;
    }

/*     // LÍNEAS TEMPORALES DE DIAGNÓSTICO
    if (digitalRead(PIN_ENCODER_CLK) == LOW || digitalRead(PIN_ENCODER_SW) == LOW) {
        Serial.printf("[TEST INPUT] CLK: %d | DT: %d | SW: %d\n", 
                      digitalRead(PIN_ENCODER_CLK), digitalRead(PIN_ENCODER_DT), digitalRead(PIN_ENCODER_SW));
    } */

}

bool agroInput_isButtonPressed() {
    return botonPresionadoEfectivo;
}

bool agroInput_hasTurned() {
    return huboMovimiento;
}

int8_t agroInput_getTurnDirection() {
    return direccionGiro;
}