/**
 * @file JuegoCapacitivoNeoPixel_8col.ino
 *
 * @brief Juego interactivo con sensores capacitivos y tiras de LEDs de colores
 *        (NeoPixel WS2812B) controlado por un ESP32.
 *
 * ---------------------------------------------------------------------------
 * ¿QUÉ HACE ESTE PROGRAMA?
 * ---------------------------------------------------------------------------
 *  Una barra roja se mueve de izquierda a derecha y de derecha a izquierda
 *  sobre una grilla de 5 filas × 8 columnas de sensores capacitivos.
 *  Debajo de cada sensor hay una tira de 20 LEDs WS2812B.
 *
 *  El jugador tiene que pisar la celda verde sin que la barra roja
 *  esté pasando por ahí. Si lo logra, suma un punto. Con 10 puntos, gana.
 *  Si la barra roja lo pilla pisando, pierde.
 *
 * ---------------------------------------------------------------------------
 * HARDWARE NECESARIO
 * ---------------------------------------------------------------------------
 *  - 1× ESP32 DevKit v1 (o compatible)
 *  - 1× Multiplexor analógico CD4051 (8 canales)
 *  - 40× Sensor capacitivo TTP223 (5 filas × 8 columnas)
 *  - 40× Tira de 20 LEDs WS2812B = 800 LEDs en total encadenados
 *  - 1× Resistor de 470 Ω en la línea de datos del primer LED
 *  - 1× Capacitor electrolítico de 1000 µF entre VCC y GND de los LEDs
 *  - 1× Fuente de alimentación de 5 V con al menos 15 A de capacidad
 *    (brillo limitado al 30% → consumo ≈ 14.4 A máximo)
 *  - 2× Pulsador normalmente abierto con resistencia pull-down de 10 kΩ
 *
 * ---------------------------------------------------------------------------
 * LIBRERÍAS QUE HAY QUE INSTALAR EN ARDUINO IDE
 * ---------------------------------------------------------------------------
 *  - "Adafruit NeoPixel" by Adafruit  (versión 1.11 o superior)
 *  - El core de ESP32 para Arduino    (versión 2.x o superior)
 *    (incluye WiFi.h, WebServer.h y SPIFFS.h automáticamente)
 *
 * ---------------------------------------------------------------------------
 * PARTICIÓN DE LA MEMORIA FLASH
 * ---------------------------------------------------------------------------
 *  En Arduino IDE ir a:  Herramientas → Partition Scheme
 *  Elegir: "Default 4MB with spiffs"
 *  Esto reserva 1.5 MB para SPIFFS, donde se guarda el historial de partidas.
 *
 * @author  Matías Aldana - Diego Poletti - Jonathan Garrido
 * @version 2.1.0  (ampliado a 8 columnas, 20 LEDs por celda)
 * @date    17/04/2026
 * @license MIT
 */


// ═══════════════════════════════════════════════════════════════════════════
//  SECCIÓN 1 — LIBRERÍAS
//  Las librerías son archivos de código ya escritos que le agregan funciones
//  nuevas al programa. Se incluyen con la instrucción #include.
// ═══════════════════════════════════════════════════════════════════════════

#include <Arduino.h>           // Base de todo programa Arduino / ESP32
#include <Adafruit_NeoPixel.h> // Manejo de los LEDs WS2812B (NeoPixel)
#include <WiFi.h>              // Módulo de Wi-Fi del ESP32
#include <WebServer.h>         // Servidor web embebido en el ESP32
#include <SPIFFS.h>            // Sistema de archivos en la memoria flash


// ═══════════════════════════════════════════════════════════════════════════
//  SECCIÓN 2 — PINES GPIO
//  Un pin GPIO es una "pata" del ESP32 que puede enviar o recibir señales.
//  Con #define le ponemos un nombre fácil de recordar a cada número de pin.
// ═══════════════════════════════════════════════════════════════════════════

// ── Multiplexor CD4051 ──────────────────────────────────────────────────
//  El CD4051 es como un interruptor electrónico de 8 posiciones.
//  Con solo 3 pines del ESP32 (A, B, C) elegimos cuál de las 8 salidas
//  del multiplexor se activa, ahorrando pines del microcontrolador.
//
//  Las 3 señales A, B, C forman un número binario de 3 bits:
//    C B A = 0 0 0  →  canal Y0 activo  (columna 0)
//    C B A = 0 0 1  →  canal Y1 activo  (columna 1)
//    C B A = 0 1 0  →  canal Y2 activo  (columna 2)
//    C B A = 0 1 1  →  canal Y3 activo  (columna 3)
//    C B A = 1 0 0  →  canal Y4 activo  (columna 4)
//    C B A = 1 0 1  →  canal Y5 activo  (columna 5)
//    C B A = 1 1 0  →  canal Y6 activo  (columna 6)
//    C B A = 1 1 1  →  canal Y7 activo  (columna 7)

#define PIN_MUX_A   4    // Pin que controla el bit 0 (menos significativo) del multiplexor
#define PIN_MUX_B   5    // Pin que controla el bit 1 del multiplexor
#define PIN_MUX_C   18   // Pin que controla el bit 2 (más significativo) del multiplexor

// ── Pines de lectura de filas ───────────────────────────────────────────
//  Cada pin de fila está conectado a la salida de todos los sensores
//  de esa fila. Cuando el multiplexor activa una columna, si el jugador
//  toca ese sensor, el pin correspondiente a su fila lee HIGH (1).
//
//  IMPORTANTE: GPIO 34 y GPIO 35 solo funcionan como ENTRADA en el ESP32
//  y NO tienen resistencia pull-up interna, por eso necesitan pull-down externo.

#define PIN_FILA_0  34   // Fila 0 → solo entrada, pull-down de 10 kΩ externo
#define PIN_FILA_1  35   // Fila 1 → solo entrada, pull-down de 10 kΩ externo
#define PIN_FILA_2  32   // Fila 2 → entrada con pull-down interno del ESP32
#define PIN_FILA_3  33   // Fila 3 → entrada con pull-down interno del ESP32
#define PIN_FILA_4  25   // Fila 4 → entrada con pull-down interno del ESP32

// ── LED NeoPixel ────────────────────────────────────────────────────────
//  Todas las tiras de LEDs están encadenadas y conectadas a un solo pin.
//  El resistor de 470 Ω protege la línea de datos de picos de voltaje.

#define PIN_NEOPIXEL        26   // Pin de datos (DIN) del primer LED WS2812B
#define LEDS_POR_CELDA      20   // Cantidad de LEDs que hay debajo de cada sensor
#define BRILLO_MAXIMO       77   // Brillo = 30% de 255 para no quemar la fuente
                                 // (800 LEDs × 60 mA × 30% ≈ 14.4 A máximo)

// ── Pulsadores ──────────────────────────────────────────────────────────
//  Dos botones físicos: uno para iniciar la partida y otro para resetear.
//  Tienen resistencia pull-down externa de 10 kΩ, así que en reposo leen
//  LOW (0) y al presionarlos leen HIGH (1).

#define PIN_BTN_INICIO  27   // Botón que inicia una nueva partida
#define PIN_BTN_RESET   15   // Botón que reinicia el juego en cualquier momento
#define DEBOUNCE_MS     50   // Tiempo mínimo en ms para filtrar rebotes del botón
                             // (los botones mecánicos "rebotan" varias veces al presionar)


// ═══════════════════════════════════════════════════════════════════════════
//  SECCIÓN 3 — DIMENSIONES DE LA GRILLA
//  Acá definimos el tamaño de la grilla de sensores.
//  Ahora usamos las 8 columnas del multiplexor para aprovechar al máximo.
// ═══════════════════════════════════════════════════════════════════════════

#define FILAS          5               // Cantidad de filas de sensores
#define COLS           8               // Cantidad de columnas (todas las del CD4051)
#define TOTAL_CELDAS   (FILAS * COLS)  // 5 × 8 = 40 celdas en total
#define TOTAL_LEDS     (TOTAL_CELDAS * LEDS_POR_CELDA)  // 40 × 20 = 800 LEDs


// ═══════════════════════════════════════════════════════════════════════════
//  SECCIÓN 4 — CONSTANTES DE JUEGO
//  Son valores que no cambian durante el programa y configuran el juego.
// ═══════════════════════════════════════════════════════════════════════════

#define PUNTOS_PARA_GANAR     10    // Puntos necesarios para ganar la partida
#define VELOCIDAD_INICIAL_MS  520   // Tiempo en ms entre cada paso de la barra roja
                                    // Más alto = más lento = más fácil
#define VELOCIDAD_MINIMA_MS   120   // Tiempo mínimo posible (nivel máximo de dificultad)
#define REDUCCION_POR_NIVEL   40    // Cada punto ganado reduce el intervalo en 40 ms
                                    // haciendo que la barra roja se mueva más rápido
#define PAUSA_VERDE_MS        350   // Tiempo en ms que espera antes de colocar
                                    // una nueva celda verde luego de que el jugador la pisó

// ── Archivo donde se guarda el historial en la memoria flash ────────────
#define ARCHIVO_HISTORIAL  "/historial.csv"   // Ruta del archivo CSV en SPIFFS


// ═══════════════════════════════════════════════════════════════════════════
//  SECCIÓN 5 — DATOS DE LA RED Wi-Fi
//  El ESP32 crea su propia red Wi-Fi (modo Access Point).
//  El juego funciona aunque nadie se conecte a esa red.
// ═══════════════════════════════════════════════════════════════════════════

const char* NOMBRE_RED  = "JuegoNeoPixel";   // Nombre (SSID) de la red Wi-Fi
const char* CLAVE_RED   = "esp32game";        // Contraseña de la red (mínimo 8 caracteres)
                                              // IP del ESP32 en la red: 192.168.4.1


// ═══════════════════════════════════════════════════════════════════════════
//  SECCIÓN 6 — OBJETOS GLOBALES
//  Un "objeto" es como una caja que agrupa funciones y datos relacionados.
// ═══════════════════════════════════════════════════════════════════════════

// Objeto que maneja toda la tira de LEDs WS2812B.
// Parámetros: cantidad total de LEDs, pin de datos, tipo de LED.
Adafruit_NeoPixel tira(TOTAL_LEDS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// Objeto que maneja el servidor web en el puerto 80 (HTTP estándar).
WebServer servidor(80);


// ═══════════════════════════════════════════════════════════════════════════
//  SECCIÓN 7 — VARIABLES DE ESTADO DEL JUEGO
//  Las variables guardan información que cambia mientras el programa corre.
// ═══════════════════════════════════════════════════════════════════════════

// ── Enumeración de estados posibles del juego ───────────────────────────
//  Una enumeración es una lista de nombres con valores enteros internos.
//  Usarla en vez de números hace el código mucho más fácil de entender.

enum EstadoJuego {
  ESTADO_INACTIVO,   // El juego está esperando que se presione el botón Inicio
  ESTADO_JUGANDO,    // Hay una partida en curso
  ESTADO_VICTORIA,   // El jugador ganó, se está mostrando la animación verde
  ESTADO_DERROTA     // El jugador perdió, se está mostrando la animación roja
};

EstadoJuego estadoActual = ESTADO_INACTIVO;  // Empieza esperando al jugador

// ── Variables de la barra roja ───────────────────────────────────────────
int columnaRoja       = 0;   // Número de columna donde está la barra roja (0 a 7)
int direccionBarrido  = 1;   // +1 = se mueve hacia la derecha, -1 = hacia la izquierda

// ── Variables de puntaje y nivel ────────────────────────────────────────
int puntajeActual  = 0;   // Puntos que lleva el jugador en la partida actual
int nivelActual    = 1;   // Nivel de dificultad (1 a 10)
int totalPartidas  = 0;   // Cantidad total de partidas jugadas desde el encendido

// ── Matrices de estado de la grilla ─────────────────────────────────────
//  Una matriz es una tabla de filas y columnas guardada en la memoria.
//  bool significa que solo puede valer verdadero (true) o falso (false).

bool sensorPresionado[FILAS][COLS];  // true = ese sensor está siendo pisado ahora
bool celdaEsVerde[FILAS][COLS];      // true = esa celda es el objetivo verde del jugador

// ── Temporizadores no bloqueantes ───────────────────────────────────────
//  En vez de usar delay() que "congela" el ESP32, usamos millis() que
//  devuelve cuántos milisegundos pasaron desde que se encendió.
//  La idea es registrar cuándo ocurrió algo y compararlo con el tiempo actual.

unsigned long tiempoUltimoBarrido = 0;   // Momento del último movimiento de la barra roja
unsigned long tiempoUltimaVerde   = 0;   // Momento en que se colocó la última celda verde

// ── Velocidad del barrido ────────────────────────────────────────────────
unsigned long intervaloBarrido = VELOCIDAD_INICIAL_MS;  // Intervalo actual en ms

// ── Variables para el debounce de los botones ───────────────────────────
//  "Debounce" es el proceso de ignorar los rebotes mecánicos de un botón.
//  Guardamos el estado anterior del botón para detectar el momento exacto
//  en que pasa de suelto (LOW) a presionado (HIGH).

bool     estadoAnteriorInicio = LOW;    // Último estado leído del botón Inicio
bool     estadoAnteriorReset  = LOW;    // Último estado leído del botón Reset
unsigned long tiempoDebInicio = 0;      // Tiempo del último cambio detectado en Inicio
unsigned long tiempoDebReset  = 0;      // Tiempo del último cambio detectado en Reset

// ── Array con los pines de las filas ────────────────────────────────────
//  Guardar los pines en un array nos permite recorrerlos con un for,
//  evitando repetir código para cada fila.

const uint8_t pinesFila[FILAS] = {
  PIN_FILA_0,   // Fila 0
  PIN_FILA_1,   // Fila 1
  PIN_FILA_2,   // Fila 2
  PIN_FILA_3,   // Fila 3
  PIN_FILA_4    // Fila 4
};

// ── Colores de la barra de progreso ─────────────────────────────────────
//  Cada vez que el jugador suma un punto, uno de estos colores se enciende
//  en la barra de progreso. Van de verde a violeta pasando por amarillo y azul.
//  El formato es 0xRRGGBB (hexadecimal: rojo, verde, azul).

const uint32_t coloresProgreso[PUNTOS_PARA_GANAR] = {
  0x00FF44,   // Verde brillante       — punto 1
  0x33FF22,   // Verde lima            — punto 2
  0x88FF00,   // Verde amarillento     — punto 3
  0xCCFF00,   // Amarillo verdoso      — punto 4
  0xFFFF00,   // Amarillo puro         — punto 5
  0xFFAA00,   // Naranja               — punto 6
  0xFF5500,   // Naranja rojizo        — punto 7
  0x2266FF,   // Azul brillante        — punto 8
  0x6644FF,   // Violeta azulado       — punto 9
  0xBB33FF    // Violeta intenso       — punto 10
};


// ═══════════════════════════════════════════════════════════════════════════
//  SECCIÓN 8 — DECLARACIONES ANTICIPADAS DE FUNCIONES
//  Le avisamos al compilador qué funciones existen antes de definirlas.
//  Esto permite que las funciones se puedan llamar entre sí sin importar
//  el orden en que aparecen en el archivo.
// ═══════════════════════════════════════════════════════════════════════════

void seleccionarColumna(int columna);       // Activa una columna en el multiplexor
void escanearSensores();                    // Lee todos los sensores de la columna activa
void avanzarBarraRoja();                    // Mueve la barra roja un paso
void verificarAccionesJugador();            // Evalúa si el jugador pisó algo
void colocarCeldasVerdes();                 // Pone 1 o 2 celdas objetivo verdes al azar
void dibujarLeds();                         // Actualiza los colores de todos los LEDs
void colorearCelda(int f, int c, uint32_t color); // Pinta los 20 LEDs de una celda
void dibujarBarraProgreso();                // Muestra el puntaje en los primeros 10 LEDs
void iniciarPartida();                      // Prepara todo para una nueva partida
void terminarPartida(bool gano);            // Cierra la partida y guarda estadísticas
void resetearJuego();                       // Vuelve todo al estado inicial
void animacionVictoria();                   // Efecto visual al ganar
void animacionDerrota();                    // Efecto visual al perder
void manejarBotones();                      // Lee los botones con debounce
void configurarWifi();                      // Inicia el Access Point Wi-Fi
void configurarServidorWeb();               // Registra las páginas del servidor
void paginaPrincipal();                     // Muestra la página HTML con estadísticas
void descargarCSV();                        // Envía el archivo CSV al navegador
void estadoJSON();                          // Devuelve el estado del juego en formato JSON
void inicializarSPIFFS();                   // Monta el sistema de archivos flash
void guardarPartida(bool gano);             // Escribe una línea en el CSV
String tiempoTranscurrido();                // Devuelve el tiempo desde el encendido


// ═══════════════════════════════════════════════════════════════════════════
//  SECCIÓN 9 — setup()
//  Esta función se ejecuta UNA SOLA VEZ cuando el ESP32 se enciende o resetea.
//  Se usa para configurar el hardware y dejar todo listo antes del juego.
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Inicialización del sistema completo.
 *
 * Configura pines, LEDs, Wi-Fi, SPIFFS y el servidor web.
 * No bloquea el programa esperando ninguna conexión externa.
 */
void setup() {

  // Iniciamos la comunicación con la computadora por el puerto serie.
  // El número 115200 es la velocidad en baudios (bits por segundo).
  // Esto nos permite ver mensajes de depuración en el Monitor Serie de Arduino IDE.
  Serial.begin(115200);
  Serial.println();                                      // Línea en blanco para separar
  Serial.println(F("=== JuegoCapacitivoNeoPixel v2.0 ===")); // F() guarda el texto en flash
  Serial.println(F("Grilla: 5 filas x 8 columnas = 40 celdas"));
  Serial.println(F("Total de LEDs: 800"));

  // ── Configurar pines del multiplexor como SALIDAS ──────────────────────
  //  OUTPUT significa que el ESP32 va a enviar señales por esos pines.
  pinMode(PIN_MUX_A, OUTPUT);          // Pin A del multiplexor → salida
  pinMode(PIN_MUX_B, OUTPUT);          // Pin B del multiplexor → salida
  pinMode(PIN_MUX_C, OUTPUT);          // Pin C del multiplexor → salida
  digitalWrite(PIN_MUX_A, LOW);        // Iniciar en 0 para seleccionar columna 0
  digitalWrite(PIN_MUX_B, LOW);        // Iniciar en 0
  digitalWrite(PIN_MUX_C, LOW);        // Iniciar en 0

  // ── Configurar pines de las filas como ENTRADAS ────────────────────────
  //  INPUT significa que el ESP32 va a leer señales por esos pines.
  //  INPUT_PULLDOWN activa una resistencia interna que mantiene el pin en LOW
  //  cuando no hay señal, evitando lecturas erráticas.
  for (int f = 0; f < FILAS; f++) {
    // GPIO 34 y 35 no tienen resistencia pull-down interna,
    // así que se configuran solo como INPUT (usan la resistencia externa de 10 kΩ)
    if (pinesFila[f] == 34 || pinesFila[f] == 35) {
      pinMode(pinesFila[f], INPUT);            // Solo entrada, sin pull-down interno
    } else {
      pinMode(pinesFila[f], INPUT_PULLDOWN);   // Entrada con pull-down interno del ESP32
    }
  }

  // ── Configurar pines de los botones como ENTRADAS ─────────────────────
  //  Los botones tienen resistencia pull-down externa de 10 kΩ,
  //  por eso se configuran como INPUT simple (no INPUT_PULLDOWN).
  pinMode(PIN_BTN_INICIO, INPUT);   // Botón de inicio → entrada
  pinMode(PIN_BTN_RESET,  INPUT);   // Botón de reset  → entrada

  // ── Inicializar la tira de LEDs NeoPixel ──────────────────────────────
  tira.begin();                      // Prepara la comunicación con los LEDs
  tira.setBrightness(BRILLO_MAXIMO); // Limita el brillo al 30% para no quemar la fuente
  tira.clear();                      // Apaga todos los LEDs (pone todos en color negro)
  tira.show();                       // Envía la información a los LEDs físicamente
  Serial.println(F("NeoPixel: 800 LEDs inicializados correctamente"));

  // ── Limpiar las matrices de estado ────────────────────────────────────
  //  memset llena un bloque de memoria con un valor específico.
  //  Acá ponemos todo en 0 (false) para empezar con nada presionado ni verde.
  memset(sensorPresionado, 0, sizeof(sensorPresionado)); // Ningún sensor pisado
  memset(celdaEsVerde,     0, sizeof(celdaEsVerde));     // Ninguna celda verde

  // ── Inicializar el sistema de archivos SPIFFS ─────────────────────────
  inicializarSPIFFS();

  // ── Configurar el Wi-Fi en modo Access Point ──────────────────────────
  configurarWifi();

  // ── Registrar las páginas del servidor web ────────────────────────────
  configurarServidorWeb();

  // Mensaje final indicando que todo está listo
  Serial.println(F("Sistema listo. Presioná BTN_INICIO para jugar."));
  Serial.println(F("Conectate a la red 'JuegoNeoPixel' y entrá a http://192.168.4.1"));
}


// ═══════════════════════════════════════════════════════════════════════════
//  SECCIÓN 10 — loop()
//  Esta función se ejecuta UNA Y OTRA VEZ de manera continua mientras
//  el ESP32 está encendido. Es el "corazón" del programa.
//  Usamos una técnica llamada "máquina de estados" para organizar la lógica:
//  según el estado actual del juego, hacemos cosas distintas.
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Bucle principal no bloqueante.
 *
 * Maneja el servidor web, los botones y la lógica del juego
 * sin usar delay(), lo que permite responder a múltiples eventos
 * al mismo tiempo.
 */
void loop() {

  // Atender las peticiones que llegan al servidor web.
  // Esto revisa si algún teléfono o computadora pidió una página.
  // No bloquea el programa aunque nadie esté conectado.
  servidor.handleClient();

  // Leer los botones físicos con filtro de rebote (debounce)
  manejarBotones();

  // Decidir qué hacer según el estado actual del juego
  switch (estadoActual) {

    case ESTADO_INACTIVO:
      // No hacemos nada, esperamos que el jugador presione el botón Inicio.
      break;

    case ESTADO_JUGANDO:
      escanearSensores();              // Leer qué sensores están siendo pisados
      verificarAccionesJugador();      // Evaluar si ganó un punto, perdió o nada
      avanzarBarraRoja();              // Mover la barra roja si ya pasó el tiempo
      dibujarLeds();                   // Actualizar los colores de todos los LEDs
      break;

    case ESTADO_VICTORIA:
      animacionVictoria();             // Mostrar efecto visual de victoria
      estadoActual = ESTADO_INACTIVO;  // Volver a esperar al jugador
      break;

    case ESTADO_DERROTA:
      animacionDerrota();              // Mostrar efecto visual de derrota
      estadoActual = ESTADO_INACTIVO;  // Volver a esperar al jugador
      break;
  }
}


// ═══════════════════════════════════════════════════════════════════════════
//  SECCIÓN 11 — MANEJO DE BOTONES
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Lee los botones físicos con debounce por software.
 *
 * El debounce evita que un solo presionado del botón se registre
 * muchas veces por el rebote mecánico de los contactos internos.
 * La técnica consiste en detectar el flanco de subida (LOW→HIGH) y
 * luego esperar un tiempo mínimo antes de volver a registrar el botón.
 */
void manejarBotones() {

  unsigned long ahora = millis();   // Leer el tiempo actual en milisegundos

  // ── Botón INICIO ──────────────────────────────────────────────────────
  bool estadoInicio = digitalRead(PIN_BTN_INICIO);   // Leer si el botón está presionado

  // Detectar flanco de subida: el botón pasó de suelto (LOW) a presionado (HIGH)
  if (estadoInicio == HIGH && estadoAnteriorInicio == LOW) {

    // Verificar que pasó suficiente tiempo desde la última detección (debounce)
    if (ahora - tiempoDebInicio > DEBOUNCE_MS) {
      tiempoDebInicio = ahora;                        // Actualizar tiempo de referencia

      // Solo iniciar si el juego no está corriendo
      if (estadoActual == ESTADO_INACTIVO) {
        iniciarPartida();                              // ¡Arrancar una nueva partida!
      }
    }
  }
  estadoAnteriorInicio = estadoInicio;   // Guardar estado para la próxima comparación

  // ── Botón RESET ───────────────────────────────────────────────────────
  bool estadoReset = digitalRead(PIN_BTN_RESET);     // Leer si el botón está presionado

  // Detectar flanco de subida del botón Reset
  if (estadoReset == HIGH && estadoAnteriorReset == LOW) {

    // Verificar debounce
    if (ahora - tiempoDebReset > DEBOUNCE_MS) {
      tiempoDebReset = ahora;                         // Actualizar tiempo de referencia
      resetearJuego();                                // Reiniciar todo inmediatamente
    }
  }
  estadoAnteriorReset = estadoReset;     // Guardar estado para la próxima comparación
}


// ═══════════════════════════════════════════════════════════════════════════
//  SECCIÓN 12 — MULTIPLEXOR
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Selecciona qué columna activa el multiplexor CD4051.
 *
 * El CD4051 usa 3 líneas de control (A, B, C) que forman un número
 * binario de 3 bits. Ese número indica cuál de los 8 canales (Y0-Y7)
 * se conecta a la salida común, energizando así esa columna de sensores.
 *
 * Ejemplo: columna 5 → binario 101 → C=1, B=0, A=1
 *
 * @param columna Número de columna a activar (0 a 7).
 */
void seleccionarColumna(int columna) {

  // Extraer cada bit de la columna usando desplazamiento de bits y máscara
  // >> 0 desplaza 0 posiciones → toma el bit 0 (bit menos significativo)
  // & 0x01 enmascara para quedarse solo con el bit que nos interesa
  digitalWrite(PIN_MUX_A, (columna >> 0) & 0x01);   // Bit 0 → pin A del multiplexor
  digitalWrite(PIN_MUX_B, (columna >> 1) & 0x01);   // Bit 1 → pin B del multiplexor
  digitalWrite(PIN_MUX_C, (columna >> 2) & 0x01);   // Bit 2 → pin C del multiplexor

  // Esperar 2 microsegundos para que la señal del multiplexor se estabilice
  // antes de leer los sensores de esa columna
  delayMicroseconds(2);
}


// ═══════════════════════════════════════════════════════════════════════════
//  SECCIÓN 13 — LECTURA DE SENSORES
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Escanea los sensores de una columna por vez de manera rotativa.
 *
 * En lugar de leer las 8 columnas de un golpe (lo que tomaría tiempo),
 * leemos una columna por cada llamada a esta función y avanzamos a la
 * siguiente. Como loop() se ejecuta miles de veces por segundo, en pocos
 * milisegundos completamos el barrido de toda la grilla.
 *
 * Se usa una variable estática para recordar en qué columna estamos
 * entre una llamada y la siguiente.
 */
void escanearSensores() {

  // Variable estática: mantiene su valor entre llamadas sucesivas.
  // Es como una variable global pero que solo existe dentro de esta función.
  static int columnaEscaneo = 0;   // Columna que vamos a leer en esta llamada

  // Activar la columna correspondiente en el multiplexor
  seleccionarColumna(columnaEscaneo);

  // Leer las 5 filas de esa columna
  for (int f = 0; f < FILAS; f++) {
    // digitalRead devuelve HIGH (1) si el sensor detectó un toque, LOW (0) si no
    // El sensor TTP223 pone la salida en HIGH cuando alguien lo toca
    sensorPresionado[f][columnaEscaneo] = (digitalRead(pinesFila[f]) == HIGH);
  }

  // Avanzar a la siguiente columna (cuando llega a 8, vuelve a 0)
  columnaEscaneo = (columnaEscaneo + 1) % COLS;
}


// ═══════════════════════════════════════════════════════════════════════════
//  SECCIÓN 14 — MOVIMIENTO DE LA BARRA ROJA
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Mueve la barra roja una columna según el temporizador no bloqueante.
 *
 * En vez de usar delay() que detendría todo el programa, comparamos
 * el tiempo actual con el momento del último movimiento.
 * Cuando la diferencia supera el intervalo configurado, movemos la barra.
 *
 * La barra rebota en los extremos: cuando llega a la columna 7 va hacia
 * la izquierda y cuando llega a la columna 0 va hacia la derecha.
 */
void avanzarBarraRoja() {

  unsigned long ahora = millis();   // Tiempo actual en milisegundos

  // Verificar si todavía no pasó el tiempo suficiente para moverse
  if (ahora - tiempoUltimoBarrido < intervaloBarrido) return; // Salir sin hacer nada

  tiempoUltimoBarrido = ahora;   // Registrar el momento de este movimiento

  // Mover la barra en la dirección actual (+1 o -1)
  columnaRoja += direccionBarrido;

  // Verificar si llegó al borde derecho (columna 8, que no existe)
  if (columnaRoja >= COLS) {
    columnaRoja = COLS - 2;      // Retroceder para no salirse de la grilla
    direccionBarrido = -1;        // Cambiar dirección: ahora va hacia la izquierda
  }
  // Verificar si llegó al borde izquierdo (columna -1, que no existe)
  else if (columnaRoja < 0) {
    columnaRoja = 1;             // Avanzar para no salirse de la grilla
    direccionBarrido = +1;        // Cambiar dirección: ahora va hacia la derecha
  }
}


// ═══════════════════════════════════════════════════════════════════════════
//  SECCIÓN 15 — LÓGICA PRINCIPAL DEL JUEGO
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Evalúa las presiones del jugador y actualiza el estado del juego.
 *
 * Para cada sensor presionado verifica tres casos:
 *  1. ¿Está en la columna roja?  → El jugador fue alcanzado → DERROTA
 *  2. ¿Es una celda verde?       → El jugador la pisó bien  → PUNTO
 *  3. ¿Ninguna de las anteriores?→ Pisada sin efecto (neutro)
 *
 * También controla cuándo colocar nuevas celdas verdes.
 */
void verificarAccionesJugador() {

  // Recorrer toda la grilla buscando sensores presionados
  for (int f = 0; f < FILAS; f++) {
    for (int c = 0; c < COLS; c++) {

      // Si este sensor no está siendo pisado, pasar al siguiente
      if (!sensorPresionado[f][c]) continue;

      // ── Caso 1: El jugador pisó la columna donde está la barra roja ──
      if (c == columnaRoja) {
        Serial.printf("💥 Derrota: sensor [%d][%d] pisado bajo la barra roja\n", f, c);
        terminarPartida(false);   // false = el jugador perdió
        return;                   // Salir de la función inmediatamente
      }

      // ── Caso 2: El jugador pisó una celda verde ───────────────────────
      if (celdaEsVerde[f][c]) {
        celdaEsVerde[f][c] = false;   // Desactivar el objetivo (ya fue pisado)
        puntajeActual++;               // Sumar un punto al jugador
        Serial.printf("✅ Punto! Celda [%d][%d] — Puntaje: %d\n", f, c, puntajeActual);

        // Verificar si el jugador ya alcanzó los 10 puntos para ganar
        if (puntajeActual >= PUNTOS_PARA_GANAR) {
          terminarPartida(true);       // true = el jugador ganó
          return;                      // Salir de la función inmediatamente
        }

        // Subir el nivel de dificultad cada 2 puntos (máximo nivel 10)
        nivelActual = min(10, 1 + puntajeActual / 2);

        // Recalcular el intervalo del barrido: más puntos = más rápido
        // max() asegura que no bajemos del mínimo configurado
        intervaloBarrido = (unsigned long) max(
          (long) VELOCIDAD_MINIMA_MS,
          (long)(VELOCIDAD_INICIAL_MS - (long)(nivelActual - 1) * REDUCCION_POR_NIVEL)
        );
        Serial.printf("Nivel: %d — Velocidad: %lu ms\n", nivelActual, intervaloBarrido);

        // Marcar cuándo fue el último punto para controlar la pausa antes de la nueva celda
        tiempoUltimaVerde = millis();
      }
      // ── Caso 3: Pisada sin efecto (celda neutra) ──────────────────────
      // No hacemos nada, el jugador puede caminar libremente por celdas neutras
    }
  }

  // ── Colocar nueva celda verde si no hay ninguna activa ────────────────
  //  Revisamos si hay alguna celda verde en el tablero
  bool hayVerde = false;   // Asumir que no hay hasta encontrar una
  for (int f = 0; f < FILAS && !hayVerde; f++) {
    for (int c = 0; c < COLS && !hayVerde; c++) {
      if (celdaEsVerde[f][c]) hayVerde = true;   // Encontramos una celda verde
    }
  }

  // Si no hay ninguna verde y ya pasó la pausa configurada, colocar nueva(s)
  if (!hayVerde && millis() - tiempoUltimaVerde >= PAUSA_VERDE_MS) {
    colocarCeldasVerdes();           // Colocar 1 o 2 celdas objetivo al azar
    tiempoUltimaVerde = millis();    // Registrar el momento para el próximo control
  }
}

/**
 * @brief Coloca 1 o 2 celdas objetivo verdes en posiciones aleatorias.
 *
 * Usa esp_random() que genera números verdaderamente aleatorios aprovechando
 * el ruido térmico del hardware del ESP32. Esto es mucho mejor que random()
 * que usa una fórmula matemática y puede repetir patrones.
 *
 * La celda verde nunca se coloca en la misma columna que la barra roja.
 */
void colocarCeldasVerdes() {

  // Decidir si colocar 1 o 2 celdas verdes.
  // esp_random() devuelve un número enorme entre 0 y 4294967295.
  // Con % 100 lo reducimos a un número entre 0 y 99.
  // Si es menor que 40, colocamos 2 celdas; si no, 1 sola.
  int cantidad = ((esp_random() % 100) < 40) ? 2 : 1;

  int colocadas = 0;   // Contador de celdas verdes ya ubicadas
  int intentos  = 0;   // Contador de intentos para no quedar en un bucle infinito

  // Intentar colocar la cantidad de celdas verdes definida
  while (colocadas < cantidad && intentos < 60) {
    intentos++;   // Contar este intento

    // Elegir posición aleatoria dentro de la grilla
    int f = esp_random() % FILAS;   // Fila aleatoria entre 0 y 4
    int c = esp_random() % COLS;    // Columna aleatoria entre 0 y 7

    // No colocar verde en la columna donde está la barra roja (sería injusto)
    if (c == columnaRoja) continue;

    // No colocar verde donde ya hay otra celda verde
    if (celdaEsVerde[f][c]) continue;

    // ¡Todo bien! Colocar la celda verde aquí
    celdaEsVerde[f][c] = true;   // Marcar esta celda como objetivo
    colocadas++;                   // Contar la celda colocada
    Serial.printf("🎯 Nueva celda verde en [%d][%d]\n", f, c);
  }
}


// ═══════════════════════════════════════════════════════════════════════════
//  SECCIÓN 16 — RENDERIZADO DE LEDs
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Actualiza el color de todos los LEDs según el estado del juego.
 *
 * El flujo es:
 *  1. Apagar todos los LEDs (limpiar el buffer interno).
 *  2. Pintar en rojo la columna de la barra roja.
 *  3. Pintar en verde las celdas objetivo.
 *  4. Mostrar la barra de progreso del puntaje.
 *  5. Enviar los datos a los LEDs físicos (tira.show()).
 *
 * Nota: tira.show() es la única instrucción que realmente enciende los LEDs.
 * Las instrucciones anteriores solo modifican un buffer en la memoria del ESP32.
 */
void dibujarLeds() {

  tira.clear();   // Poner todos los LEDs en negro (apagados) en el buffer

  // ── Dibujar la columna roja ───────────────────────────────────────────
  for (int f = 0; f < FILAS; f++) {
    // Pintar los 20 LEDs de cada celda de la columna roja con color rojo puro
    colorearCelda(f, columnaRoja, tira.Color(255, 0, 0));   // Rojo: R=255, G=0, B=0
  }

  // ── Dibujar las celdas verdes ─────────────────────────────────────────
  for (int f = 0; f < FILAS; f++) {
    for (int c = 0; c < COLS; c++) {
      if (celdaEsVerde[f][c]) {
        // Si esta celda coincide con la columna roja, el rojo tiene prioridad visual
        // (el jugador ya sabe que la barra roja está ahí, es una advertencia)
        if (c != columnaRoja) {
          colorearCelda(f, c, tira.Color(0, 255, 0));   // Verde: R=0, G=255, B=0
        }
      }
    }
  }

  // ── Dibujar la barra de progreso del puntaje ──────────────────────────
  dibujarBarraProgreso();

  // ── Enviar los datos a los LEDs físicos ──────────────────────────────
  // Esta instrucción tarda un tiempo proporcional a la cantidad de LEDs.
  // Con 800 LEDs tarda aproximadamente 24 ms.
  tira.show();
}

/**
 * @brief Pinta los LEDs de una celda de la grilla con un color sólido.
 *
 * Calcula el índice del primer LED de la celda dentro de la cadena total
 * y aplica el color a los LEDS_POR_CELDA LEDs de esa celda.
 *
 * Las celdas están ordenadas de izquierda a derecha, de arriba hacia abajo:
 *   Celda [0][0] → LEDs 0-19
 *   Celda [0][1] → LEDs 20-39
 *   ...
 *   Celda [0][7] → LEDs 140-159
 *   Celda [1][0] → LEDs 160-179
 *   ...
 *   Celda [4][7] → LEDs 780-799
 *
 * @param f     Fila de la celda (0 a 4).
 * @param c     Columna de la celda (0 a 7).
 * @param color Color en formato uint32_t (usar tira.Color(R, G, B)).
 */
void colorearCelda(int f, int c, uint32_t color) {

  // Calcular en qué número de celda estamos dentro de la cadena de 40 celdas
  int numeroCelda = f * COLS + c;   // Ej: fila 2, columna 3 → celda nro 19

  // Calcular el índice del primer LED de esa celda
  int primerLed = numeroCelda * LEDS_POR_CELDA;   // Ej: celda 19 → LED 380

  // Asignar el color a cada uno de los 20 LEDs de la celda
  for (int led = 0; led < LEDS_POR_CELDA; led++) {
    tira.setPixelColor(primerLed + led, color);   // Asignar color al LED
  }
}

/**
 * @brief Muestra el puntaje del jugador en los primeros 10 LEDs de la cadena.
 *
 * Los LEDs 0 al 9 forman la barra de progreso.
 * Cada punto ganado enciende un LED con un color progresivo.
 * Los LEDs 10 al 19 se apagan para no interferir con la barra de progreso.
 *
 * Estos LEDs pertenecen a la celda [0][0], que comparte espacio
 * con el primer cuadro de la grilla.
 */
void dibujarBarraProgreso() {

  // Encender un LED por cada punto que tiene el jugador
  for (int i = 0; i < PUNTOS_PARA_GANAR; i++) {
    if (i < puntajeActual) {
      // El jugador ya ganó este punto → encender con el color de progreso
      tira.setPixelColor(i, coloresProgreso[i]);
    } else {
      // El jugador todavía no llegó a este punto → apagar el LED
      tira.setPixelColor(i, 0);   // 0 = negro = apagado
    }
  }

  // Apagar los LEDs 10 a 15 de la primera celda (los que no son de progreso)
  for (int i = PUNTOS_PARA_GANAR; i < LEDS_POR_CELDA; i++) {
    tira.setPixelColor(i, 0);   // Apagar
  }
}


// ═══════════════════════════════════════════════════════════════════════════
//  SECCIÓN 17 — GESTIÓN DE PARTIDAS
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Prepara todo para empezar una nueva partida desde cero.
 *
 * Reinicia el puntaje, el nivel, la barra roja, los temporizadores
 * y coloca las primeras celdas verdes para que el jugador empiece a jugar.
 */
void iniciarPartida() {

  Serial.println(F("▶ Iniciando nueva partida..."));

  // Reiniciar todas las variables del juego a sus valores iniciales
  puntajeActual     = 0;                    // Sin puntos
  nivelActual       = 1;                    // Nivel más fácil
  columnaRoja       = 0;                    // Barra roja en la primera columna
  direccionBarrido  = 1;                    // Empieza yendo hacia la derecha
  intervaloBarrido  = VELOCIDAD_INICIAL_MS; // Velocidad inicial (más lenta)
  totalPartidas++;                          // Sumar al contador de partidas

  // Limpiar los mapas de estado de la grilla
  memset(celdaEsVerde,     0, sizeof(celdaEsVerde));     // Sin celdas verdes
  memset(sensorPresionado, 0, sizeof(sensorPresionado)); // Sin sensores pisados

  // Sincronizar temporizadores con el tiempo actual
  tiempoUltimoBarrido = millis();   // La barra roja "recién se movió"
  tiempoUltimaVerde   = 0;          // Forzar que aparezca una celda verde de inmediato

  // Cambiar al estado de juego activo
  estadoActual = ESTADO_JUGANDO;

  // Colocar las primeras celdas verdes para que el jugador tenga un objetivo
  colocarCeldasVerdes();

  Serial.printf("Partida #%d en curso\n", totalPartidas);
}

/**
 * @brief Cierra la partida actual y activa la animación correspondiente.
 *
 * Guarda el resultado en el historial de SPIFFS y cambia el estado
 * del juego para mostrar la animación de victoria o derrota.
 *
 * @param gano true si el jugador llegó a 10 puntos (ganó),
 *             false si fue alcanzado por la barra roja (perdió).
 */
void terminarPartida(bool gano) {

  // Definir el nuevo estado según el resultado
  estadoActual = gano ? ESTADO_VICTORIA : ESTADO_DERROTA;

  // Guardar el resultado en el archivo CSV de la memoria flash
  guardarPartida(gano);

  // Mostrar resultado en el Monitor Serie
  Serial.printf("%s — Puntaje final: %d puntos — Nivel: %d\n",
    gano ? "🏆 VICTORIA" : "💀 DERROTA",
    puntajeActual,
    nivelActual
  );
}

/**
 * @brief Reinicia el juego completamente al estado inicial.
 *
 * Funciona en cualquier momento, incluso durante una partida.
 * Apaga todos los LEDs y espera que el jugador presione Inicio.
 */
void resetearJuego() {

  Serial.println(F("⟳ Sistema reseteado por el jugador"));

  // Volver al estado de espera
  estadoActual = ESTADO_INACTIVO;

  // Reiniciar todas las variables
  puntajeActual     = 0;
  nivelActual       = 1;
  columnaRoja       = 0;
  direccionBarrido  = 1;
  intervaloBarrido  = VELOCIDAD_INICIAL_MS;

  // Limpiar matrices
  memset(celdaEsVerde,     0, sizeof(celdaEsVerde));
  memset(sensorPresionado, 0, sizeof(sensorPresionado));

  // Apagar todos los LEDs inmediatamente
  tira.clear();   // Poner todo en negro en el buffer
  tira.show();    // Enviar el buffer a los LEDs físicos
}


// ═══════════════════════════════════════════════════════════════════════════
//  SECCIÓN 18 — ANIMACIONES
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Animación de victoria: barrido verde de izquierda a derecha.
 *
 * Enciende las columnas de verde una por una para crear el efecto
 * de "ola verde". Luego parpadea toda la grilla 3 veces.
 * Durante la animación se siguen atendiendo las peticiones web.
 */
void animacionVictoria() {

  Serial.println(F("🏆 Animación de victoria"));

  // Barrer columna por columna de izquierda a derecha
  for (int c = 0; c < COLS; c++) {
    for (int f = 0; f < FILAS; f++) {
      colorearCelda(f, c, tira.Color(0, 255, 0));   // Verde puro
    }
    tira.show();               // Mostrar el avance de la ola
    servidor.handleClient();   // Atender la web mientras esperamos
    delay(70);                 // Pequeña pausa para que se vea la ola
  }

  // Parpadear toda la grilla en verde 3 veces
  for (int rep = 0; rep < 3; rep++) {
    // Apagar todo
    tira.clear();
    tira.show();
    servidor.handleClient();
    delay(180);

    // Encender todo en verde
    for (int f = 0; f < FILAS; f++) {
      for (int c = 0; c < COLS; c++) {
        colorearCelda(f, c, tira.Color(0, 255, 0));   // Verde puro
      }
    }
    tira.show();
    servidor.handleClient();
    delay(280);
  }

  // Pausa final con todo verde antes de volver al menú
  servidor.handleClient();
  delay(900);

  // Apagar todo al terminar la animación
  tira.clear();
  tira.show();
}

/**
 * @brief Animación de derrota: parpadeo rojo de toda la grilla.
 *
 * Enciende toda la grilla en rojo y parpadea 4 veces para indicar
 * claramente que el jugador fue alcanzado por la barra roja.
 */
void animacionDerrota() {

  Serial.println(F("💀 Animación de derrota"));

  // Parpadear toda la grilla en rojo 4 veces
  for (int rep = 0; rep < 4; rep++) {
    // Encender todo en rojo
    for (int f = 0; f < FILAS; f++) {
      for (int c = 0; c < COLS; c++) {
        colorearCelda(f, c, tira.Color(255, 0, 0));   // Rojo puro
      }
    }
    tira.show();
    servidor.handleClient();   // Atender la web durante la pausa
    delay(220);

    // Apagar todo
    tira.clear();
    tira.show();
    servidor.handleClient();
    delay(160);
  }

  // Pausa final antes de volver al menú
  servidor.handleClient();
  delay(700);
}


// ═══════════════════════════════════════════════════════════════════════════
//  SECCIÓN 19 — Wi-Fi (MODO ACCESS POINT)
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Configura el ESP32 como punto de acceso Wi-Fi (Access Point).
 *
 * El ESP32 crea su propia red Wi-Fi a la que pueden conectarse
 * teléfonos o computadoras para ver el historial de partidas.
 * El juego funciona aunque nadie esté conectado (no bloqueante).
 *
 * Una vez conectado a la red "JuegoNeoPixel", entrar al navegador
 * y escribir: http://192.168.4.1
 */
void configurarWifi() {

  Serial.print(F("Configurando Access Point Wi-Fi... "));

  // Cambiar el modo del Wi-Fi a solo Access Point (no intenta conectarse a ningún router)
  WiFi.mode(WIFI_AP);

  // Crear la red Wi-Fi con el nombre y la contraseña definidos
  bool resultado = WiFi.softAP(NOMBRE_RED, CLAVE_RED);

  if (resultado) {
    // Éxito: mostrar la IP del ESP32 en la red que creó
    Serial.printf("OK\n");
    Serial.printf("Red: '%s' — IP: %s\n", NOMBRE_RED, WiFi.softAPIP().toString().c_str());
  } else {
    Serial.println(F("ERROR: No se pudo crear el Access Point"));
  }
}


// ═══════════════════════════════════════════════════════════════════════════
//  SECCIÓN 20 — SERVIDOR WEB
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Registra las rutas URL del servidor web.
 *
 * Cuando alguien escribe una URL en el navegador, el servidor
 * llama a la función correspondiente para generar la respuesta.
 *
 * Rutas disponibles:
 *   http://192.168.4.1/        → Página HTML con el historial de partidas
 *   http://192.168.4.1/csv    → Descarga del archivo CSV
 *   http://192.168.4.1/estado → Estado actual del juego en formato JSON
 */
void configurarServidorWeb() {

  servidor.on("/",       HTTP_GET, paginaPrincipal);  // Ruta principal
  servidor.on("/csv",    HTTP_GET, descargarCSV);     // Descarga del CSV
  servidor.on("/estado", HTTP_GET, estadoJSON);       // Estado en JSON

  servidor.begin();   // Iniciar el servidor (queda escuchando en el puerto 80)
  Serial.println(F("Servidor web iniciado en http://192.168.4.1"));
}

/**
 * @brief Genera y envía la página HTML principal con el historial de partidas.
 *
 * Lee el archivo CSV de SPIFFS y construye una tabla HTML con los datos.
 * También muestra estadísticas del juego actual.
 */
void paginaPrincipal() {

  // Construir la tabla HTML con las partidas guardadas en SPIFFS
  String filas = "";

  if (SPIFFS.exists(ARCHIVO_HISTORIAL)) {
    File archivo = SPIFFS.open(ARCHIVO_HISTORIAL, "r");   // Abrir en modo lectura

    if (archivo) {
      bool primeraLinea = true;   // La primera línea es el encabezado del CSV

      while (archivo.available()) {
        String linea = archivo.readStringUntil('\n');   // Leer una línea completa
        linea.trim();                                    // Eliminar espacios y saltos de línea

        if (linea.isEmpty()) continue;   // Saltar líneas vacías

        // Separar los campos de la línea CSV por coma
        // Los campos son: Partida, Tiempo, Puntaje, Resultado, Nivel
        String campos[5];
        int cantCampos = 0;
        int inicio = 0;
        for (int i = 0; i <= (int)linea.length() && cantCampos < 5; i++) {
          if (i == (int)linea.length() || linea[i] == ',') {
            campos[cantCampos++] = linea.substring(inicio, i);   // Extraer campo
            inicio = i + 1;   // El próximo campo empieza después de la coma
          }
        }

        if (primeraLinea) {
          // La primera línea es el encabezado: crear celdas <th>
          filas += "<tr>";
          for (int i = 0; i < cantCampos; i++) filas += "<th>" + campos[i] + "</th>";
          filas += "</tr>";
          primeraLinea = false;
        } else {
          // Las demás líneas son datos: crear celdas <td>
          filas += "<tr>";
          for (int i = 0; i < cantCampos; i++) filas += "<td>" + campos[i] + "</td>";
          filas += "</tr>";
        }
      }
      archivo.close();   // Cerrar el archivo cuando terminamos de leerlo
    }
  } else {
    // No hay archivo todavía
    filas = "<tr><td colspan='5'>Todavía no se jugó ninguna partida.</td></tr>";
  }

  // Traducir el estado actual a texto legible
  const char* textoEstado[] = { "Esperando jugador", "Jugando", "Victoria", "Derrota" };

  // Construir el documento HTML completo
  // F() guarda el texto en la memoria flash para no ocupar la RAM del ESP32
  String html = "";
  html += F("<!DOCTYPE html><html lang='es'>");
  html += F("<head><meta charset='UTF-8'>");
  html += F("<meta name='viewport' content='width=device-width, initial-scale=1'>");
  html += F("<meta http-equiv='refresh' content='10'>"); // Actualizar cada 10 segundos
  html += F("<title>Juego NeoPixel ESP32</title>");
  html += F("<style>");
  html += F("body { font-family: sans-serif; background: #0f172a; color: #e2e8f0; margin: 16px; }");
  html += F("h1   { color: #22c55e; margin-bottom: 4px; }");
  html += F(".sub { color: #64748b; font-size: 13px; margin-bottom: 20px; }");
  html += F(".stats { display: flex; flex-wrap: wrap; gap: 10px; margin-bottom: 20px; }");
  html += F(".card { background: #1e293b; border-radius: 8px; padding: 10px 16px; min-width: 120px; }");
  html += F(".card .label { font-size: 11px; color: #94a3b8; margin-bottom: 4px; }");
  html += F(".card .valor { font-size: 22px; font-weight: 600; }");
  html += F("table { width: 100%; border-collapse: collapse; margin-top: 8px; }");
  html += F("th { background: #1e293b; color: #86efac; padding: 9px 10px; text-align: left; font-weight: 500; }");
  html += F("td { padding: 7px 10px; border-bottom: 1px solid #1e293b; font-size: 14px; }");
  html += F("tr:hover td { background: #1e293b; }");
  html += F("a.btn { display: inline-block; margin-top: 14px; background: #1e293b;");
  html += F("        color: #93c5fd; border: 1px solid #334155; padding: 7px 16px;");
  html += F("        border-radius: 6px; text-decoration: none; font-size: 13px; }");
  html += F("a.btn:hover { background: #334155; }");
  html += F("footer { margin-top: 24px; font-size: 11px; color: #475569; }");
  html += F("</style></head><body>");

  html += F("<h1>&#127918; Juego Capacitivo NeoPixel</h1>");
  html += F("<p class='sub'>Grilla 5 &times; 8 &mdash; 800 LEDs WS2812B &mdash; ESP32 Access Point</p>");

  // Tarjetas de estadísticas
  html += F("<div class='stats'>");

  html += F("<div class='card'><div class='label'>Partidas jugadas</div><div class='valor'>");
  html += totalPartidas;
  html += F("</div></div>");

  html += F("<div class='card'><div class='label'>Estado</div><div class='valor' style='font-size:14px;padding-top:6px;'>");
  html += textoEstado[estadoActual];
  html += F("</div></div>");

  html += F("<div class='card'><div class='label'>Puntaje actual</div><div class='valor'>");
  html += puntajeActual;
  html += F(" / 10</div></div>");

  html += F("<div class='card'><div class='label'>Nivel actual</div><div class='valor'>");
  html += nivelActual;
  html += F("</div></div>");

  html += F("<div class='card'><div class='label'>Vel. barrido</div><div class='valor' style='font-size:15px;padding-top:4px;'>");
  html += intervaloBarrido;
  html += F(" ms</div></div>");

  html += F("</div>"); // Cierre de .stats

  // Tabla del historial
  html += F("<h2 style='font-size:16px;font-weight:500;margin-bottom:6px;'>Hist&oacute;rico de partidas</h2>");
  html += F("<table><thead>");
  html += filas;
  html += F("</thead></table>");

  // Botón de descarga CSV
  html += F("<a class='btn' href='/csv'>&#128229; Descargar CSV</a>");

  // Pie de página
  html += F("<footer><p>P&aacute;gina se actualiza cada 10 segundos &mdash; ");
  html += F("<a href='/' style='color:#475569'>Actualizar ahora</a></p></footer>");

  html += F("</body></html>");

  // Enviar la respuesta HTTP con el código 200 (OK)
  servidor.send(200, "text/html; charset=UTF-8", html);
}

/**
 * @brief Envía el archivo CSV del historial para ser descargado.
 *
 * El header Content-Disposition: attachment hace que el navegador
 * abra el diálogo de "Guardar archivo" en lugar de mostrar el texto.
 */
void descargarCSV() {

  if (!SPIFFS.exists(ARCHIVO_HISTORIAL)) {
    // Si no existe el archivo, enviar un CSV con solo el encabezado
    servidor.send(200, "text/csv; charset=UTF-8",
                  "Partida,TiempoDesdeEncendido,Puntaje,Resultado,NivelAlcanzado\n");
    return;
  }

  File archivo = SPIFFS.open(ARCHIVO_HISTORIAL, "r");   // Abrir en lectura

  if (!archivo) {
    servidor.send(500, "text/plain", "Error al abrir el archivo");
    return;
  }

  // Agregar el header para forzar la descarga en el navegador
  servidor.sendHeader("Content-Disposition",
                       "attachment; filename=\"historial_juego_neopixel.csv\"");

  // Enviar el contenido del archivo directamente al cliente (eficiente con archivos grandes)
  servidor.streamFile(archivo, "text/csv; charset=UTF-8");

  archivo.close();   // Cerrar el archivo
}

/**
 * @brief Devuelve el estado actual del juego en formato JSON.
 *
 * JSON (JavaScript Object Notation) es un formato de texto muy usado
 * para intercambiar datos entre sistemas. Es fácil de leer y procesar.
 *
 * Ejemplo de respuesta:
 *   {"estado":"jugando","puntaje":3,"nivel":2,"columnaRoja":5,"partidas":7,"velocidadMs":360}
 */
void estadoJSON() {

  // Nombres de los estados en texto para incluir en el JSON
  const char* nombresEstado[] = { "inactivo", "jugando", "victoria", "derrota" };

  // Construir la cadena JSON manualmente
  String json = "{";
  json += "\"estado\":\"";     json += nombresEstado[estadoActual]; json += "\",";
  json += "\"puntaje\":";      json += puntajeActual;               json += ",";
  json += "\"nivel\":";        json += nivelActual;                 json += ",";
  json += "\"columnaRoja\":";  json += columnaRoja;                 json += ",";
  json += "\"partidas\":";     json += totalPartidas;               json += ",";
  json += "\"velocidadMs\":";  json += intervaloBarrido;            json += ",";
  json += "\"filas\":";        json += FILAS;                       json += ",";
  json += "\"columnas\":";     json += COLS;                        json += ",";
  json += "\"totalLeds\":";    json += TOTAL_LEDS;
  json += "}";

  // Permitir acceso desde cualquier origen (CORS → útil si se consulta desde otra app)
  servidor.sendHeader("Access-Control-Allow-Origin", "*");

  // Enviar el JSON con código 200 (OK) y tipo de contenido JSON
  servidor.send(200, "application/json", json);
}


// ═══════════════════════════════════════════════════════════════════════════
//  SECCIÓN 21 — SPIFFS (Sistema de Archivos en la Memoria Flash)
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Inicializa el sistema de archivos SPIFFS.
 *
 * SPIFFS (SPI Flash File System) permite leer y escribir archivos
 * en la memoria flash del ESP32, igual que una memoria USB pequeña.
 * Los datos persisten aunque se apague el ESP32.
 *
 * Si el montaje falla, el parámetro 'true' hace que lo formatee
 * automáticamente y lo intente de nuevo.
 *
 * IMPORTANTE: en Arduino IDE hay que seleccionar en Herramientas →
 * Partition Scheme → "Default 4MB with spiffs" para que exista
 * espacio reservado para SPIFFS en la flash.
 */
void inicializarSPIFFS() {

  Serial.print(F("Inicializando SPIFFS... "));

  // Intentar montar SPIFFS. Si falla, formatear y reintentar (true).
  if (!SPIFFS.begin(true)) {
    Serial.println(F("ERROR: No se pudo montar SPIFFS"));
    return;
  }
  Serial.println(F("OK"));

  // Si el archivo de historial no existe todavía, crearlo con el encabezado del CSV
  if (!SPIFFS.exists(ARCHIVO_HISTORIAL)) {
    File archivo = SPIFFS.open(ARCHIVO_HISTORIAL, "w");   // Crear en modo escritura

    if (archivo) {
      // Escribir la primera línea con los nombres de las columnas
      archivo.println("Partida,TiempoDesdeEncendido,Puntaje,Resultado,NivelAlcanzado");
      archivo.close();   // Cerrar el archivo para guardar los cambios
      Serial.println(F("Archivo CSV creado con encabezado"));
    } else {
      Serial.println(F("ERROR: No se pudo crear el archivo CSV"));
    }
  }

  // Mostrar el espacio disponible en SPIFFS
  Serial.printf("SPIFFS — Total: %d bytes | Usado: %d bytes\n",
    SPIFFS.totalBytes(), SPIFFS.usedBytes());
}

/**
 * @brief Guarda el resultado de la partida en el archivo CSV de SPIFFS.
 *
 * Abre el archivo en modo "append" (agregar al final, sin borrar lo anterior)
 * y escribe una línea con los datos de la partida terminada.
 *
 * @param gano true si el jugador ganó los 10 puntos,
 *             false si fue alcanzado por la barra roja.
 */
void guardarPartida(bool gano) {

  // Abrir el archivo en modo 'a' = append (agregar al final)
  File archivo = SPIFFS.open(ARCHIVO_HISTORIAL, "a");

  if (!archivo) {
    Serial.println(F("ERROR: No se pudo abrir el CSV para escribir"));
    return;
  }

  // Construir la línea de datos separada por comas (formato CSV)
  String linea = "";
  linea += totalPartidas;                          // Número de partida
  linea += ",";
  linea += tiempoTranscurrido();                   // Tiempo desde encendido
  linea += ",";
  linea += puntajeActual;                          // Puntaje obtenido
  linea += ",";
  linea += gano ? "Victoria" : "Derrota";          // Resultado de la partida
  linea += ",";
  linea += nivelActual;                            // Nivel máximo que alcanzó

  archivo.println(linea);   // Escribir la línea y agregar salto de línea
  archivo.close();          // Cerrar el archivo → esto garantiza que se guarden los datos

  Serial.printf("Partida guardada en SPIFFS: %s\n", linea.c_str());
}

/**
 * @brief Devuelve el tiempo transcurrido desde el encendido del ESP32.
 *
 * Como el ESP32 en modo AP no tiene acceso a servidores de hora (NTP),
 * usamos millis() para indicar cuánto tiempo lleva encendido.
 * El formato es: horas, minutos y segundos (HH:MM:SS).
 *
 * @return String con el tiempo en formato "T+HH:MM:SS".
 */
String tiempoTranscurrido() {

  unsigned long totalSegundos = millis() / 1000;          // Convertir ms a segundos
  unsigned long horas         = totalSegundos / 3600;     // Calcular horas completas
  unsigned long minutos       = (totalSegundos % 3600) / 60; // Minutos restantes
  unsigned long segundos      = totalSegundos % 60;       // Segundos restantes

  char buffer[16];   // Buffer de texto donde armar el string
  snprintf(buffer, sizeof(buffer), "T+%02lu:%02lu:%02lu", horas, minutos, segundos);
  // %02lu → número entero sin signo, mínimo 2 dígitos, rellena con cero a la izquierda

  return String(buffer);   // Convertir el buffer de texto en un String de Arduino
}


// ═══════════════════════════════════════════════════════════════════════════
//  FIN DEL CÓDIGO
// ═══════════════════════════════════════════════════════════════════════════
/*
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │           TABLA COMPLETA DE ASIGNACIÓN DE PINES GPIO                │
 * ├──────────────────────┬──────────┬───────────────────────────────────┤
 * │ Función              │ GPIO     │ Notas                             │
 * ├──────────────────────┼──────────┼───────────────────────────────────┤
 * │ MUX — bit A          │ GPIO  4  │ Salida digital                    │
 * │ MUX — bit B          │ GPIO  5  │ Salida digital                    │
 * │ MUX — bit C          │ GPIO 18  │ Salida digital                    │
 * │ Fila 0 (lectura)     │ GPIO 34  │ Solo entrada — pull-down externo  │
 * │ Fila 1 (lectura)     │ GPIO 35  │ Solo entrada — pull-down externo  │
 * │ Fila 2 (lectura)     │ GPIO 32  │ Entrada + pull-down interno       │
 * │ Fila 3 (lectura)     │ GPIO 33  │ Entrada + pull-down interno       │
 * │ Fila 4 (lectura)     │ GPIO 25  │ Entrada + pull-down interno       │
 * │ NeoPixel DIN         │ GPIO 26  │ Resistor 470 Ω en serie           │
 * │ Botón INICIO         │ GPIO 27  │ Pull-down 10 kΩ externo           │
 * │ Botón RESET          │ GPIO 15  │ Pull-down 10 kΩ externo           │
 * ├──────────────────────┴──────────┴───────────────────────────────────┤
 * │ RESUMEN DE LA GRILLA                                                │
 * │  Filas: 5  —  Columnas: 8  —  Total celdas: 40                     │
 * │  LEDs por celda: 20  —  Total de LEDs: 800                         │
 * │  Corriente máxima (100% blanco): 800 × 60 mA = 48.0 A              │
 * │  Corriente con brillo al 30%:    800 × 60 mA × 0.30 ≈ 14.4 A      │
 * │  Fuente recomendada: 5 V / 15 A o más                              │
 * ├──────────────────────────────────────────────────────────────────────┤
 * │ MAPA DE ÍNDICES DE LEDs (primera celda de cada fila)               │
 * │  [0][0] → LEDs   0-19    [1][0] → LEDs 160-179                     │
 * │  [0][1] → LEDs  20-39    [2][0] → LEDs 320-339                     │
 * │  [0][7] → LEDs 140-159   [4][7] → LEDs 780-799                     │
 * └──────────────────────────────────────────────────────────────────────┘
 */
