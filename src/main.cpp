/**
 * @file main.cpp
 * @project AgroLumenIOT
 * @brief Orquestador principal y FSM asíncrona con conectividad Blynk.
 * @version 2.0.0
 * @date 2026-06-04
 */

#include <Arduino.h>
#include <Wire.h>
#include "Config.h"

// Inclusión de los módulos nativos encapsulados desde /lib
#include <AgroLumen_FSM.h>   // Define el enum class SystemState
#include <AgroLumen_Input.h> // Gestión por polling del Encoder y botón
#include <AgroLumen_OLED.h>  // Layouts de pantalla en el SSD1306
#include <AgroLumen_Storage.h> // Controlador de tiempo DS3231 y EEPROM AT24C32
#include <AgroLumen_Sensor.h>  // Driver del espectrómetro AS7341 y DLI
#include <AgroLumen_Web.h>     // Conectividad Wi-Fi y Blynk no bloqueante

// Variable de estado global del sistema (C++17 enum class)
SystemState currentState = SystemState::MONITORING;

// Cronómetros no bloqueantes (en milisegundos)
uint32_t lastControlTick = 0;
uint32_t lastWebUploadTick = 0;

// Banderas de control diario
bool hasSavedToday = false;
bool alarmaSilenciada = false;

// Variables auxiliares para la sub-máquina de ajuste de reloj local
uint8_t clockSubStep = 0; // 0: Horas, 1: Minutos
uint8_t tempHour = 12;
uint8_t tempMinute = 0;

// Variables globales temporales para el control de jornada
float dliObjetivo = 15.0;

// Control de transición de jornada (Etapa 2)
static uint8_t ultimoDiaArchivado = 0;

void setup() {
    Serial.begin(115200);
    Serial.println(F("--- Inicializando Sistema AgroLumenIOT ---"));

    // 1. Inicializar el canal físico I2C compartido (Pines 21 y 22)
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setClock(400000); // 400kHz I2C Fast Mode

    // 2. Inicializar subsistemas de hardware y memoria local
    agroStorage_begin();
    agroOLED_begin();
    agroInput_begin();
    agroSensor_begin();

    // Sincronizar el día inicial para el historial automático (Etapa 2)
    uint8_t d, m; uint16_t a;
    agroStorage_getDate(&d, &m, &a);
    ultimoDiaArchivado = d; 

    // 3. Inicializar la pila de red en segundo plano (No Bloqueante)
    agroWeb_begin();

    // 4. Recuperar configuraciones previas del hardware
    dliObjetivo = agroStorage_readLastTargetDLI();

    // 5. Configurar hardware de control
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);

    // Sincronizar el tick de inicio
    lastControlTick = millis();
    lastWebUploadTick = millis();

    Serial.println(F("[Main] AgroLumenIOT listo y corriendo de forma cooperativa."));
}

/**
 * @brief Verifica si cambió el día en el RTC para archivar el DLI acumulado y resetear el contador.
 */
// --- VARIABLE GLOBAL NUEVA PARA EL SCROLL (Etapa 4 preliminar) ---
static uint8_t indiceScroll = 0;
static AgroRegistroDLI historialRAM[15]; // Búfer en RAM para no estresar la EEPROM al scrollear

/**
 * @brief Centinela global de tiempo: Ejecuta el archivado de 15 días a la medianoche.
 * Se ejecuta de forma asíncrona en la raíz del loop.
 */
void verificarCambioDeDia() {
    uint8_t h, m, s;
    uint8_t dia, mes;
    uint16_t anio;
    
    // Leemos el tiempo y fecha actuales del RTC
    agroStorage_getTime(&h, &m, &s);
    agroStorage_getDate(&dia, &mes, &anio);

    // Si el día físico del RTC no coincide con el último archivado, cruzamos las 00:00
    if (dia != ultimoDiaArchivado) {
        Serial.println(F("[Main] Cambio de jornada detectado. Archivando DLI de ayer..."));
        
        // Estructuramos el registro con los datos del día que acaba de concluir
        AgroRegistroDLI jornadaTerminada;
        jornadaTerminada.dli  = agroSensor_getDLI();
        jornadaTerminada.dia  = ultimoDiaArchivado; // El día real donde se acumuló la luz
        jornadaTerminada.mes  = mes;  // Simplificación operativa para el cambio de mes
        jornadaTerminada.anio = anio;

        // Desplazamos la bitácora en la EEPROM y guardamos en Index 0
        agroStorage_archiveNewDay(jornadaTerminada);
        
        // Reseteamos el integrador del sensor a cero para arrancar el nuevo día limpio
        agroSensor_resetDLI();
        
        // Rehabilitamos controles de alarmas diarias
        alarmaSilenciada = false;
        
        // Actualizamos el centinela para congelar el proceso hasta la próxima medianoche
        ultimoDiaArchivado = dia;
    }
}

void loop() {
    // -----------------------------------------------------------------
    // TAREAS CRÍTICAS DE MÁXIMA VELOCIDAD (POLLING ASÍNCRONO)
    // -----------------------------------------------------------------
    agroInput_update(); // Escaneo del encoder y debounce del botón
    agroWeb_handle();   // Mantenimiento de la pila de Blynk si hay Wi-Fi
    
    // Centinela asíncrono de medianoche: Corre SIEMPRE, independiente de la pantalla
    verificarCambioDeDia();

    // -----------------------------------------------------------------
    // MÁQUINA DE ESTADOS FINITOS (FSM)
    // -----------------------------------------------------------------
    switch (currentState) {
        
        case SystemState::MONITORING: {
            // Tarea periódica secuencial local (Cada 2 segundos)
            if (millis() - lastControlTick >= CONTROL_INTERVAL_MS) {
                lastControlTick = millis();
                
                // Procesar muestreo integrando el tiempo transcurrido (2 segundos)
                agroSensor_procesarMuestreo(CONTROL_INTERVAL_MS / 1000);
                
                uint8_t h, m, s;
                uint8_t dia, mes;
                uint16_t anio;
                agroStorage_getTime(&h, &m, &s);
                agroStorage_getDate(&dia, &mes, &anio);
    
                // Actualizar pantalla con el Layout Completo
                agroOLED_showMonitoringScreen(agroSensor_getDLI(), dliObjetivo, agroSensor_getPPFD(), h, m, dia, mes, anio);

                // --- EVALUACIÓN DE ALERTA POR EXCESO SOLAR (FILTRADA) ---
                if (agroSensor_getDLI() > (dliObjetivo + 5.0f) && !alarmaSilenciada) {
                    currentState = SystemState::ALARM_TRIGGERED;
                }
            }

            // --- CRONÓMETRO ASÍNCRONO DE SUBIDA WEB (Cada 15 minutos) ---
            if (millis() - lastWebUploadTick >= WEB_UPLOAD_INTERVAL_MS) {
                lastWebUploadTick = millis();
                agroWeb_publish(agroSensor_getPPFD(), agroSensor_getDLI());
            }

            // Transición por interacción del usuario
            if (agroInput_isButtonPressed()) {
                currentState = SystemState::MENU_SELECT;
                agroOLED_showMenuScreen();
            }
            break;
        }

        case SystemState::MENU_SELECT: {
            if (agroInput_hasTurned()) {
                agroOLED_moveMenuCursor(agroInput_getTurnDirection());
            }

            if (agroInput_isButtonPressed()) {
                uint8_t option = agroOLED_getSelectedMenuOption();
                
                if (option == AgroMenu::MANUAL_DLI) {
                    currentState = SystemState::MANUAL_CONFIG;
                    agroOLED_showManualConfigScreen(dliObjetivo);
                } 
                else if (option == AgroMenu::ADJUST_CLOCK) {
                    currentState = SystemState::CLOCK_CONFIG;
                    clockSubStep = 0; 
                    uint8_t seg;
                    agroStorage_getTime(&tempHour, &tempMinute, &seg);
                    agroOLED_showClockConfigScreen(tempHour, tempMinute, clockSubStep);
                } 
                else if (option == AgroMenu::VIEW_HISTORY) {
                    currentState = SystemState::HISTORY_VIEW;
                    indiceScroll = 0; // Inicializamos el visor desde el día más reciente (Ayer)
                    
                    // Traemos los 15 días guardados en la EEPROM hacia la RAM una única vez al entrar
                    agroStorage_readFullHistory(historialRAM);
                    
                    // LLAMADO DEFINITIVO (Etapa 4): Envía los datos para dibujar en el OLED
                    agroOLED_showHistoryScreen(historialRAM, indiceScroll); 
                }
                else if (option == AgroMenu::EXIT) {
                    currentState = SystemState::MONITORING;
                }
            }
            break;
        }

        case SystemState::MANUAL_CONFIG: {
            if (agroInput_hasTurned()) {
                dliObjetivo += (agroInput_getTurnDirection() * 0.5f);
                if (dliObjetivo < 0.0f)  dliObjetivo = 0.0f;
                if (dliObjetivo > 50.0f) dliObjetivo = 50.0f;
                agroOLED_updateManualConfigValue(dliObjetivo);
            }

            if (agroInput_isButtonPressed()) {
                agroStorage_writeLastTargetDLI(dliObjetivo);
                currentState = SystemState::MONITORING;
            }
            break;
        }

        case SystemState::CLOCK_CONFIG: {
            if (agroInput_hasTurned()) {
                int8_t dir = agroInput_getTurnDirection();
                if (clockSubStep == 0) { 
                    int16_t calculo = tempHour + dir;
                    if (calculo > 23) calculo = 0;
                    if (calculo < 0)  calculo = 23;
                    tempHour = (uint8_t)calculo;
                } else { 
                    int16_t calculo = tempMinute + dir;
                    if (calculo > 59) calculo = 0;
                    if (calculo < 0)  calculo = 59;
                    tempMinute = (uint8_t)calculo;
                }
                agroOLED_showClockConfigScreen(tempHour, tempMinute, clockSubStep);
            }

            if (agroInput_isButtonPressed()) {
                if (clockSubStep == 0) {
                    clockSubStep = 1; 
                    agroOLED_showClockConfigScreen(tempHour, tempMinute, clockSubStep);
                } else {
                    agroStorage_setTime(tempHour, tempMinute);
                    currentState = SystemState::MONITORING;
                }
            }
            break;
        }

        case SystemState::HISTORY_VIEW: {
            // --- ETAPA 4: CAPTURA DEL GIRO DEL ENCODER PARA NAVEGAR EL SCROLL ---
            if (agroInput_hasTurned()) {
                int8_t dir = agroInput_getTurnDirection(); // Retorna 1 o -1
                
                // Si gira a la derecha (1) y no llegamos al límite inferior (15 días - 4 visibles = índice 11)
                if (dir > 0 && indiceScroll < 11) {
                    indiceScroll++;
                    agroOLED_showHistoryScreen(historialRAM, indiceScroll); // <-- ASEGURATE DE QUE ESTÉ DESCOMENTADA
                }
                // Si gira a la izquierda (-1) y no estamos al inicio
                else if (dir < 0 && indiceScroll > 0) {
                    indiceScroll--;
                    agroOLED_showHistoryScreen(historialRAM, indiceScroll); // <-- ASEGURATE DE QUE ESTÉ DESCOMENTADA
                }
            }

            // Al presionar el botón salimos de la vista de historial de vuelta al menú de selección
            if (agroInput_isButtonPressed()) {
                currentState = SystemState::MENU_SELECT;
                agroOLED_showMenuScreen(); // Redibujar el menú al volver
            }
            break;
        }

        case SystemState::ALARM_TRIGGERED: {
            static uint32_t cronoAlarma = 0;
            static bool estadoAlarma = false;
            
            if (millis() - cronoAlarma >= 500) {
                cronoAlarma = millis();
                estadoAlarma = !estadoAlarma;
                digitalWrite(PIN_BUZZER, estadoAlarma ? HIGH : LOW);
                agroOLED_blinkAlarmScreen();
            }

            if (agroInput_isButtonPressed()) {
                digitalWrite(PIN_BUZZER, LOW);
                alarmaSilenciada = true; 
                currentState = SystemState::MONITORING;
                Serial.println(F("[Main] Alarma de DLI silenciada por el usuario."));
            }
            
            if (agroSensor_getDLI() <= dliObjetivo) {
                digitalWrite(PIN_BUZZER, LOW);
                currentState = SystemState::MONITORING;
            }
            break;
        }
    }
}