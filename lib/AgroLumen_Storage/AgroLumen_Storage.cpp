/**
 * @file AgroLumen_Storage.cpp
 * @brief Implementación del controlador de tiempo DS3231 y persistencia I2C en AT24C32.
 */

#include "AgroLumen_Storage.h"
#include "../../src/Config.h"
#include <Wire.h>
#include <RtcDS3231.h>

// Direcciones físicas I2C fijas en el módulo físico
#define EEPROM_I2C_ADDRESS 0x57 

// Direcciones de memoria fijas dentro de la AT24C32 para configuraciones (2 bytes por float)
#define ADDR_TARGET_DLI    0x0000

// El historial circular de jornadas comenzará a partir de la dirección 0x0010
#define ADDR_HISTORY_START 0x0010
#define MAX_REGISTROS      100    // Capacidad del búfer circular para AgroLumenIOT

// Instancia estática del RTC oculta en el módulo
static RtcDS3231<TwoWire> rtc(Wire);

void agroStorage_begin() {
    rtc.Begin();

    if (!rtc.IsDateTimeValid()) {
        Serial.println(F("[AgroStorage] Alerta: RTC perdió la hora. Configurando base..."));
        // Fecha de compilación aproximada como salvaguarda inicial si la pila falló
        rtc.SetDateTime(RtcDateTime(__DATE__, __TIME__));
    }

    // Asegurar que el oscilador interno del DS3231 esté activo
    rtc.Enable32kHzPin(false);
    rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
    
    Serial.println(F("[AgroStorage] RTC DS3231 y EEPROM AT24C32 enlazados de forma exitosa."));
}

void agroStorage_getTime(uint8_t* hora, uint8_t* minuto, uint8_t* segundo) {
    RtcDateTime now = rtc.GetDateTime();
    *hora = now.Hour();
    *minuto = now.Minute();
    *segundo = now.Second();
}

void agroStorage_getDate(uint8_t* dia, uint8_t* mes, uint16_t* anio) {
    RtcDateTime now = rtc.GetDateTime();
    *dia = now.Day();
    *mes = now.Month();
    *anio = now.Year();
}

void agroStorage_setTime(uint8_t hora, uint8_t minuto) {
    RtcDateTime now = rtc.GetDateTime();
    // Preservamos Año, Mes y Día actuales, reescribiendo solo hora, minuto y reseteando segundos
    RtcDateTime nuevoTiempo(now.Year(), now.Month(), now.Day(), hora, minuto, 0);
    rtc.SetDateTime(nuevoTiempo);
    Serial.println(F("[AgroStorage] Hora del hardware actualizada correctamente."));
}

float agroStorage_readLastTargetDLI() {
    Wire.beginTransmission(EEPROM_I2C_ADDRESS);
    Wire.write((int)(ADDR_TARGET_DLI >> 8));   // MSB de la dirección
    Wire.write((int)(ADDR_TARGET_DLI & 0xFF)); // LSB de la dirección
    if (Wire.endTransmission() != 0) return 15.0; // Valor por defecto seguro si falla el bus

    Wire.requestFrom(EEPROM_I2C_ADDRESS, 2);
    if (Wire.available() == 2) {
        uint8_t parteEntera = Wire.read();
        uint8_t parteDecimal = Wire.read();
        if (parteEntera == 0xFF) return 15.0; // Memoria virgen
        return (float)parteEntera + ((float)parteDecimal / 10.0);
    }
    return 15.0;
}

void agroStorage_writeLastTargetDLI(float targetDLI) {
    uint8_t parteEntera = (uint8_t)targetDLI;
    uint8_t parteDecimal = (uint8_t)((targetDLI - parteEntera) * 10);

    Wire.beginTransmission(EEPROM_I2C_ADDRESS);
    Wire.write((int)(ADDR_TARGET_DLI >> 8));
    Wire.write((int)(ADDR_TARGET_DLI & 0xFF));
    Wire.write(parteEntera);
    Wire.write(parteDecimal);
    Wire.endTransmission();
    delay(5); // Tiempo físico necesario de escritura de página de la EEPROM (Write Cycle)
}

// ============================================================================
// FUNCIONES ANTERIORES (Mantenidas para evitar errores de enlace en el main)
// ============================================================================

void agroStorage_saveDayRecord(AgroJornada jornada) {
    uint16_t direccionEscritura = ADDR_HISTORY_START;

    Wire.beginTransmission(EEPROM_I2C_ADDRESS);
    Wire.write((int)(direccionEscritura >> 8));
    Wire.write((int)(direccionEscritura & 0xFF));
    
    Wire.write(jornada.dliSolar);
    Wire.write(jornada.dliLed);
    Wire.write(jornada.fotoperiodo);
    Wire.write(jornada.plantaPreset);
    
    Wire.endTransmission();
    delay(5); 
    Serial.println(F("[AgroStorage] Histórico de la jornada persistido en la EEPROM local."));
}

AgroJornada agroStorage_readDayHistory(uint16_t diaOffset) {
    AgroJornada registroVacio = {0, 0, 0, 0};
    uint16_t direccionLectura = ADDR_HISTORY_START; 

    Wire.beginTransmission(EEPROM_I2C_ADDRESS);
    Wire.write((int)(direccionLectura >> 8));
    Wire.write((int)(direccionLectura & 0xFF));
    if (Wire.endTransmission() != 0) return registroVacio;

    Wire.requestFrom(EEPROM_I2C_ADDRESS, 4);
    if (Wire.available() == 4) {
        registroVacio.dliSolar     = Wire.read();
        registroVacio.dliLed       = Wire.read();
        registroVacio.fotoperiodo  = Wire.read();
        registroVacio.plantaPreset = Wire.read();
    }
    return registroVacio;
}


// ============================================================================
// NUEVAS FUNCIONES DE PERSISTENCIA PARA HISTORIAL DE 15 DÍAS (ETAPA 1)
// ============================================================================

/**
 * @brief Guarda un registro de DLI + Fecha en un índice específico del historial (0 al 14).
 * @param index Posición en el arreglo del historial (0 = el más reciente / ayer).
 * @param record Estructura que contiene los datos a persistir.
 */
void agroStorage_writeHistoryRecord(uint8_t index, AgroRegistroDLI record) {
    if (index >= 15) return; // Protección contra desborde de límites

    // Calculamos la dirección física I2C exacta para este bloque (8 bytes por registro)
    uint16_t address = ADDR_HISTORY_START + (index * 8);

    Wire.beginTransmission(EEPROM_I2C_ADDRESS);
    Wire.write((int)(address >> 8));   // Byte alto de la dirección (MSB)
    Wire.write((int)(address & 0xFF)); // Byte bajo de la dirección (LSB)

    // 1. Volcamos los 4 bytes del float (DLI) apuntando directamente a su espacio de memoria
    uint8_t* pFloat = (uint8_t*)&record.dli;
    Wire.write(pFloat[0]);
    Wire.write(pFloat[1]);
    Wire.write(pFloat[2]);
    Wire.write(pFloat[3]);

    // 2. Volcamos la Fecha (Día, Mes, Año)
    Wire.write(record.dia);
    Wire.write(record.mes);
    
    // Aislamos la variable antes del corrimiento
    Wire.write((int)((record.anio >> 8) & 0xFF)); // Byte alto del año de 16 bits (MSB)
    Wire.write((int)(record.anio & 0xFF));        // Byte bajo del año de 16 bits (LSB)

    Wire.endTransmission();
    delay(5); // Espera de hardware obligatoria para el ciclo de escritura de la EEPROM
}

/**
 * @brief Lee el historial completo de 15 días desde la EEPROM y lo vuelca en la RAM.
 * @param historyDest Puntero al arreglo de 15 estructuras de destino en la RAM.
 */
void agroStorage_readFullHistory(AgroRegistroDLI* historyDest) {
    for (uint8_t i = 0; i < 15; i++) {
        uint16_t address = ADDR_HISTORY_START + (i * 8);

        Wire.beginTransmission(EEPROM_I2C_ADDRESS);
        Wire.write((int)(address >> 8));
        Wire.write((int)(address & 0xFF));
        
        if (Wire.endTransmission() != 0) {
            // Protección por falla del bus: limpiamos memoria para evitar mostrar basura en pantalla
            historyDest[i] = {0.0f, 0, 0, 0};
            continue;
        }

        // Solicitamos los 8 bytes que componen el bloque diario
        Wire.requestFrom(EEPROM_I2C_ADDRESS, 8);
        if (Wire.available() == 8) {
            // Reconstruimos el float de DLI byte por byte
            uint8_t bufferFloat[4];
            bufferFloat[0] = Wire.read();
            bufferFloat[1] = Wire.read();
            bufferFloat[2] = Wire.read();
            bufferFloat[3] = Wire.read();
            historyDest[i].dli = *(float*)bufferFloat;

            // Reconstruimos la Fecha
            historyDest[i].dia = Wire.read();
            historyDest[i].mes = Wire.read();
            
            uint8_t anioMSB = Wire.read();
            uint8_t anioLSB = Wire.read();
            historyDest[i].anio = ((uint16_t)anioMSB << 8) | anioLSB;
        } else {
            // Si el chip no responde con los bytes esperados, inicializamos el índice seguro
            historyDest[i] = {0.0f, 0, 0, 0};
        }
    }
}

/**
 * @brief Desplaza el historial un día hacia atrás e inserta la nueva jornada en el índice 0.
 * @param newRecord Estructura con el DLI finalizado y la fecha del día que cierra.
 */
void agroStorage_archiveNewDay(AgroRegistroDLI newRecord) {
    // 1. Corrimiento: Movemos los registros viejos una posición hacia atrás.
    // Arrancamos desde el penúltimo (13) y lo copiamos sobre el último (14), y así sucesivamente.
    for (int i = 13; i >= 0; i--) {
        AgroRegistroDLI temp;
        
        // Leemos el registro en la posición 'i'
        uint16_t addrRead = ADDR_HISTORY_START + (i * 8);
        Wire.beginTransmission(EEPROM_I2C_ADDRESS);
        Wire.write((int)(addrRead >> 8));
        Wire.write((int)(addrRead & 0xFF));
        if (Wire.endTransmission() == 0) {
            Wire.requestFrom(EEPROM_I2C_ADDRESS, 8);
            if (Wire.available() == 8) {
                uint8_t bufferFloat[4];
                bufferFloat[0] = Wire.read(); bufferFloat[1] = Wire.read();
                bufferFloat[2] = Wire.read(); bufferFloat[3] = Wire.read();
                temp.dli = *(float*)bufferFloat;
                temp.dia = Wire.read();
                temp.mes = Wire.read();
                uint8_t msb = Wire.read(); uint8_t lsb = Wire.read();
                temp.anio = ((uint16_t)msb << 8) | lsb;
                
                // Lo escribimos en la posición siguiente (i + 1)
                agroStorage_writeHistoryRecord(i + 1, temp);
            }
        }
    }

    // 2. Guardamos el día que acaba de terminar en la posición 0 (Ayer)
    agroStorage_writeHistoryRecord(0, newRecord);
    Serial.println(F("[AgroStorage] Historial rotado con éxito. Nueva jornada archivada en Index 0."));
}