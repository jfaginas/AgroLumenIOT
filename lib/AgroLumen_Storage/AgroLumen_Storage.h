#ifndef AGROLUMEN_STORAGE_H
#define AGROLUMEN_STORAGE_H

#include <Arduino.h>

// Estructura antigua (Mantenida para no romper compatibilidad con código existente)
struct AgroJornada {
    uint8_t dliSolar;     
    uint8_t dliLed;       
    uint8_t fotoperiodo;  
    uint8_t plantaPreset; 
};

// NUEVA ESTRUCTURA: Almacenamiento de 8 bytes para el DLI real con su fecha (Etapa 1)
struct AgroRegistroDLI {
    float dli;
    uint8_t dia;
    uint8_t mes;
    uint16_t anio;
};

/**
 * @brief Inicializa el chip RTC DS3231 y verifica la comunicación con la EEPROM AT24C32.
 */
void agroStorage_begin();

/**
 * @brief Obtiene las componentes de la hora actual del RTC.
 */
void agroStorage_getTime(uint8_t* hora, uint8_t* minuto, uint8_t* segundo);

/**
 * @brief Obtiene las componentes de la fecha actual del RTC.
 */
void agroStorage_getDate(uint8_t* dia, uint8_t* mes, uint16_t* anio);

/**
 * @brief Actualiza manualmente la hora y el minuto en el hardware del RTC.
 */
void agroStorage_setTime(uint8_t hora, uint8_t minuto);

/**
 * @brief Lee el último DLI objetivo guardado por el usuario para restaurarlo tras un apagón.
 */
float agroStorage_readLastTargetDLI();

/**
 * @brief Guarda de forma permanente el DLI objetivo seleccionado por el usuario.
 */
void agroStorage_writeLastTargetDLI(float targetDLI);

/**
 * @brief Almacena el registro de una jornada en el búfer circular antiguo.
 */
void agroStorage_saveDayRecord(AgroJornada jornada);

/**
 * @brief Lee un registro histórico guardado según un índice de desplazamiento antiguo.
 */
AgroJornada agroStorage_readDayHistory(uint16_t diaOffset);


// ============================================================================
// NUEVAS NUEVAS NATIVAS: HISTORIAL DE 15 DÍAS CON COHERENCIA DE IDIOMA
// ============================================================================

/**
 * @brief Guarda un registro de DLI + Fecha en un índice específico del historial (0 al 14).
 */
void agroStorage_writeHistoryRecord(uint8_t index, AgroRegistroDLI record);

/**
 * @brief Lee el historial completo de 15 días desde la EEPROM y lo vuelca en la RAM.
 */
void agroStorage_readFullHistory(AgroRegistroDLI* historyDest);

void agroStorage_archiveNewDay(AgroRegistroDLI newRecord);

#endif // AGROLUMEN_STORAGE_H