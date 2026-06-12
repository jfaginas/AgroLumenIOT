/**
 * @file AgroLumen_Web.cpp
 * @brief Implementación asíncrona y no bloqueante para Blynk IoT y Wi-Fi.
 */

#include "AgroLumen_Web.h"
#include "../../src/Config.h" // Rutas relativas para PlatformIO lib/

// Desactivar el comportamiento bloqueante interno de Blynk
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// Variables internas de control de estado (Encapsuladas, no visibles fuera de aquí)
static bool wifiConectadoAnteriormente = false;
static unsigned long cronoReconexion = 0;
const unsigned long TIMEOUT_RECONEXION_MS = 30000; // Intentar reconectar cada 30 segundos si cae

void agroWeb_begin() {
    Serial.println(F("[AgroWeb] Inicializando pila de red en modo asíncrono..."));

    // 1. Configurar Wi-Fi en modo Estación sin conectar inmediatamente de forma bloqueante
    WiFi.mode(WIFI_STA);
    
    // 2. Pasar las credenciales a la capa nativa. El chip gestionará la conexión en segundo plano
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // 3. Configurar Blynk con su Token de seguridad SIN iniciar Blynk.begin()
    // Esto le dice a Blynk cómo y a dónde conectarse, pero no detiene el procesador.
    Blynk.config(BLYNK_AUTH_TOKEN);
    
    Serial.println(F("[AgroWeb] Configuración web lista. El loop local iniciará de inmediato."));
}

void agroWeb_handle() {
    // Verificar el estado actual del Wi-Fi nativo
    bool wifiActual = (WiFi.status() == WL_CONNECTED);

    // Gestión de logs informativos en consola serie al cambiar de estado
    if (wifiActual && !wifiConectadoAnteriormente) {
        Serial.print(F("[AgroWeb] ¡Wi-Fi Conectado exitosamente! IP: "));
        Serial.println(WiFi.localIP());
        wifiConectadoAnteriormente = true;
    } 
    else if (!wifiActual && wifiConectadoAnteriormente) {
        Serial.println(F("[AgroWeb] Alerta: Se ha perdido la conexión Wi-Fi."));
        wifiConectadoAnteriormente = false;
        cronoReconexion = millis();
    }

    // Si hay Wi-Fi, mantenemos activa la máquina de procesamiento de Blynk (No bloqueante)
    if (wifiActual) {
        Blynk.run();
    } 
    // Si no hay Wi-Fi, evitamos llamar a Blynk.run() ya que intentará conectarse 
    // internamente de forma síncrona y podría ralentizar el polling del encoder.
    else {
        // Manejador de reconexión manual no bloqueante en caso de fallas prolongadas del router
        if (millis() - cronoReconexion >= TIMEOUT_RECONEXION_MS) {
            cronoReconexion = millis();
            Serial.println(F("[AgroWeb] Reintentando asociar Wi-Fi en segundo plano..."));
            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PASS);
        }
    }
}

void agroWeb_publish(float ppfd, float dli) {
    // Encapsulamiento total: Solo transmitimos si la red está en condiciones óptimas
    if (WiFi.status() == WL_CONNECTED && Blynk.connected()) {
        Serial.print(F("[AgroWeb] Publicando telemetría -> PPFD: "));
        Serial.print(ppfd, 2);
        Serial.print(F(" | DLI: "));
        Serial.println(dli, 2);

        // V1 para el valor instantáneo del espectrómetro, V2 para el acumulador diario
        Blynk.virtualWrite(V1, ppfd);
        Blynk.virtualWrite(V2, dli);
    } else {
        // Filosofía KISS: Si no hay red, se descarta el envío web de este bloque de 15 min.
        // El sistema local sigue integrando de forma perfecta.
        Serial.println(F("[AgroWeb] Envío omitido: Servidor o Wi-Fi no disponibles."));
    }
}