/**
 * @file AgroLumen_Sensor.cpp
 * @brief Driver de captura para AS7341 e integrador temporal para DLI.
 */

#include "AgroLumen_Sensor.h"
#include "../../src/Config.h"
#include <Wire.h>
#include <Adafruit_AS7341.h>

// Instancia estática del sensor oculta dentro del módulo
static Adafruit_AS7341 as7341;

// Variables globales del módulo (Encapsuladas)
static float ppfdActual = 0.0;
static float dliAcumulado = 0.0;

void agroSensor_begin() {
    if (!as7341.begin()) {
        Serial.println(F("[AgroSensor] Error: No se detectó el sensor AS7341 en el bus I2C."));
        // No bloqueamos con bucle infinito para permitir que el resto de las funciones sigan vivas
        return;
    }

    // Configuración óptima para medición agrícola: Ganancia intermedia para evitar saturación solar
    as7341.setGain(AS7341_GAIN_64X);
    as7341.setASTEP(599);
    as7341.setATIME(29); // Tiempo de integración balanceado
    
    Serial.println(F("[AgroSensor] Sensor multiespectral AS7341 configurado exitosamente."));
}

void agroSensor_procesarMuestreo(uint32_t segundosTranscurridos) {
    bool lecturaValida = false;
    
    // Arreglo unificado con los 8 canales PAR exactos
    as7341_color_channel_t canalesPAR[] = {
        AS7341_CHANNEL_415nm_F1, AS7341_CHANNEL_445nm_F2,
        AS7341_CHANNEL_480nm_F3, AS7341_CHANNEL_515nm_F4,
        AS7341_CHANNEL_555nm_F5, AS7341_CHANNEL_590nm_F6,
        AS7341_CHANNEL_630nm_F7, AS7341_CHANNEL_680nm_F8
    };

    // --- ALGORITMO DE AUTO-GANANCIA CORREGIDO ---
    for (uint8_t intento = 0; intento < 3; intento++) {
        if (!as7341.readAllChannels()) {
            Serial.println(F("[AgroSensor] Error al leer canales."));
            return;
        }

        // Buscamos el valor máximo de conteo entre los canales PAR
        uint16_t maxCount = 0;
        for (uint8_t i = 0; i < 8; i++) {
            uint16_t c = as7341.getChannel(canalesPAR[i]);
            if (c > maxCount) maxCount = c;
        }

        as7341_gain_t gananciaActual = as7341.getGain();

        // CORRECCIÓN: El límite real con tu ATIME/ASTEP es 18000.
        // Si maxCount llega a 17500, el integrador del chip está saturado.
        if (maxCount >= 17500) {
            if (gananciaActual > AS7341_GAIN_0_5X) {
                // Bajamos la ganancia dos escalones para adaptarnos rápido al sol
                int nuevaGanancia = (int)gananciaActual - 2;
                if (nuevaGanancia < (int)AS7341_GAIN_0_5X) nuevaGanancia = (int)AS7341_GAIN_0_5X;
                
                as7341.setGain((as7341_gain_t)nuevaGanancia);
                
                // Sincronización de Hardware: Esperamos un ciclo completo de integración (50.04ms + margen)
                delay(60); 
                as7341.readAllChannels(); // Lectura de descarte para limpiar el búfer viejo del chip
                continue;                 // Reevaluar en el siguiente ciclo del bucle
            }
        } 
        // Si la lectura es muy baja, subimos la ganancia para no perder resolución bajo sombra o LED suave
        else if (maxCount < 1500 && gananciaActual < AS7341_GAIN_64X) {
            int nuevaGanancia = (int)gananciaActual + 2;
            if (nuevaGanancia > (int)AS7341_GAIN_64X) nuevaGanancia = (int)AS7341_GAIN_64X;
            
            as7341.setGain((as7341_gain_t)nuevaGanancia);
            
            delay(60);
            as7341.readAllChannels(); // Lectura de descarte
            continue;
        }
        
        lecturaValida = true;
        break; // La ganancia ya es óptima, salimos del ajuste
    }

    if (!lecturaValida) {
        Serial.println(F("[AgroSensor] Advertencia: No se pudo estabilizar la ganancia."));
    }

    // --- 1. OBTENCIÓN DE CONTEOS NORMALIZADOS ---
    float integrationTimeMs = (29 + 1) * (599 + 1) * 0.00278f; 
    float gainMultiplier = 1.0;

    switch(as7341.getGain()) {
        case AS7341_GAIN_0_5X:  gainMultiplier = 0.5;   break;
        case AS7341_GAIN_1X:    gainMultiplier = 1.0;   break;
        case AS7341_GAIN_2X:    gainMultiplier = 2.0;   break;
        case AS7341_GAIN_4X:    gainMultiplier = 4.0;   break;
        case AS7341_GAIN_8X:    gainMultiplier = 8.0;   break;
        case AS7341_GAIN_16X:   gainMultiplier = 16.0;  break;
        case AS7341_GAIN_32X:   gainMultiplier = 32.0;  break;
        case AS7341_GAIN_64X:   gainMultiplier = 64.0;  break;
        case AS7341_GAIN_128X:  gainMultiplier = 128.0; break;
        case AS7341_GAIN_256X:  gainMultiplier = 256.0; break;
        case AS7341_GAIN_512X:  gainMultiplier = 512.0; break;
    }

    float normalizador = integrationTimeMs * gainMultiplier;
    if (normalizador == 0) normalizador = 1.0;

    // --- 2. EXTRACCIÓN Y NORMALIZACIÓN DE CANALES PAR ---
    float f1 = (float)as7341.getChannel(AS7341_CHANNEL_415nm_F1) / normalizador;
    float f2 = (float)as7341.getChannel(AS7341_CHANNEL_445nm_F2) / normalizador;
    float f3 = (float)as7341.getChannel(AS7341_CHANNEL_480nm_F3) / normalizador;
    float f4 = (float)as7341.getChannel(AS7341_CHANNEL_515nm_F4) / normalizador;
    float f5 = (float)as7341.getChannel(AS7341_CHANNEL_555nm_F5) / normalizador;
    float f6 = (float)as7341.getChannel(AS7341_CHANNEL_590nm_F6) / normalizador;
    float f7 = (float)as7341.getChannel(AS7341_CHANNEL_630nm_F7) / normalizador;
    float f8 = (float)as7341.getChannel(AS7341_CHANNEL_680nm_F8) / normalizador;

    // --- 3. SUMA ESPECTRAL PONDERADA ---
    float sumaEspectral = f1 + f2 + f3 + f4 + f5 + f6 + f7 + f8;

    // --- 4. FACTOR DE CALIBRACIÓN GLOBAL ($K_{cal}$) ---
    // Conservamos tu constante de calibración LED
    const float K_CALIBRACION = 5.30f; //2.67

    ppfdActual = sumaEspectral * K_CALIBRACION; 

    // --- INTEGRACIÓN MATEMÁTICA DEL DLI ---
    float molesEnEstePaso = ppfdActual * segundosTranscurridos * 0.000001f;
    dliAcumulado += molesEnEstePaso;

    // --- TELEMETRÍA TEMPORAL DE CONTROL ---
    Serial.print(F("[Sensor Debug] Ganancia Actual (Index): "));
    Serial.print(as7341.getGain());
    Serial.print(F(" | MaxCount Detectado: "));
    
    uint16_t debugMax = 0;
    for(uint8_t i = 0; i < 8; i++) {
        uint16_t c = as7341.getChannel(canalesPAR[i]);
        if(c > debugMax) debugMax = c;
    }
    Serial.print(debugMax);
    Serial.print(F(" | PPFD Calculado: "));
    Serial.println(ppfdActual);
}

float agroSensor_getPPFD() {
    return ppfdActual;
}

float agroSensor_getDLI() {
    return dliAcumulado;
}

void agroSensor_resetDLI() {
    dliAcumulado = 0.0;
    Serial.println(F("[AgroSensor] DLI acumulado reseteado a cero para la nueva jornada."));
}