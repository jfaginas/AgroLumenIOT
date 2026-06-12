/**
 * @file AgroLumen_OLED.cpp
 * @brief Implementación de las pantallas y layouts para el display SSD1306.
 */

#include "AgroLumen_OLED.h"
#include "../../src/Config.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C

// Instancia del display oculta dentro del módulo
static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Variables internas para la gestión del menú local
static int8_t indiceMenuActual = 0;
const int8_t TOTAL_OPCIONES_MENU = 4; // MANUAL_DLI, ADJUST_CLOCK, VIEW_HISTORY, EXIT

// Textos estáticos para el menú
static const char* menuStrings[] = {
    "1. DLI Manual",
    "2. Ajustar Reloj",
    "3. Ver Historial",
    "4. Salir"
};

void agroOLED_begin() {
    // Inicializar el display usando el bus I2C ya configurado
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println(F("[AgroOLED] Error: No se detectó la pantalla SSD1306."));
        for (;;); // Bloqueo de seguridad si no hay pantalla
    }
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 20);
    display.print(F("AgroLumenIOT Ready"));
    display.display();
    Serial.println(F("[AgroOLED] Display inicializado correctamente."));
}

void agroOLED_showMonitoringScreen(float dliActual, float dliObjetivo, float ppfdActual, uint8_t hora, uint8_t minuto, uint8_t dia, uint8_t mes, uint16_t anio) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // --- LÍNEA 1: ENCABEZADO ---
    display.setCursor(10, 0);
    display.print(F("--- MONITOR DLI ---"));

    // --- LÍNEA 2: PPFD INSTANTÁNEO ---
    display.setCursor(0, 16);
    display.print(F("PPFD: "));
    display.print(ppfdActual, 2); 

    // --- LÍNEA 3: DLI ACUMULADO vs OBJETIVO ---
    display.setCursor(0, 32);
    display.print(F("DLI ["));
    display.print(dliActual, 2);
    display.print(F("] -> ["));
    display.print(dliObjetivo, 2); 
    display.print(F("]"));

    // --- LÍNEA 4: FECHA Y HORA COMBINADAS (Aquí ya NO dice Hora Local) ---
    display.setCursor(13, 48);
    
    // Formato Fecha: DD/MM/AAAA
    if (dia < 10) display.print(F("0"));
    display.print(dia);
    display.print(F("/"));
    if (mes < 10) display.print(F("0"));
    display.print(mes);
    display.print(F("/"));
    display.print(anio);
    
    // Espaciado intermedio
    display.print(F("  "));
    
    // Formato Hora: HH:MM
    if (hora < 10) display.print(F("0"));
    display.print(hora);
    display.print(F(":"));
    if (minuto < 10) display.print(F("0"));
    display.print(minuto);

    display.display();
}

void agroOLED_showMenuScreen() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("[ MENU CONFIG ]"));

    // Dibujar las opciones y resaltar la seleccionada con una flecha o inversión
    for (int8_t i = 0; i < TOTAL_OPCIONES_MENU; i++) {
        display.setCursor(10, 16 + (i * 12));
        if (i == indiceMenuActual) {
            display.print(F("> ")); // Indicador de cursor
        } else {
            display.print(F("  "));
        }
        display.print(menuStrings[i]);
    }
    display.display();
}

void agroOLED_moveMenuCursor(int8_t direction) {
    // Modificar el índice de forma circular
    indiceMenuActual += direction;
    if (indiceMenuActual >= TOTAL_OPCIONES_MENU) indiceMenuActual = 0;
    if (indiceMenuActual < 0) indiceMenuActual = TOTAL_OPCIONES_MENU - 1;
    
    // Refrescar la pantalla inmediatamente para dar respuesta táctil al usuario
    agroOLED_showMenuScreen();
}

uint8_t agroOLED_getSelectedMenuOption() {
    return (uint8_t)indiceMenuActual;
}

void agroOLED_showManualConfigScreen(float currentTargetDLI) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("Ajuste Manual DLI"));
    
    agroOLED_updateManualConfigValue(currentTargetDLI);
}

void agroOLED_updateManualConfigValue(float currentTargetDLI) {
    // Borrar solo la zona del valor numérico para evitar parpadeos (opcional, aquí simplificado)
    display.fillRect(0, 24, 128, 20, SSD1306_BLACK);
    
    display.setTextSize(2);
    display.setCursor(30, 24);
    display.print(currentTargetDLI, 1);
    display.setTextSize(1);
    display.print(F(" mol"));
    display.display();
}

void agroOLED_showClockConfigScreen(uint8_t hour, uint8_t minute, uint8_t subStep) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("Ajustar Hora RTC"));

    display.setTextSize(2);
    display.setCursor(20, 28);

    // Si subStep == 0 estamos editando horas, la envolvemos en corchetes
    if (subStep == 0) display.print(F("["));
    if (hour < 10) display.print(F("0"));
    display.print(hour);
    if (subStep == 0) display.print(F("]"));
    
    display.print(F(":"));

    // Si subStep == 1 estamos editando minutos
    if (subStep == 1) display.print(F("["));
    if (minute < 10) display.print(F("0"));
    display.print(minute);
    if (subStep == 1) display.print(F("]"));

    display.display();
}

void agroOLED_blinkAlarmScreen() {
    static bool inverso = false;
    display.clearDisplay();
    
    if (inverso) {
        display.fillRect(0, 0, 128, 64, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
    } else {
        display.setTextColor(SSD1306_WHITE);
    }

    // 1. Título principal Centrado (Tamaño 2)
    display.setTextSize(2);
    display.setCursor(4, 12); // "ALERTA DLI" ocupa 120px. (128-120)/2 = 4px de margen.
    display.print(F("ALERTA DLI"));
    
    // 2. Subtítulo Centrado en una sola línea (Tamaño 1)
    display.setTextSize(1);
    // "Exceso Luz Detectado" tiene 20 caracteres -> 20 * 6px = 120px de ancho.
    // Margen óptimo: (128 - 120) / 2 = 4px.
    display.setCursor(4, 42); 
    display.print(F("Exceso Luz Detectado"));

    display.display();
    display.setTextColor(SSD1306_WHITE); // Restaurar color original
    inverso = !inverso;
}

/**
 * @brief Renderiza la pantalla de historial con scroll dinámico de 4 líneas consecutivas.
 * @param historial Puntero al arreglo de 15 estructuras cargadas en RAM.
 * @param scroll Índice de inicio para el renderizado (0 a 11).
 */
void agroOLED_showHistoryScreen(AgroRegistroDLI* historial, uint8_t scroll) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // --- 1. ENCABEZADO FIJO ---
    display.setCursor(16, 0);
    display.print(F("[ HISTORIAL DLI ]"));
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE); // Línea divisoria estética

    // --- 2. RENDERIZADO DE LAS 4 LÍNEAS VISIBLES ---
    for (uint8_t linea = 0; linea < 4; linea++) {
        uint8_t idxData = scroll + linea;
        if (idxData >= 15) break;

        uint8_t posY = 14 + (linea * 12);
        display.setCursor(4, posY);

        // FILTRO EVOLUCIONADO: Si el año no es el actual (2026), 
        // consideramos de forma virtual que el casillero está libre.
        if (historial[idxData].anio != 2026 || 
            historial[idxData].dia == 0 || historial[idxData].dia > 31 || 
            historial[idxData].mes == 0 || historial[idxData].mes > 12) {
            
            display.print(idxData + 1);
            display.print(F(". --/--: 0.0 mol"));
        } else {
            // Fecha real válida del año en curso
            if (historial[idxData].dia < 10) display.print(F("0"));
            display.print(historial[idxData].dia);
            display.print(F("/"));
            if (historial[idxData].mes < 10) display.print(F("0"));
            display.print(historial[idxData].mes);
            
            display.print(F(": "));
            display.print(historial[idxData].dli, 1);
            display.print(F(" mol"));
        }
    }

    // --- 3. INDICADORES DE SCROLL (FLECHAS TRIANGULARES) ---
    // Si no estamos arriba del todo (scroll > 0), hay datos arriba -> Flecha Arriba
    if (scroll > 0) {
        display.fillTriangle(120, 5, 117, 9, 123, 9, SSD1306_WHITE);
    }
    // Si no llegamos al final (scroll < 11), hay datos abajo -> Flecha Abajo
    if (scroll < 11) {
        display.fillTriangle(120, 59, 117, 55, 123, 55, SSD1306_WHITE);
    }

    display.display();
}