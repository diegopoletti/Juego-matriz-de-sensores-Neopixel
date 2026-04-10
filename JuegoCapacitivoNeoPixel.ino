/**
 * @file JuegoCapacitivoNeoPixel.ino
 * @brief Juego interactivo con matriz de sensores capacitivos y LEDs NeoPixel WS2812B
 *        sobre plataforma ESP32, con servidor web para histórico de partidas.
 *
 * @details
 *  - Matriz de 5 filas × 6 columnas de sensores capacitivos TTP223.
 *  - Las columnas se energizan mediante el multiplexor analógico CD4051 (3 pines GPIO).
 *  - Las filas se leen en 5 pines GPIO de entrada del ESP32.
 *  - Bajo cada celda: una tira de 16 LEDs WS2812B, todas encadenadas en un único pin DIN.
 *  - El juego consiste en presionar celdas verdes antes de ser alcanzado por la barra roja.
 *  - El Wi-Fi opera como punto de acceso (AP) no bloqueante.
 *  - Las estadísticas se almacenan en SPIFFS y se sirven vía HTTP en formato CSV o HTML.
 *  - El generador de números aleatorios usa el periférico hardware del ESP32 (esp_random()).
 *
 * @hardware
 *  - ESP32 DevKit v1 (o similar)
 *  - Multiplexor CD4051 (8 canales)
 *  - 30× sensor capacitivo TTP223
 *  - 30× tira de 16 LEDs WS2812B (480 LEDs en total)
 *  - Resistor 470 Ω en la línea DIN del primer LED
 *  - Capacitor electrolítico 1000 µF entre VCC-GND de los NeoPixel
 *  - Fuente 5 V / mínimo 10 A para los NeoPixel (limitar brillo al 30% ≈ 8.6 A)
 *  - 2× pulsador normalmente abierto con resistencia pull-down 10 kΩ
 *
 * @dependencies
 *  - Adafruit NeoPixel library (v1.11+)
 *  - ESP32 Arduino core (v2.x+) con soporte SPIFFS
 *  - WebServer.h (incluido en el core ESP32)
 *
 * @author  Proyecto didáctico ESP32
 * @version 1.0.0
 * @date    2026
 *
 * @license MIT
 */

// ═══════════════════════════════════════════════════════════
//  LIBRERÍAS
// ═══════════════════════════════════════════════════════════
#include <Arduino.h>          // Núcleo de Arduino
#include <Adafruit_NeoPixel.h>// Manejo de LEDs WS2812B
#include <WiFi.h>             // Módulo Wi-Fi del ESP32
#include <WebServer.h>        // Servidor HTTP embebido
#include <SPIFFS.h>           // Sistema de archivos en flash
#include <time.h>             // Para marcas de tiempo

// ═══════════════════════════════════════════════════════════
//  CONSTANTES DE HARDWARE — asignación de pines GPIO
// ═══════════════════════════════════════════════════════════

// ── Multiplexor CD4051 (selección de columna activa) ──
#define PIN_MUX_A     4       ///< Bit 0 de la dirección del multiplexor
#define PIN_MUX_B     5       ///< Bit 1 de la dirección del multiplexor
#define PIN_MUX_C     18      ///< Bit 2 de la dirección del multiplexor

// ── Entradas de filas de sensores capacitivos ──
// Nota: GPIO34 y 35 son solo entrada en ESP32 (sin pull-up interno)
#define PIN_FILA_0    34      ///< Lectura fila 0 del sensor
#define PIN_FILA_1    35      ///< Lectura fila 1 del sensor
#define PIN_FILA_2    32      ///< Lectura fila 2 del sensor
#define PIN_FILA_3    33      ///< Lectura fila 3 del sensor
#define PIN_FILA_4    25      ///< Lectura fila 4 del sensor

// ── NeoPixel ──
#define PIN_NEOPIXEL  26      ///< DIN de la primera tira WS2812B (resistor 470Ω en serie)
#define NUM_LEDS_POR_CELDA 16 ///< LEDs por tira/celda
#define BRILLO_MAX    77      ///< Brillo máximo (30% de 255) para no superar corriente

// ── Pulsadores ──
#define PIN_BTN_INICIO 27     ///< Pulsador de inicio de partida (pull-down externo)
#define PIN_BTN_RESET  15     ///< Pulsador de reset (pull-down externo)
#define DEBOUNCE_MS   50      ///< Tiempo de debounce en milisegundos

// ── Dimensiones de la matriz ──
#define FILAS          5      ///< Número de filas de la matriz de sensores
#define COLS           6      ///< Número de columnas de la matriz de sensores
#define TOTAL_CELDAS  (FILAS * COLS) ///< Total de celdas = 30
#define TOTAL_LEDS    (TOTAL_CELDAS * NUM_LEDS_POR_CELDA) ///< Total LEDs = 480

// ═══════════════════════════════════════════════════════════
//  CONSTANTES DE LÓGICA DEL JUEGO
// ═══════════════════════════════════════════════════════════
#define PUNTOS_PARA_GANAR   10    ///< Puntos necesarios para ganar la partida
#define VELOCIDAD_INICIAL_MS 520  ///< Intervalo inicial del barrido rojo (ms)
#define VELOCIDAD_MINIMA_MS  150  ///< Intervalo mínimo (juego más difícil)
#define REDUCCION_POR_NIVEL   35  ///< ms que se restan por cada nivel ganado
#define PAUSA_CELDA_VERDE_MS 300  ///< Pausa antes de colocar nueva celda verde

// ── Archivo de histórico en SPIFFS ──
#define ARCHIVO_HISTORICO "/historico.csv" ///< Ruta del CSV en SPIFFS

// ═══════════════════════════════════════════════════════════
//  CREDENCIALES Wi-Fi (modo Access Point)
// ═══════════════════════════════════════════════════════════
const char* SSID_AP      = "JuegoNeoPixel";   ///< Nombre de la red Wi-Fi AP
const char* PASSWORD_AP  = "esp32game";        ///< Contraseña de la red (mín 8 caracteres)

// ═══════════════════════════════════════════════════════════
//  OBJETOS GLOBALES
// ═══════════════════════════════════════════════════════════

/** Objeto de la tira NeoPixel. Todos los LEDs encadenados en un solo pin. */
Adafruit_NeoPixel tira(TOTAL_LEDS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

/** Servidor HTTP en el puerto 80. */
WebServer servidor(80);

// ═══════════════════════════════════════════════════════════
//  ESTADO DEL JUEGO
// ═══════════════════════════════════════════════════════════

/** Enumeración de estados posibles del juego. */
enum EstadoJuego {
  ESTADO_INACTIVO,   ///< Esperando que el jugador presione Inicio
  ESTADO_JUGANDO,    ///< Partida en curso
  ESTADO_GANANDO,    ///< Animación de victoria (todo verde)
  ESTADO_PERDIENDO   ///< Animación de derrota (todo rojo parpadeante)
};

EstadoJuego estadoActual = ESTADO_INACTIVO; ///< Estado del juego en este momento

int    columnaRoja     = 0;    ///< Columna actualmente iluminada en rojo (0-5)
int    direccionBarrido = 1;   ///< Dirección del barrido: +1=izq→der, -1=der→izq
int    puntajeActual   = 0;    ///< Puntos acumulados en la partida actual
int    nivelActual     = 1;    ///< Nivel actual (1-10)
int    numeroPartidas  = 0;    ///< Contador total de partidas jugadas

/** Mapa de presión de cada sensor [fila][columna]. true=presionado. */
bool estadoSensor[FILAS][COLS];

/** Indica si una celda es objetivo verde [fila][columna]. */
bool celdaEsVerde[FILAS][COLS];

/** Tiempos del temporizador de barrido y otras tareas (en milisegundos). */
unsigned long tiempoUltimoBarrido  = 0;
unsigned long tiempoUltimoCelda    = 0;
unsigned long tiempoDebounceInicio = 0;
unsigned long tiempoDebounceReset  = 0;

/** Intervalo actual del barrido en ms (depende del nivel). */
unsigned long intervaloBarridoActual = VELOCIDAD_INICIAL_MS;

/** Estado anterior de los pulsadores para detectar flanco de subida. */
bool estadoAnteriorBtnInicio = LOW;
bool estadoAnteriorBtnReset  = LOW;

/** Array de pines de fila para acceso indexado. */
const uint8_t pinesFila[FILAS] = {
  PIN_FILA_0, PIN_FILA_1, PIN_FILA_2, PIN_FILA_3, PIN_FILA_4
};

/** Colores del indicador de progreso (barra de puntaje, 10 LEDs). */
const uint32_t coloresProgreso[PUNTOS_PARA_GANAR] = {
  // Verde → amarillo → naranja → azul → violeta (progresión visual)
  0x00FF44, 0x22FF22, 0x66FF00, 0xAAFF00, 0xFFFF00,
  0xFFAA00, 0xFF6600, 0x4488FF, 0x8844FF, 0xAA44FF
};

// ═══════════════════════════════════════════════════════════
//  DECLARACIONES ANTICIPADAS DE FUNCIONES
// ═══════════════════════════════════════════════════════════
void seleccionarColumna(int columna);
void leerFilas();
void renderizarLeds();
void colorearCelda(int fila, int columna, uint32_t color);
void apagarCelda(int fila, int columna);
void actualizarBarraProgreso();
void iniciarPartida();
void terminarPartida(bool gano);
void avanzarColumnaRoja();
void verificarPresiones();
void colocarCeldasVerdes();
bool hayColisionVerde(int fila, int columna);
uint32_t generarColorProgreso(int punto);
void animacionVictoria();
void animacionDerrota();
void configurarWifi();
void configurarServidor();
void manejarPaginaWeb();
void manejarDescargaCSV();
void manejarEstadoJSON();
void guardarPartidaEnSpiffs(bool gano);
String obtenerFechaHora();
void inicializarSpiffs();

// ═══════════════════════════════════════════════════════════
//  SETUP — ejecución una única vez al encender
// ═══════════════════════════════════════════════════════════

/**
 * @brief Inicialización del hardware y software al encender el ESP32.
 *
 * Configura pines GPIO, la tira NeoPixel, Wi-Fi AP, SPIFFS y el servidor web.
 * No bloquea la ejecución esperando conexión Wi-Fi externa.
 */
void setup() {
  // ── Puerto serie para depuración (115200 baudios) ──
  Serial.begin(115200);
  Serial.println(F("\n=== JuegoCapacitivoNeoPixel v1.0 ==="));

  // ── Configuración de pines del multiplexor (salidas) ──
  pinMode(PIN_MUX_A, OUTPUT);   // Bit A del multiplexor
  pinMode(PIN_MUX_B, OUTPUT);   // Bit B del multiplexor
  pinMode(PIN_MUX_C, OUTPUT);   // Bit C del multiplexor
  digitalWrite(PIN_MUX_A, LOW); // Inicializar en canal 0
  digitalWrite(PIN_MUX_B, LOW);
  digitalWrite(PIN_MUX_C, LOW);

  // ── Configuración de pines de filas (entradas, sin pull-up en GPIO 34/35) ──
  for (int f = 0; f < FILAS; f++) {
    // GPIO34 y 35 no tienen pull-up interno → resistencia externa pull-down 10kΩ
    if (pinesFila[f] == 34 || pinesFila[f] == 35) {
      pinMode(pinesFila[f], INPUT);         // Solo INPUT (sin pull-up)
    } else {
      pinMode(pinesFila[f], INPUT_PULLDOWN); // INPUT con pull-down interno
    }
  }

  // ── Configuración de pulsadores (entradas) ──
  // Pull-down externo de 10 kΩ en la PCB
  pinMode(PIN_BTN_INICIO, INPUT); // Inicio de partida
  pinMode(PIN_BTN_RESET,  INPUT); // Reset del juego

  // ── Inicialización de la tira NeoPixel ──
  tira.begin();                        // Iniciar comunicación WS2812B
  tira.setBrightness(BRILLO_MAX);      // Limitar brillo (30%) por consumo
  tira.clear();                        // Apagar todos los LEDs
  tira.show();                         // Enviar datos a la tira
  Serial.println(F("NeoPixel OK — 480 LEDs inicializados"));

  // ── Inicializar matrices de estado ──
  memset(estadoSensor, 0, sizeof(estadoSensor)); // Todos los sensores apagados
  memset(celdaEsVerde, 0, sizeof(celdaEsVerde)); // Sin celdas objetivo

  // ── Inicializar SPIFFS para histórico ──
  inicializarSpiffs();

  // ── Configurar Wi-Fi como Access Point (no bloqueante) ──
  configurarWifi();

  // ── Configurar rutas del servidor web ──
  configurarServidor();

  Serial.println(F("Sistema listo. Presiona BTN_INICIO para jugar."));
}

// ═══════════════════════════════════════════════════════════
//  LOOP — ejecución continua
// ═══════════════════════════════════════════════════════════

/**
 * @brief Bucle principal no bloqueante del ESP32.
 *
 * Atiende el servidor web, lee sensores, actualiza el estado del juego
 * y renderiza los LEDs, todo sin usar delay() para evitar bloqueos.
 */
void loop() {
  // ── Atender peticiones HTTP del servidor web (no bloqueante) ──
  servidor.handleClient();

  // ── Leer pulsadores físicos con debounce ──
  manejarPulsadores();

  // ── Lógica según estado del juego ──
  switch (estadoActual) {

    case ESTADO_INACTIVO:
      // No hacer nada; esperar presión de BTN_INICIO
      break;

    case ESTADO_JUGANDO:
      leerFilas();              // Escanear sensores capacitivos (barrido de columnas)
      verificarPresiones();     // Evaluar acciones del jugador
      avanzarColumnaRoja();     // Mover la barra roja según el temporizador
      renderizarLeds();         // Actualizar todos los LEDs
      break;

    case ESTADO_GANANDO:
      animacionVictoria();      // Efecto visual de victoria (bloqueante breve)
      estadoActual = ESTADO_INACTIVO; // Volver a esperar
      break;

    case ESTADO_PERDIENDO:
      animacionDerrota();       // Efecto visual de derrota (bloqueante breve)
      estadoActual = ESTADO_INACTIVO; // Volver a esperar
      break;
  }
}

// ═══════════════════════════════════════════════════════════
//  MANEJO DE PULSADORES
// ═══════════════════════════════════════════════════════════

/**
 * @brief Lee los pulsadores físicos con debounce por software.
 *
 * Detecta el flanco de subida (LOW→HIGH) para disparar acciones
 * sin registrar múltiples pulsaciones por rebote.
 */
void manejarPulsadores() {
  unsigned long ahora = millis(); // Tiempo actual en milisegundos

  // ── Pulsador INICIO ──
  bool estadoBtnInicio = digitalRead(PIN_BTN_INICIO); // Leer estado del pin
  if (estadoBtnInicio == HIGH && estadoAnteriorBtnInicio == LOW) {
    // Flanco de subida detectado → verificar debounce
    if (ahora - tiempoDebounceInicio > DEBOUNCE_MS) {
      tiempoDebounceInicio = ahora;            // Registrar tiempo del flanco
      if (estadoActual == ESTADO_INACTIVO) {   // Solo iniciar si está inactivo
        iniciarPartida();                       // Arrancar nueva partida
      }
    }
  }
  estadoAnteriorBtnInicio = estadoBtnInicio; // Guardar estado anterior

  // ── Pulsador RESET ──
  bool estadoBtnReset = digitalRead(PIN_BTN_RESET); // Leer estado del pin
  if (estadoBtnReset == HIGH && estadoAnteriorBtnReset == LOW) {
    // Flanco de subida detectado → verificar debounce
    if (ahora - tiempoDebounceReset > DEBOUNCE_MS) {
      tiempoDebounceReset = ahora;             // Registrar tiempo del flanco
      resetearSistema();                        // Reset completo del juego
    }
  }
  estadoAnteriorBtnReset = estadoBtnReset;   // Guardar estado anterior
}

// ═══════════════════════════════════════════════════════════
//  GESTIÓN DEL MULTIPLEXOR — selección de columna
// ═══════════════════════════════════════════════════════════

/**
 * @brief Selecciona la columna activa del multiplexor CD4051.
 *
 * El CD4051 usa 3 bits de dirección (A, B, C) para activar
 * uno de sus 8 canales (Y0-Y7). Solo usamos Y0-Y5 (columnas 0-5).
 *
 * @param columna Número de columna a activar (0-5).
 */
void seleccionarColumna(int columna) {
  // Bit 0 (pin A): bit menos significativo de la columna
  digitalWrite(PIN_MUX_A, (columna >> 0) & 0x01);
  // Bit 1 (pin B): segundo bit de la columna
  digitalWrite(PIN_MUX_B, (columna >> 1) & 0x01);
  // Bit 2 (pin C): tercer bit de la columna
  digitalWrite(PIN_MUX_C, (columna >> 2) & 0x01);
  // Pequeña pausa para estabilizar la señal del multiplexor (~2 µs)
  delayMicroseconds(2);
}

// ═══════════════════════════════════════════════════════════
//  LECTURA DE SENSORES — barrido de columna activa
// ═══════════════════════════════════════════════════════════

/**
 * @brief Escanea todas las celdas de la columna activa y actualiza estadoSensor[][].
 *
 * Se activa la columna actual mediante el multiplexor, luego se leen
 * los 5 pines de fila para detectar presiones del jugador.
 * Este método se llama desde el loop(), es decir, se barre columna por columna
 * en cada iteración, no todas a la vez.
 *
 * @note El barrido completo de las 6 columnas ocurre en sucesivas llamadas.
 *       La variable estática 'columnaEscaneo' lleva el cursor.
 */
void leerFilas() {
  static int columnaEscaneo = 0; // Columna siendo escaneada en esta iteración

  seleccionarColumna(columnaEscaneo); // Activar columna en el multiplexor

  // Leer cada fila de la columna activa
  for (int f = 0; f < FILAS; f++) {
    // HIGH indica que el sensor fue tocado (TTP223 salida activa en HIGH)
    estadoSensor[f][columnaEscaneo] = (digitalRead(pinesFila[f]) == HIGH);
  }

  // Avanzar a la siguiente columna (cíclico 0-5)
  columnaEscaneo = (columnaEscaneo + 1) % COLS;
}

// ═══════════════════════════════════════════════════════════
//  LÓGICA DEL JUEGO — avance de la barra roja
// ═══════════════════════════════════════════════════════════

/**
 * @brief Avanza la columna roja según el temporizador no bloqueante.
 *
 * Incrementa o decrementa la posición de la columna roja en función
 * de la dirección del barrido. Al llegar a los extremos invierte la dirección.
 * El intervalo de avance depende del nivel actual.
 */
void avanzarColumnaRoja() {
  unsigned long ahora = millis(); // Tiempo actual

  // Verificar si ya pasó el intervalo configurado
  if (ahora - tiempoUltimoBarrido < intervaloBarridoActual) return;
  tiempoUltimoBarrido = ahora; // Actualizar referencia de tiempo

  columnaRoja += direccionBarrido; // Avanzar en la dirección actual

  // Verificar límites y rebotar
  if (columnaRoja >= COLS) {
    columnaRoja = COLS - 2;   // Retroceder uno para no salirse
    direccionBarrido = -1;     // Cambiar dirección: derecha → izquierda
  } else if (columnaRoja < 0) {
    columnaRoja = 1;           // Avanzar uno para no salirse
    direccionBarrido = +1;     // Cambiar dirección: izquierda → derecha
  }
}

// ═══════════════════════════════════════════════════════════
//  LÓGICA DEL JUEGO — verificar presiones del jugador
// ═══════════════════════════════════════════════════════════

/**
 * @brief Evalúa las presiones detectadas en la iteración actual.
 *
 * Para cada sensor presionado evalúa:
 *  1. Si está en la columna roja: el jugador pierde (fue alcanzado).
 *  2. Si es celda verde: el jugador suma un punto.
 *  3. Si no es verde ni roja: sin efecto (pisada neutra).
 *
 * La regla de salto: si el jugador no pisa (sensor no activo) cuando
 * la barra roja pasa por su columna, sobrevive (se detecta automáticamente
 * porque estadoSensor[][columnaRoja] estará en false).
 */
void verificarPresiones() {
  for (int f = 0; f < FILAS; f++) {
    for (int c = 0; c < COLS; c++) {

      // Solo evaluar sensores actualmente presionados
      if (!estadoSensor[f][c]) continue;

      // ── Caso 1: El jugador pisa la columna roja → DERROTA ──
      if (c == columnaRoja) {
        Serial.printf("💥 Derrota! Sensor [%d,%d] bajo la barra roja\n", f, c);
        terminarPartida(false); // false = perdió
        return;                 // Salir inmediatamente
      }

      // ── Caso 2: El jugador presiona una celda verde → PUNTO ──
      if (celdaEsVerde[f][c]) {
        celdaEsVerde[f][c] = false;        // Desactivar la celda verde
        puntajeActual++;                    // Sumar punto
        Serial.printf("✅ Punto! [%d,%d] — Puntaje: %d\n", f, c, puntajeActual);

        // Verificar si el jugador ganó
        if (puntajeActual >= PUNTOS_PARA_GANAR) {
          terminarPartida(true); // true = ganó
          return;
        }

        // Aumentar nivel de dificultad según puntos acumulados
        nivelActual = min(10, 1 + puntajeActual / 2);

        // Recalcular velocidad del barrido (más rápido a mayor nivel)
        intervaloBarridoActual = max(
          (long)VELOCIDAD_MINIMA_MS,
          (long)(VELOCIDAD_INICIAL_MS - (nivelActual - 1) * REDUCCION_POR_NIVEL)
        );

        Serial.printf("Nivel: %d — Intervalo: %lu ms\n", nivelActual, intervaloBarridoActual);

        // Programar aparición de nueva celda verde con pausa breve
        tiempoUltimoCelda = millis() - PAUSA_CELDA_VERDE_MS + 300; // Corto delay
      }
      // ── Caso 3: Pisada neutra (sin efecto) ──
    }
  }

  // Verificar si es momento de colocar nuevas celdas verdes
  if (millis() - tiempoUltimoCelda >= PAUSA_CELDA_VERDE_MS) {
    // Solo colocar si no hay celdas verdes activas
    bool hayVerde = false;
    for (int f = 0; f < FILAS && !hayVerde; f++)
      for (int c = 0; c < COLS && !hayVerde; c++)
        if (celdaEsVerde[f][c]) hayVerde = true;

    if (!hayVerde) {
      colocarCeldasVerdes(); // Colocar 1 o 2 celdas objetivo
      tiempoUltimoCelda = millis(); // Resetear temporizador
    }
  }
}

// ═══════════════════════════════════════════════════════════
//  LÓGICA DEL JUEGO — colocar celdas verdes al azar
// ═══════════════════════════════════════════════════════════

/**
 * @brief Coloca 1 o 2 celdas objetivo verdes en posiciones aleatorias.
 *
 * Usa esp_random() para obtener números verdaderamente aleatorios
 * del generador de ruido térmico del hardware del ESP32.
 * La celda verde no se ubica en la misma posición que la barra roja.
 */
void colocarCeldasVerdes() {
  // Determinar cantidad de celdas verdes (1 o 2) con 40% de probabilidad de 2
  int cantidad = ((esp_random() % 100) < 40) ? 2 : 1;

  int colocadas = 0;    // Contador de celdas ubicadas exitosamente
  int intentos  = 0;    // Evitar bucle infinito

  while (colocadas < cantidad && intentos < 50) {
    intentos++;
    // Generar posición aleatoria usando el RNG hardware del ESP32
    int f = esp_random() % FILAS; // Fila aleatoria (0-4)
    int c = esp_random() % COLS;  // Columna aleatoria (0-5)

    // Evitar colocar verde en la columna actualmente roja
    if (c == columnaRoja) continue;

    // Evitar colocar verde donde ya hay otra verde
    if (celdaEsVerde[f][c]) continue;

    celdaEsVerde[f][c] = true; // Marcar celda como objetivo
    colocadas++;
    Serial.printf("🎯 Celda verde en [%d,%d]\n", f, c);
  }
}

// ═══════════════════════════════════════════════════════════
//  RENDERIZADO DE LEDs
// ═══════════════════════════════════════════════════════════

/**
 * @brief Actualiza el estado visual de todos los LEDs NeoPixel.
 *
 * Mapea el estado lógico del juego a colores:
 *  - Columna roja  → color rojo puro
 *  - Celda verde   → color verde puro
 *  - Resto         → apagado
 * Los primeros 10 LEDs de la celda [0][0] muestran la barra de progreso.
 */
void renderizarLeds() {
  tira.clear(); // Apagar todos los LEDs antes de redibujar

  // ── Dibujar columna roja ──
  for (int f = 0; f < FILAS; f++) {
    colorearCelda(f, columnaRoja, tira.Color(255, 0, 0)); // Rojo puro
  }

  // ── Dibujar celdas verdes ──
  for (int f = 0; f < FILAS; f++) {
    for (int c = 0; c < COLS; c++) {
      if (celdaEsVerde[f][c] && c != columnaRoja) {
        // Verde puro (si no coincide con barra roja)
        colorearCelda(f, c, tira.Color(0, 255, 0));
      }
    }
  }

  // ── Dibujar barra de progreso en los primeros 10 LEDs de la celda [0][0] ──
  actualizarBarraProgreso();

  tira.show(); // Enviar todos los datos a los LEDs (operación física)
}

/**
 * @brief Colorea todos los LEDs de una celda de la matriz.
 *
 * Calcula el índice base del primer LED de la celda y aplica
 * el color especificado a los NUM_LEDS_POR_CELDA LEDs de esa celda.
 *
 * @param fila    Fila de la celda (0-4).
 * @param columna Columna de la celda (0-5).
 * @param color   Color en formato uint32_t (GRB para WS2812B).
 */
void colorearCelda(int fila, int columna, uint32_t color) {
  // Calcular índice del primer LED de la celda (orden: columna mayor, fila menor)
  int indiceCelda = fila * COLS + columna;          // Índice de la celda (0-29)
  int indiceLedBase = indiceCelda * NUM_LEDS_POR_CELDA; // Índice del primer LED

  // Colorear los 16 LEDs de la celda
  for (int led = 0; led < NUM_LEDS_POR_CELDA; led++) {
    tira.setPixelColor(indiceLedBase + led, color); // Asignar color al LED
  }
}

/**
 * @brief Apaga todos los LEDs de una celda específica.
 *
 * @param fila    Fila de la celda (0-4).
 * @param columna Columna de la celda (0-5).
 */
void apagarCelda(int fila, int columna) {
  colorearCelda(fila, columna, 0); // Color 0 = apagado
}

/**
 * @brief Actualiza la barra de progreso del puntaje en los primeros 10 LEDs.
 *
 * Los LEDs 0 a 9 de la cadena (celda [0][0], primeros 10 LEDs) muestran
 * cuántos puntos lleva el jugador con colores progresivos.
 * Los LEDs 10-15 de esa celda se apagan.
 */
void actualizarBarraProgreso() {
  for (int i = 0; i < PUNTOS_PARA_GANAR; i++) {
    // Si el punto fue obtenido → iluminar con color de progreso
    uint32_t color = (i < puntajeActual) ? coloresProgreso[i] : 0;
    tira.setPixelColor(i, color); // LEDs 0-9 son los primeros de la cadena
  }
  // Apagar los LEDs 10-15 de la primera celda
  for (int i = PUNTOS_PARA_GANAR; i < NUM_LEDS_POR_CELDA; i++) {
    tira.setPixelColor(i, 0); // Apagado
  }
}

// ═══════════════════════════════════════════════════════════
//  GESTIÓN DE PARTIDA
// ═══════════════════════════════════════════════════════════

/**
 * @brief Inicia una nueva partida reseteando el estado del juego.
 *
 * Reinicia variables, posiciona la barra roja en la columna 0,
 * coloca las primeras celdas verdes y activa el estado JUGANDO.
 */
void iniciarPartida() {
  Serial.println(F("▶ Iniciando nueva partida..."));

  // Resetear variables de estado
  puntajeActual          = 0;                   // Puntaje a cero
  nivelActual            = 1;                   // Nivel inicial
  columnaRoja            = 0;                   // Barra roja en columna 0
  direccionBarrido       = 1;                   // Dirección izquierda → derecha
  intervaloBarridoActual = VELOCIDAD_INICIAL_MS;// Velocidad inicial del barrido
  numeroPartidas++;                              // Incrementar contador de partidas

  // Limpiar celdas verdes
  memset(celdaEsVerde, 0, sizeof(celdaEsVerde));
  // Limpiar estado de sensores
  memset(estadoSensor,  0, sizeof(estadoSensor));

  // Inicializar temporizadores
  tiempoUltimoBarrido = millis(); // Sincronizar con el tiempo actual
  tiempoUltimoCelda   = 0;        // Forzar aparición inmediata de celdas verdes

  // Activar estado JUGANDO
  estadoActual = ESTADO_JUGANDO;

  // Colocar primeras celdas verdes
  colocarCeldasVerdes();

  Serial.printf("Partida #%d iniciada\n", numeroPartidas);
}

/**
 * @brief Finaliza la partida actual (victoria o derrota).
 *
 * Guarda las estadísticas en SPIFFS y transiciona al estado
 * de animación correspondiente.
 *
 * @param gano true si el jugador ganó (10 puntos), false si perdió.
 */
void terminarPartida(bool gano) {
  estadoActual = gano ? ESTADO_GANANDO : ESTADO_PERDIENDO; // Cambiar estado

  // Guardar resultado en el archivo CSV de SPIFFS
  guardarPartidaEnSpiffs(gano);

  Serial.printf("%s — Puntaje final: %d puntos\n", gano ? "🏆 VICTORIA" : "💀 DERROTA", puntajeActual);
}

/**
 * @brief Resetea completamente el sistema al estado inicial.
 *
 * Funciona tanto durante una partida como en estado inactivo.
 * Apaga todos los LEDs y espera la próxima pulsación de Inicio.
 */
void resetearSistema() {
  Serial.println(F("⟳ Sistema reseteado"));

  estadoActual           = ESTADO_INACTIVO; // Volver al estado de espera
  puntajeActual          = 0;               // Limpiar puntaje
  nivelActual            = 1;               // Volver al nivel 1
  columnaRoja            = 0;               // Barra roja al inicio
  direccionBarrido       = 1;               // Dirección inicial
  intervaloBarridoActual = VELOCIDAD_INICIAL_MS;

  // Limpiar matrices de estado
  memset(celdaEsVerde, 0, sizeof(celdaEsVerde));
  memset(estadoSensor,  0, sizeof(estadoSensor));

  // Apagar todos los LEDs
  tira.clear();
  tira.show();
}

// ═══════════════════════════════════════════════════════════
//  ANIMACIONES
// ═══════════════════════════════════════════════════════════

/**
 * @brief Animación de victoria: ilumina todo de verde progresivamente.
 *
 * Barre todas las celdas de verde con un pequeño retraso entre columnas
 * para crear un efecto visual. Luego parpadea 3 veces.
 */
void animacionVictoria() {
  Serial.println(F("🏆 Animación de victoria"));

  // Barrer todo de verde columna por columna
  for (int c = 0; c < COLS; c++) {
    for (int f = 0; f < FILAS; f++) {
      colorearCelda(f, c, tira.Color(0, 255, 0)); // Verde
    }
    tira.show();          // Mostrar progreso
    servidor.handleClient(); // Atender peticiones web mientras animamos
    delay(80);            // Pequeño retraso entre columnas
  }

  // Parpadear 3 veces
  for (int i = 0; i < 3; i++) {
    tira.clear(); tira.show(); delay(200);          // Apagar
    for (int f = 0; f < FILAS; f++)
      for (int c = 0; c < COLS; c++)
        colorearCelda(f, c, tira.Color(0, 255, 0)); // Verde
    tira.show(); delay(300);                         // Encender
  }

  delay(1000); // Pausa final antes de volver al menú
  tira.clear(); tira.show(); // Apagar
}

/**
 * @brief Animación de derrota: parpadeo rojo de toda la grilla.
 *
 * Ilumina todo de rojo y parpadea 4 veces para indicar al jugador que perdió.
 */
void animacionDerrota() {
  Serial.println(F("💀 Animación de derrota"));

  for (int rep = 0; rep < 4; rep++) {
    // Encender todo en rojo
    for (int f = 0; f < FILAS; f++)
      for (int c = 0; c < COLS; c++)
        colorearCelda(f, c, tira.Color(255, 0, 0)); // Rojo
    tira.show();
    servidor.handleClient(); // Atender peticiones web durante la pausa
    delay(250);

    // Apagar
    tira.clear(); tira.show();
    delay(180);
  }

  delay(800); // Pausa antes de volver al menú
}

// ═══════════════════════════════════════════════════════════
//  Wi-Fi — MODO ACCESS POINT (NO BLOQUEANTE)
// ═══════════════════════════════════════════════════════════

/**
 * @brief Configura el ESP32 como punto de acceso Wi-Fi (AP).
 *
 * No espera conexión de ningún cliente externo. El juego
 * funciona independientemente del estado de la red.
 * IP del AP: 192.168.4.1
 */
void configurarWifi() {
  Serial.print(F("Configurando AP Wi-Fi... "));

  // Iniciar en modo solo AP (sin conectarse a router externo)
  WiFi.mode(WIFI_AP);

  // Crear la red Wi-Fi con SSID y contraseña definidos
  bool ok = WiFi.softAP(SSID_AP, PASSWORD_AP);

  if (ok) {
    Serial.printf("OK — SSID: %s — IP: %s\n", SSID_AP,
                  WiFi.softAPIP().toString().c_str());
  } else {
    Serial.println(F("ERROR al crear AP"));
  }
}

// ═══════════════════════════════════════════════════════════
//  SERVIDOR WEB — rutas y handlers
// ═══════════════════════════════════════════════════════════

/**
 * @brief Registra las rutas HTTP del servidor web embebido.
 *
 * Rutas disponibles:
 *  - GET /        → Página HTML con histórico de partidas
 *  - GET /csv     → Descargar el archivo CSV de estadísticas
 *  - GET /estado  → JSON con estado actual del juego
 */
void configurarServidor() {
  servidor.on("/",       HTTP_GET, manejarPaginaWeb);    // Página principal
  servidor.on("/csv",    HTTP_GET, manejarDescargaCSV);  // Descarga CSV
  servidor.on("/estado", HTTP_GET, manejarEstadoJSON);   // Estado JSON
  servidor.begin(); // Iniciar el servidor (no bloqueante)
  Serial.println(F("Servidor web iniciado en http://192.168.4.1"));
}

/**
 * @brief Sirve la página HTML principal con el histórico de partidas.
 *
 * Lee el archivo CSV de SPIFFS y lo presenta en una tabla HTML
 * con estilo responsivo. También muestra el estado actual del juego.
 */
void manejarPaginaWeb() {
  // ── Construir la tabla HTML con datos de SPIFFS ──
  String tabla = "";
  if (SPIFFS.exists(ARCHIVO_HISTORICO)) {
    File archivo = SPIFFS.open(ARCHIVO_HISTORICO, "r"); // Abrir en lectura
    if (archivo) {
      bool encabezado = true;
      while (archivo.available()) {
        String linea = archivo.readStringUntil('\n'); // Leer línea CSV
        linea.trim();
        if (linea.isEmpty()) continue;
        String celdas[5]; int nc = 0;
        // Separar la línea por comas
        int ini = 0;
        for (int i = 0; i <= (int)linea.length() && nc < 5; i++) {
          if (i == (int)linea.length() || linea[i] == ',') {
            celdas[nc++] = linea.substring(ini, i);
            ini = i + 1;
          }
        }
        if (encabezado) {
          tabla += "<tr>";
          for (int i = 0; i < nc; i++) tabla += "<th>" + celdas[i] + "</th>";
          tabla += "</tr>";
          encabezado = false;
        } else {
          tabla += "<tr>";
          for (int i = 0; i < nc; i++) tabla += "<td>" + celdas[i] + "</td>";
          tabla += "</tr>";
        }
      }
      archivo.close(); // Cerrar el archivo
    }
  } else {
    tabla = "<tr><td colspan='5'>Sin partidas registradas aún.</td></tr>";
  }

  // ── Estado actual del juego en texto ──
  const char* estadoTexto[] = {"Inactivo", "Jugando", "Victoria", "Derrota"};

  // ── Construir HTML completo ──
  String html = F("<!DOCTYPE html><html lang='es'><head><meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>Juego NeoPixel ESP32</title>"
    "<style>body{font-family:sans-serif;margin:20px;background:#111;color:#eee}"
    "h1{color:#22c55e}table{width:100%;border-collapse:collapse;margin-top:16px}"
    "th{background:#1f2937;color:#86efac;padding:8px;text-align:left}"
    "td{padding:7px 8px;border-bottom:1px solid #374151}"
    "tr:hover td{background:#1f2937}.badge{padding:2px 8px;border-radius:4px;font-size:12px}"
    ".win{background:#14532d;color:#86efac}.lose{background:#7f1d1d;color:#fca5a5}"
    ".stat{display:inline-block;margin:0 16px 8px 0;background:#1f2937;"
    "padding:8px 16px;border-radius:6px}"
    "a{color:#60a5fa}button{background:#1f2937;color:#eee;border:1px solid #374151;"
    "padding:6px 14px;border-radius:6px;cursor:pointer;margin-top:12px}"
    "</style></head><body>"
    "<h1>&#127918; Juego Capacitivo NeoPixel</h1>"
    "<div><span class='stat'>Partidas jugadas: <b>");
  html += numeroPartidas;
  html += F("</b></span><span class='stat'>Estado: <b>");
  html += estadoTexto[estadoActual];
  html += F("</b></span><span class='stat'>Puntaje actual: <b>");
  html += puntajeActual;
  html += F(" / 10</b></span><span class='stat'>Nivel: <b>");
  html += nivelActual;
  html += F("</b></span></div>"
    "<a href='/csv'><button>&#128229; Descargar CSV</button></a>"
    "<h2>Hist&#243;rico de partidas</h2>"
    "<table><thead><tr><th>#</th><th>Fecha/Hora</th><th>Puntaje</th><th>Resultado</th>"
    "<th>Nivel alcanzado</th></tr></thead><tbody>");
  html += tabla;
  html += F("</tbody></table>"
    "<p style='color:#6b7280;font-size:12px;margin-top:20px'>"
    "Actualizar: <a href='/'>F5</a> &mdash; ESP32 NeoPixel Game &mdash; SPIFFS v1.0</p>"
    "</body></html>");

  // Enviar respuesta HTTP
  servidor.send(200, "text/html; charset=UTF-8", html);
}

/**
 * @brief Sirve el archivo CSV de estadísticas para descargar.
 *
 * Agrega el header Content-Disposition para que el navegador lo descargue.
 */
void manejarDescargaCSV() {
  if (!SPIFFS.exists(ARCHIVO_HISTORICO)) {
    // Si no existe el archivo, devolver CSV vacío con encabezado
    servidor.send(200, "text/csv; charset=UTF-8",
                  "Partida,FechaHora,Puntaje,Resultado,NivelAlcanzado\n");
    return;
  }

  File archivo = SPIFFS.open(ARCHIVO_HISTORICO, "r"); // Abrir en lectura
  if (!archivo) {
    servidor.send(500, "text/plain", "Error al abrir el archivo");
    return;
  }

  // Header para forzar descarga en el navegador
  servidor.sendHeader("Content-Disposition",
                       "attachment; filename=\"historico_juego.csv\"");
  servidor.streamFile(archivo, "text/csv; charset=UTF-8"); // Enviar stream
  archivo.close(); // Cerrar el archivo
}

/**
 * @brief Sirve el estado actual del juego en formato JSON.
 *
 * Útil para integración con aplicaciones externas o monitoreo remoto.
 * Responde con: estado, puntaje, nivel, columnaRoja, numeroPartidas.
 */
void manejarEstadoJSON() {
  const char* estados[] = {"inactivo", "jugando", "victoria", "derrota"};

  String json = "{";
  json += "\"estado\":\"" + String(estados[estadoActual]) + "\",";
  json += "\"puntaje\":" + String(puntajeActual) + ",";
  json += "\"nivel\":" + String(nivelActual) + ",";
  json += "\"columnaRoja\":" + String(columnaRoja) + ",";
  json += "\"partidas\":" + String(numeroPartidas) + ",";
  json += "\"intervaloMs\":" + String(intervaloBarridoActual);
  json += "}";

  // Permitir acceso desde cualquier origen (CORS)
  servidor.sendHeader("Access-Control-Allow-Origin", "*");
  servidor.send(200, "application/json", json);
}

// ═══════════════════════════════════════════════════════════
//  SPIFFS — Persistencia de estadísticas
// ═══════════════════════════════════════════════════════════

/**
 * @brief Inicializa el sistema de archivos SPIFFS.
 *
 * Si el montaje falla, intenta formatear y volver a montar.
 * Crea el encabezado del CSV si el archivo no existe.
 */
void inicializarSpiffs() {
  Serial.print(F("Inicializando SPIFFS... "));

  if (!SPIFFS.begin(true)) { // true = formatear si hay error
    Serial.println(F("ERROR al montar SPIFFS"));
    return;
  }
  Serial.println(F("OK"));

  // Crear el archivo CSV con encabezado si no existe
  if (!SPIFFS.exists(ARCHIVO_HISTORICO)) {
    File f = SPIFFS.open(ARCHIVO_HISTORICO, "w"); // Crear en escritura
    if (f) {
      f.println("Partida,FechaHora,Puntaje,Resultado,NivelAlcanzado"); // Encabezado
      f.close(); // Cerrar el archivo
      Serial.println(F("Archivo CSV creado con encabezado"));
    }
  }
}

/**
 * @brief Guarda el resultado de la partida finalizada en el CSV de SPIFFS.
 *
 * Agrega una línea al final del archivo con los datos de la partida:
 * número, fecha/hora, puntaje, resultado y nivel máximo alcanzado.
 *
 * @param gano true si el jugador ganó, false si perdió.
 */
void guardarPartidaEnSpiffs(bool gano) {
  File archivo = SPIFFS.open(ARCHIVO_HISTORICO, "a"); // Abrir en modo append
  if (!archivo) {
    Serial.println(F("ERROR al abrir CSV para escritura"));
    return;
  }

  // Construir línea CSV
  String linea = String(numeroPartidas) + ",";  // Número de partida
  linea += obtenerFechaHora() + ",";             // Fecha y hora (si hay NTP) o millis
  linea += String(puntajeActual) + ",";          // Puntaje obtenido
  linea += gano ? "Victoria" : "Derrota";        // Resultado
  linea += "," + String(nivelActual);            // Nivel alcanzado

  archivo.println(linea); // Escribir la línea en el archivo
  archivo.close();        // Cerrar el archivo para garantizar el flush

  Serial.printf("Partida guardada en SPIFFS: %s\n", linea.c_str());
}

/**
 * @brief Retorna la fecha/hora actual como cadena de texto.
 *
 * Como el ESP32 en modo AP no tiene acceso a NTP, se usa
 * el tiempo de funcionamiento (millis) como referencia relativa.
 * Si se integra NTP, aquí se puede retornar la hora real.
 *
 * @return String con el tiempo en formato "T+Xmin Ys" (tiempo desde encendido).
 */
String obtenerFechaHora() {
  unsigned long seg  = millis() / 1000;          // Segundos desde encendido
  unsigned long min  = seg / 60;                 // Minutos
  unsigned long segg = seg % 60;                 // Segundos restantes
  char buf[32];
  snprintf(buf, sizeof(buf), "T+%02lum%02lus", min, segg); // Formato T+MMmSSs
  return String(buf);
}

// ═══════════════════════════════════════════════════════════
//  FIN DEL CÓDIGO
// ═══════════════════════════════════════════════════════════
/*
 * TABLA DE ASIGNACIÓN DE PINES — Resumen rápido
 * ┌─────────────────────┬───────────┬────────────────────────────────┐
 * │ Función             │ GPIO      │ Notas                          │
 * ├─────────────────────┼───────────┼────────────────────────────────┤
 * │ MUX A (bit 0)       │ GPIO 4    │ Salida digital                 │
 * │ MUX B (bit 1)       │ GPIO 5    │ Salida digital                 │
 * │ MUX C (bit 2)       │ GPIO 18   │ Salida digital                 │
 * │ Fila 0              │ GPIO 34   │ Entrada, sin pull-up interno   │
 * │ Fila 1              │ GPIO 35   │ Entrada, sin pull-up interno   │
 * │ Fila 2              │ GPIO 32   │ Entrada con pull-down interno  │
 * │ Fila 3              │ GPIO 33   │ Entrada con pull-down interno  │
 * │ Fila 4              │ GPIO 25   │ Entrada con pull-down interno  │
 * │ NeoPixel DIN        │ GPIO 26   │ Resistor 470Ω en serie         │
 * │ Pulsador INICIO     │ GPIO 27   │ Pull-down 10kΩ externo         │
 * │ Pulsador RESET      │ GPIO 15   │ Pull-down 10kΩ externo         │
 * └─────────────────────┴───────────┴────────────────────────────────┘
 *
 * LIBRERÍAS REQUERIDAS (instalar en Arduino IDE / PlatformIO):
 *  - Adafruit NeoPixel by Adafruit (v1.11.0 o superior)
 *  - ESP32 Arduino core (v2.x) — incluye WebServer.h, SPIFFS.h, WiFi.h
 *
 * PARTICIÓN FLASH recomendada:
 *  - "Default 4MB with spiffs" en Arduino IDE → Herramientas → Partition Scheme
 *    Esto reserva 1.5 MB para SPIFFS donde se almacena el CSV.
 */
