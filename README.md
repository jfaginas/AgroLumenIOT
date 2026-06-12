# AgroLumenIOT 🌿💡

Monitor de radiación lumínica y registrador de DLI basado en ESP32 con conectividad IoT.
Un proyecto de hardware y software abierto para medir la luz en cultivos mediante un sensor con ganancia automática (AGC). El sistema calcula el DLI diario de forma continua, gestiona alarmas por exceso de luz y guarda automáticamente un historial de los últimos 15 días en una memoria EEPROM externa para evitar pérdidas ante cortes de energía. Los datos se publican en tiempo real en la plataforma Blynk para su monitoreo remoto, mientras que el control local se maneja de forma fluida a través de un menú en pantalla OLED y un encoder rotativo. Se lo ha programado de manera eficiente, usando una máquina de estados (FSM) y obviamente sin funciones bloqueantes en PlatformIO + VsCode.

---

## 📌 Características Principales

* **Muestreo Multiespectral Avanzado:** Captura precisa de PPFD mediante optimización dinámica de ganancia e integración continua en tiempo real.
* **Control Automático de Ganancia (AGC):** Algoritmo nativo adaptativo que ajusta dinámicamente los tiempos de integración y la ganancia del sensor óptico. Esto previene la saturación bajo radiación solar pico y maximiza la resolución en condiciones de baja luminosidad.
* **Bitácora Local de 15 Días:** Persistencia automatizada de datos a la medianoche (00:00) en memoria EEPROM externa mediante corrimiento físico de bloques, inmune a cortes de energía.
* **Interfaz de Usuario OLED (KISS):** Menú de configuración local y navegación fluida por historial mediante Encoder Rotativo con scroll dinámico de 4 líneas consecutivas.
* **Seguridad y Control:** Sistema de alarma acústica/visual intermitente no bloqueante por exceso de DLI con opción de silenciamiento manual por hardware para la jornada en curso.
* **Conectividad IoT:** Pila de red asíncrona dedicada para la publicación remota de métricas a través de Blynk.

---

## 🔌 Componentes de Hardware Utilizados

Para garantizar la precisión en las mediciones y la robustez en el almacenamiento de datos, se seleccionaron los siguientes componentes físicos:

1. **Microcontrolador principal:** * **Placa de desarrollo ESP32 DevKit v1** (Espressif Systems). Cuenta con un microprocesador Dual-Core de 32 bits a 240 MHz, memoria SRAM interna suficiente para el búfer del historial, y conectividad Wi-Fi nativa para la sincronización con Blynk.
2. **Sensor Óptico Multiespectral:** * **Sensor AS7341** (o compatible de alta precisión). Conexión mediante bus I2C. Cuenta con canales espectrales específicos para capturar la irradiancia en diferentes longitudes de onda, permitiendo el cálculo preciso de PPFD bajo el control de su algoritmo AGC adaptativo.
3. **Módulo de Tiempo Real y Almacenamiento Persistente:**
   * **Módulo RTC DS3231 + EEPROM AT24C32** (Módulo I2C integrado). 
     * **Reloj (DS3231):** Oscilador de cristal compensado por temperatura (TCXO) que garantiza que el centinela de medianoche detecte las `00:00:00` con máxima precisión cronométrica.
     * **Memoria (AT24C32):** Chip EEPROM externo de $32\text{ Kbits}$ ($4\text{ KB}$ de capacidad), utilizado para el guardado físico y cíclico de los 120 bytes de la bitácora de 15 días.
4. **Unidad de Interfaz Visual:**
   * **Display OLED SSD1306** ($128 \times 64$ píxeles, monocromo). Pantalla I2C de alto contraste encargada de renderizar el monitor principal, las alertas intermitentes y las 4 líneas consecutivas del menú de historial.
5. **Periférico de Control Humano:**
   * **Encoder Rotativo Mecánico (EC11 o similar) con Pulsador Integrado.** Dispositivo utilizado para el desplazamiento ágil por el menú de configuración y para controlar el scroll dinámico del historial de datos.
6. **Actuador de Alerta:**
   * **Buzzer Piezoeléctrico Activo (5V).** Encargado de emitir la señal acústica intermitente no bloqueante cuando el DLI acumulado supera el umbral objetivo fijado por el usuario.

---

## 🛠️ Cuadro de Conexiones y Pines (Pinout ESP32 DevKit v1)

El sistema utiliza el bus físico **I2C compartido** operando en **Fast Mode (400 kHz)** para interconectar los sensores, la pantalla y el módulo de memoria. La captura del encoder rotativo y el pulsador se realiza mediante **Polling Asincrónico** de alta velocidad en la raíz del bucle principal, garantizando un filtrado de rebotes de señal (debounce) eficiente por software sin bloquear la CPU.

| Periférico / Componente | Función / Señal | Pin ESP32 (GPIO) | Tipo de Señal | Descripción |
| :--- | :--- | :---: | :---: | :--- |
| **Bus I2C Compartido** | SDA (Datos) | `GPIO 21` | Digital (I2C) | Línea de datos compartida (OLED, RTC, EEPROM, Sensor) |
| | SCL (Reloj) | `GPIO 22` | Digital (I2C) | Línea de reloj del bus maestro configurada a 400 kHz |
| **Encoder Rotativo** | CLK | `GPIO 18` | Entrada Digital | Monitoreado por Polling continuo para detectar flancos de giro |
| | DT | `GPIO 19` | Entrada Digital | Evaluado en conjunto con CLK para determinar el sentido de giro |
| | SW (Botón) | `GPIO 23` | Entrada (Pull-Up) | Pulsador central con debounce por software para selección de menús |
| **Alarma Acústica** | Buzzer | `GPIO 25` | Salida Digital | Actuador piezoeléctrico para alertas de exceso lumínico |

---

## 📁 Estructura del Proyecto

El desarrollo está organizado bajo el estándar de arquitectura modular de **PlatformIO**, separando la lógica de control principal de las librerías nativas de bajo nivel de cada periférico:

```text
AgroLumenIOT/
├── lib/                               # Librerías modulares del sistema
│   ├── AgroLumen_Input/               # Control y debounce por software del encoder
│   │   ├── AgroLumen_Input.cpp
│   │   └── AgroLumen_Input.h
│   ├── AgroLumen_OLED/                # Layouts gráficos y lógica de scroll del display
│   │   ├── AgroLumen_OLED.cpp
│   │   └── AgroLumen_OLED.h
│   ├── AgroLumen_Sensor/              # Integración de PPFD, cálculo de DLI y algoritmo AGC
│   │   ├── AgroLumen_Sensor.cpp
│   │   └── AgroLumen_Sensor.h
│   └── AgroLumen_Storage/             # Drivers I2C, control del RTC DS3231 y corrimiento de EEPROM
│       ├── AgroLumen_Storage.cpp
│       └── AgroLumen_Storage.h
├── src/                               # Núcleo de la aplicación
│   ├── Config.h                       # Definición global de pines (Pinout) y constantes de tiempos
│   └── main.cpp                       # Inicialización y Máquina de Estados Finitos (FSM)
├── platformio.ini                     # Configuración del entorno de compilación y dependencias
└── README.md                          # Documentación técnica del proyecto

```
---

## 💾 Estructura y Distribución de la Memoria EEPROM

Para garantizar la durabilidad del hardware y la persistencia de datos ante fallas de alimentación, la memoria del historial se organiza en bloques contiguos fijos a partir de una dirección base (`ADDR_HISTORY_START`).

### Formato de Registro de Datos (`AgroRegistroDLI` - 8 Bytes por día)
Cada jornada ocupa un bloque exacto de **8 bytes** empaquetado de la siguiente manera:

* **Bytes 0-3 (`4 bytes`):** Tipo `float` que almacena el DLI acumulado del día.
* **Byte 4 (`1 byte`):** Tipo `uint8_t` para el Día del mes ($1-31$).
* **Byte 5 (`1 byte`):** Tipo `uint8_t` para el Mes ($1-12$).
* **Bytes 6-7 (`2 bytes`):** Tipo `uint16_t` para el Año (ej: $2026$).

El búfer lineal almacena de forma cíclica los últimos 15 días ($15 \text{ registros} \times 8 \text{ bytes} = 120 \text{ bytes}$ ocupados en el chip AT24C32).

---

## ⚙️ Arquitectura del Firmware y Lógica de Funcionamiento

El código se ejecuta bajo un esquema de **Máquina de Estados Finitos (FSM)** asistida por temporizadores basados en `millis()`, garantizando que ninguna tarea tape el flujo cooperativo del programa.

### 1. El Centinela de Medianoche (Transición de Jornada)
La función cooperativa `verificarCambioDeDia()` corre libremente en la raíz del `loop()`, asegurando el control temporal las 24 horas del día sin importar el estado de la pantalla.
* **Detección:** Evalúa si el día del RTC cambió respecto al `ultimoDiaArchivado`.
* **Rotación por Hardware (Shift):** Realiza un desplazamiento hacia atrás directamente en la EEPROM (leyendo desde el índice 13 hacia el 0 y escribiendo en la posición inmediata posterior) para liberar el casillero inicial.
* **Archivado:** Captura el DLI consolidado bajo el AGC, lo estampa con la fecha correspondiente en el índice `0` (Ayer), reinicia el integrador lumínico del sensor y rehabilita la alarma diaria.

### 2. Visor Histórico con Scroll Inteligente
Al ingresar al estado `HISTORY_VIEW`, el firmware optimiza el bus I2C realizando **una única lectura en bloque** de los 15 registros hacia un búfer dinámico en la memoria RAM del ESP32 (`historialRAM`).
* **Navegación:** El encoder altera la variable `indiceScroll` limitándola entre $0$ y $11$.
* **Renderizado:** La pantalla OLED dibuja dinámicamente un bloque de 4 días consecutivos con indicadores triangulares laterales de scroll (flechas arriba/abajo). El sistema valida la integridad temporal de los datos antes de graficarlos en pantalla.

---

## 💻 Entorno de Desarrollo

El firmware está optimizado para su compilación y despliegue bajo el siguiente ecosistema técnico:
* **IDE:** VS Code + PlatformIO.
* **Plataforma de Hardware:** `espressif32` (placa de desarrollo `esp32dev`).
* **Framework:** Arduino Core para ESP32.
* **Librerías Core:** `Wire`, `Adafruit_GFX`, `Adafruit_SSD1306`.

---

## 📄 Licencia

Este proyecto está bajo la Licencia MIT. Esto significa que sos libre de usar, modificar, copiar y distribuir el software para fines educativos, personales o comerciales, siempre y cuando se mantenga el aviso de derechos de autor original.
Desarrollado de forma soberana y colaborativa por José Faginas Simil (2026).