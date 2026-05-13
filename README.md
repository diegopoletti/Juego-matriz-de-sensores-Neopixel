# 🎮 Juego Capacitivo NeoPixel — ESP32

> Juego interactivo de pisada sobre una grilla de sensores capacitivos iluminados con tiras de LEDs RGB, controlado por un ESP32.
> <img width="1024" height="559" alt="image" src="https://github.com/user-attachments/assets/e6d7d13b-f298-47f8-9f03-c48ee0442550" />

## 🕹️ Simulador jugable en el navegador

Podés probar el juego directamente desde el navegador, sin necesidad de ningún hardware.
> https://diegopoletti.github.io/Juego-matriz-de-sensores-Neopixel/simulador.html

### Controles del simulador

| Acción | Teclado | Mobile |
|---|---|---|
| Mover selección | `W` `A` `S` `D` o flechas | — |
| Pisar celda | `Espacio` o `Enter` | Tocar la celda |
| Iniciar partida | `I` | Botón "Iniciar partida" |
| Resetear | `R` | Botón "Resetear" |

### Qué simula

El simulador reproduce fielmente la lógica del firmware:

- Grilla de **5 filas × 8 columnas** con **20 LEDs por celda** (representados visualmente).
- **Barra roja** que se mueve de izquierda a derecha y rebota en los extremos.
- **Celdas verdes** que aparecen aleatoriamente (1 o 2 por vez, nunca en la columna roja).
- **Dificultad progresiva**: la barra acelera 40 ms por nivel, desde 520 ms hasta 120 ms mínimo.
- **Historial de partidas** con puntaje, nivel máximo y velocidad final.
- **Barra de progreso** con los 10 colores del firmware (`coloresProgreso[]`).
- Animaciones de victoria y derrota.

---

## 📖 ¿Qué hace este proyecto?

Una **barra roja** se desplaza de izquierda a derecha y de derecha a izquierda sobre una grilla de **5 filas × 8 columnas** de sensores capacitivos. Debajo de cada sensor hay una tira de **20 LEDs WS2812B**.

El jugador debe **pisar la celda verde** sin que la barra roja esté pasando por esa columna. Si lo logra, suma un punto. Con **10 puntos gana**. Si la barra roja lo detecta pisando su columna, **pierde**.

El juego incluye:
- Dificultad progresiva: la barra se mueve más rápido con cada punto.
- Servidor web integrado vía Wi-Fi (modo Access Point) con historial de partidas.
- Almacenamiento de estadísticas en la memoria flash del ESP32 (SPIFFS).
- Panel web accesible desde cualquier dispositivo conectado a la red del ESP32.
- **Distribución serpenteante de la tira LED** — sin cortes de cable entre filas (novedad v3.0).

---

## 🗺️ Descripción de la grilla

```
        Col 0   Col 1   Col 2   Col 3   Col 4   Col 5   Col 6   Col 7
Fila 0  [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]
Fila 1  [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]
Fila 2  [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]
Fila 3  [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]
Fila 4  [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]   [ ○ ]

○ = 20 LEDs WS2812B + 1 sensor TTP223 por celda
Total: 40 celdas · 800 LEDs
```

---

## 💡 Novedad v3.0 — Distribución serpenteante de la tira LED

### ¿Por qué se cambió?

En la versión anterior (v2.x), la tira recorría la grilla **fila por fila** de izquierda a derecha. Esto obligaba a **cortar y reconectar** la tira 4 veces (una vez al pasar de cada fila a la siguiente), lo que introducía puntos de falla mecánica y complicaba el tendido físico.

### ¿Cómo funciona ahora?

La tira recorre la grilla en **zigzag columna por columna**: baja por las columnas pares y sube por las impares. Al llegar al final de una columna, dobla en U hacia la siguiente sin necesidad de ningún corte.

```
  Col 0 ↓   Col 1 ↑   Col 2 ↓   Col 3 ↑   Col 4 ↓   Col 5 ↑   Col 6 ↓   Col 7 ↑
  [0][0]    [4][1]    [0][2]    [4][3]    [0][4]    [4][5]    [0][6]    [4][7]
  [1][0]    [3][1]    [1][2]    [3][3]    [1][4]    [3][5]    [1][6]    [3][7]
  [2][0]    [2][1]    [2][2]    [2][3]    [2][4]    [2][5]    [2][6]    [2][7]
  [3][0]    [1][1]    [3][2]    [1][3]    [3][4]    [1][5]    [3][6]    [1][7]
  [4][0]    [0][1]    [4][2]    [0][3]    [4][4]    [0][5]    [4][6]    [0][7]
              ↑ La tira dobla en U aquí, sin corte
```

### Tabla de índices de celdas físicas

La celda física número N × 20 indica el primer LED de esa celda en la cadena.

```
        Col 0   Col 1   Col 2   Col 3   Col 4   Col 5   Col 6   Col 7
Fila 0:   0       9      10      19      20      29      30      39
Fila 1:   1       8      11      18      21      28      31      38
Fila 2:   2       7      12      17      22      27      32      37
Fila 3:   3       6      13      16      23      26      33      36
Fila 4:   4       5      14      15      24      25      34      35
```

### Ejemplos de primer LED por celda

| Celda lógica | Celda física | Primer LED | Último LED |
|---|---|---|---|
| `[0][0]` | 0 | 0 | 19 |
| `[4][0]` | 4 | 80 | 99 |
| `[4][1]` | 5 | 100 | 119 |
| `[0][1]` | 9 | 180 | 199 |
| `[0][2]` | 10 | 200 | 219 |
| `[4][7]` | 35 | 700 | 719 |
| `[0][7]` | 39 | 780 | 799 |

> **Nota:** La barra de progreso usa los LEDs 0–9 de la celda `[0][0]`, que sigue siendo la primera de la cadena en la distribución serpenteante.

### Función clave: `calcularIndiceLed()`

```cpp
/**
 * Columnas pares  (0,2,4,6): la tira BAJA → fila 0 es la primera del grupo.
 *   numeroCelda = columna * FILAS + fila
 *
 * Columnas impares (1,3,5,7): la tira SUBE → fila 4 es la primera del grupo.
 *   numeroCelda = columna * FILAS + (FILAS - 1 - fila)
 */
int calcularIndiceLed(int fila, int columna) {
  int numeroCelda;
  if (columna % 2 == 0) {
    numeroCelda = columna * FILAS + fila;
  } else {
    numeroCelda = columna * FILAS + (FILAS - 1 - fila);
  }
  return numeroCelda * LEDS_POR_CELDA;
}
```

---

## 🔧 Hardware necesario

| Cantidad | Componente | Notas |
|---|---|---|
| 1 | ESP32 DevKit v1 | O cualquier módulo compatible |
| 1 | Multiplexor CD4051 | Selección de 8 columnas con 3 pines |
| 40 | Sensor capacitivo TTP223 | 5 filas × 8 columnas |
| 800 | LED WS2812B | 20 por celda, encadenados en serie |
| 1 | Resistor 470 Ω | Protección línea de datos DIN |
| 1 | Capacitor 1000 µF / 10 V | Desacople bus 5V de los LEDs |
| 1 | Regulador AMS1117-3.3 | Lógica a 3.3V para sensores y MUX |
| 2 | Capacitor 100 µF / 10 V | Desacople entrada y salida del regulador |
| 2 | Capacitor 100 nF cerámico | Filtro alta frecuencia (regulador + LEDs) |
| 2 | Pulsador normalmente abierto | BTN Inicio / BTN Reset |
| 4 | Resistor 10 kΩ | Pull-down (2 botones + GPIO34 + GPIO35) |
| 2 | Resistor 1 kΩ | Serie en línea de pulsadores |
| 2 | Capacitor 100 nF | Anti-rebote hardware de botones |
| 1 | Fuente DC 5V / ≥ 15 A | Alimentación principal del sistema |
| 1 | Fusible 20 A | Protección en bus 5V (recomendado) |

> **⚠️ Nota de tensión:** El ESP32 tolera máximo **3.6 V** en sus pines GPIO. Toda la lógica (CD4051 y TTP223) opera a 3.3 V suministrados por el regulador AMS1117-3.3.

---

## 📌 Asignación de pines GPIO

| Función | GPIO | Dirección | Notas |
|---|---|---|---|
| Multiplexor bit A | 4 | Salida | Bit 0 (LSB) selección de columna |
| Multiplexor bit B | 5 | Salida | Bit 1 selección de columna |
| Multiplexor bit C | 18 | Salida | Bit 2 (MSB) selección de columna |
| Lectura Fila 0 | 34 | Entrada | **Solo entrada** — pull-down 10 kΩ externo obligatorio |
| Lectura Fila 1 | 35 | Entrada | **Solo entrada** — pull-down 10 kΩ externo obligatorio |
| Lectura Fila 2 | 32 | Entrada | Pull-down activable por software |
| Lectura Fila 3 | 33 | Entrada | Pull-down activable por software |
| Lectura Fila 4 | 25 | Entrada | Pull-down activable por software |
| NeoPixel DIN | 26 | Salida | Resistor 470 Ω en serie |
| Botón Inicio | 27 | Entrada | Pull-down 10 kΩ + antirrebote 100 nF |
| Botón Reset | 15 | Entrada | Pull-down 10 kΩ + antirrebote 100 nF |

> **⚠️ GPIO34 y GPIO35:** Son pines de **entrada exclusiva** sin resistencia pull-down interna. El pull-down de 10 kΩ a GND es obligatorio para evitar lecturas flotantes.

---

## ⚡ Circuito de alimentación

```
Fuente 5V / 15A
     │
     ├─ [Fusible 20A] ─────────────── BUS +5V ──┬── VCC WS2812B (×800 LEDs)
     │                                          │   (inyección cada ≤80 LEDs)
     │                                          └── VIN ESP32
     │
     └─ [AMS1117-3.3]
          │  IN: C1 100µF || C2 100nF (entrada)
          │  OUT: C3 100µF || C4 100nF (salida)
          └── BUS +3.3V ──┬── ESP32 pin 3V3
                          ├── TTP223 VCC (×40)
                          └── CD4051 VDD / VEE
```

### Consumo estimado WS2812B

| Condición | Corriente | Potencia |
|---|---|---|
| Blanco 100% × 800 LEDs | 48.0 A | 240 W |
| **Brillo 30% (configurado en código)** | **≈ 14.4 A** | **≈ 72 W** |
| Fuente mínima recomendada | 15 A | 75 W |

> El código limita el brillo con `BRILLO_MAXIMO = 77` (30% de 255).

---

## 📡 Funcionamiento del multiplexor CD4051

El CD4051 permite leer 8 columnas usando solo 3 pines de control:

| C | B | A | Canal activo   |
|---|---|---|----------------|
| 0 | 0 | 0 | Y0 → Columna 0 |
| 0 | 0 | 1 | Y1 → Columna 1 |
| 0 | 1 | 0 | Y2 → Columna 2 |
| 0 | 1 | 1 | Y3 → Columna 3 |
| 1 | 0 | 0 | Y4 → Columna 4 |
| 1 | 0 | 1 | Y5 → Columna 5 |
| 1 | 1 | 0 | Y6 → Columna 6 |
| 1 | 1 | 1 | Y7 → Columna 7 |

El ESP32 activa una columna, lee las 5 filas, avanza a la siguiente columna y repite el ciclo.

---

## 🛠️ Instalación y configuración

### 1. Requisitos de software

- [Arduino IDE 2.x](https://www.arduino.cc/en/software) o superior
- **Core ESP32** para Arduino (versión 2.x o superior)
  - En Arduino IDE: `Archivo → Preferencias → URLs adicionales`
  - Agregar: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
  - Luego: `Herramientas → Placa → Gestor de placas` → buscar "esp32" e instalar
- **Adafruit NeoPixel** (versión 1.11 o superior)
  - `Herramientas → Administrar bibliotecas` → buscar "Adafruit NeoPixel" e instalar

### 2. Configuración de la partición de flash

El historial de partidas se guarda en SPIFFS. Para habilitarlo:

```
Herramientas → Partition Scheme → "Default 4MB with spiffs"
```

Esto reserva **1.5 MB** para el sistema de archivos donde se guarda `historial.csv`.

### 3. Cargar el código

1. Conectar el ESP32 al PC por USB.
2. Seleccionar la placa: `Herramientas → Placa → ESP32 Arduino → ESP32 Dev Module`.
3. Seleccionar el puerto COM correspondiente.
4. Verificar la partición (paso 2).
5. Hacer clic en **Subir** (→).

### 4. Verificar el funcionamiento

Abrir el **Monitor Serie** a `115200 baudios`. Al encender deberían aparecer mensajes como:

```
=== JuegoCapacitivoNeoPixel v3.0 ===
Grilla: 5 filas x 8 columnas = 40 celdas
Total de LEDs: 800 — Distribucion: SERPENTEANTE por columna
Inicializando SPIFFS... OK
Red: 'JuegoNeoPixel' — IP: 192.168.4.1
Servidor web iniciado en http://192.168.4.1
Sistema listo. Presiona BTN_INICIO para jugar.
```

---

## 🌐 Servidor web integrado

El ESP32 crea una red Wi-Fi propia en **modo Access Point**:

| Parámetro | Valor |
|---|---|
| Nombre de red (SSID) | `JuegoNeoPixel` |
| Contraseña | `esp32game` |
| Dirección IP | `192.168.4.1` |

### Endpoints disponibles

| Ruta | Descripción |
|---|---|
| `http://192.168.4.1/` | Panel principal con estadísticas en tiempo real |
| `http://192.168.4.1/csv` | Descarga del historial en formato CSV |
| `http://192.168.4.1/estado` | Estado del juego en formato JSON |

El panel web se **actualiza automáticamente cada 10 segundos**. Es accesible desde cualquier dispositivo (celular, tablet, PC) conectado a la red `JuegoNeoPixel`.

### Ejemplo de respuesta JSON (`/estado`)

```json
{
  "estado": "jugando",
  "puntaje": 3,
  "nivel": 2,
  "columnaRoja": 5,
  "partidas": 7,
  "velocidadMs": 440,
  "filas": 5,
  "columnas": 8,
  "totalLeds": 800
}
```

### Formato del CSV (`historial.csv`)

```
Partida,TiempoDesdeEncendido,Puntaje,Resultado,NivelAlcanzado
1,T+00:01:34,10,Victoria,5
2,T+00:03:12,4,Derrota,3
```

---

## 🎯 Mecánica del juego

### Estados posibles

| Estado | Descripción |
|---|---|
| `INACTIVO` | Esperando que el jugador presione el botón Inicio |
| `JUGANDO` | Partida en curso |
| `VICTORIA` | El jugador alcanzó 10 puntos — animación verde |
| `DERROTA` | La barra roja atrapó al jugador — animación roja |

### Flujo de una partida

```
[Presionar BTN Inicio]
        ↓
  Grilla se activa
  Celda(s) verde(s) aparecen al azar
  Barra roja comienza a moverse
        ↓
  Jugador pisa celda verde ──→ +1 punto → nueva celda verde
  Jugador pisa columna roja ──→ DERROTA
        ↓
  Jugador llega a 10 puntos → VICTORIA
        ↓
  [Presionar BTN Inicio para nueva partida]
  [Presionar BTN Reset en cualquier momento para reiniciar]
```

### Sistema de dificultad progresiva

| Evento | Efecto |
|---|---|
| Velocidad inicial | 520 ms entre pasos de la barra |
| Por cada 2 puntos | Se reduce 40 ms el intervalo |
| Velocidad mínima | 120 ms (nivel máximo) |
| Nivel máximo | 10 |

---

## 🗂️ Estructura del código

El archivo `JuegoCapacitivoNeoPixel_serpentine.ino` está organizado en **22 secciones** comentadas didácticamente:

| Sección | Contenido |
|---|---|
| 1 | Librerías incluidas |
| 2 | Definición de pines GPIO |
| 3 | Dimensiones de la grilla |
| 4 | Constantes de juego |
| 5 | Configuración Wi-Fi |
| 6 | Objetos globales (NeoPixel, WebServer) |
| 7 | Variables de estado del juego |
| 8 | Declaraciones anticipadas de funciones |
| 9 | `setup()` — inicialización del sistema |
| 10 | `loop()` — ciclo principal del juego |
| 11 | Manejo de botones con debounce |
| 12 | Multiplexor CD4051 |
| 13 | Escaneo de sensores capacitivos |
| 14 | Movimiento de la barra roja |
| 15 | Lógica principal del juego |
| 16 | **`calcularIndiceLed()` — mapeo serpenteante** ← nuevo en v3.0 |
| 17 | Renderizado de LEDs (`dibujarLeds`, `colorearCelda`, `dibujarBarraProgreso`) |
| 18 | Gestión de partidas (inicio / fin / reset) |
| 19 | Animaciones de victoria y derrota |
| 20 | Wi-Fi (modo Access Point) |
| 21 | Servidor web (rutas, HTML, JSON, CSV) |
| 22 | SPIFFS — sistema de archivos flash |

---

## 📋 Constantes configurables

Estas constantes se encuentran al inicio del archivo `.ino` y permiten ajustar el comportamiento del juego sin modificar la lógica:

```cpp
#define PUNTOS_PARA_GANAR     10    // Puntos necesarios para ganar
#define VELOCIDAD_INICIAL_MS  520   // Intervalo inicial de la barra (ms)
#define VELOCIDAD_MINIMA_MS   120   // Intervalo mínimo (máxima dificultad)
#define REDUCCION_POR_NIVEL   40    // Reducción de intervalo por nivel (ms)
#define PAUSA_VERDE_MS        350   // Pausa antes de nueva celda verde (ms)
#define BRILLO_MAXIMO         77    // Brillo LEDs: 77/255 = 30%
#define DEBOUNCE_MS           50    // Tiempo anti-rebote pulsadores (ms)
```

La red Wi-Fi también es personalizable:

```cpp
const char* NOMBRE_RED = "JuegoNeoPixel";
const char* CLAVE_RED  = "esp32game";
```

---

## ⚠️ Consideraciones eléctricas importantes

### Protección del ESP32 frente al ruido

- **Capacitores de desacople:** Colocar 100 nF cerámico en paralelo con cada CI (CD4051, TTP223) lo más cerca posible de los pines VCC/GND. Los capacitores cerámicos filtran el ruido de alta frecuencia que los electrolíticos no pueden absorber.
- **Plano de tierra:** En la PCB, usar un plano de GND continuo. Minimizar las impedancias de tierra.
- **Cable de datos NeoPixel corto:** Mantener el cable entre el pin GPIO26 y el DIN del primer LED lo más corto posible (< 30 cm idealmente). El resistor de 470 Ω forma un filtro RC con la capacitancia del cable.

### Inyección de corriente en los LEDs

Con 800 LEDs, **no alimentar toda la tira desde un único punto**. Dividir en grupos de ≤ 80 LEDs y conectar cada grupo al bus 5V con cables de ≥ 1.5 mm². Esto evita caídas de tensión que pueden causar colores incorrectos en los últimos LEDs o reinicios del ESP32.

> **Con la distribución serpenteante**, los puntos naturales de inyección de corriente coinciden con la doblez de la tira al pasar de una columna a la siguiente, lo que facilita físicamente el acceso al bus de alimentación en esos puntos.

### GPIO34 y GPIO35

Estos pines no tienen resistencia pull-down interna. Sin el pull-down de 10 kΩ externo a GND, el pin flota y produce lecturas aleatorias (falsos positivos en los sensores). Es obligatorio colocar la resistencia aunque el TTP223 esté conectado.

### Compatibilidad de tensiones DIN / WS2812B

El WS2812B alimentado a 5V requiere que la señal DIN sea ≥ 3.5V (70% de VCC). El ESP32 entrega 3.3V, lo cual está en el límite. El resistor de 470 Ω mejora el nivel lógico percibido. Si la tira no responde correctamente, agregar un **74HCT125** como convertidor de nivel 3.3V → 5V entre el GPIO26 y el DIN del primer LED.

---

## 📁 Archivos del proyecto

```
JuegoCapacitivoNeoPixel/
├── JuegoCapacitivoNeoPixel_serpentine.ino   ← Código fuente principal (v3.0)
└── README.md                                 ← Este archivo
```

---

## 🧩 Dependencias

| Librería | Versión mínima | Instalación |
|---|---|---|
| Adafruit NeoPixel | 1.11 | Gestor de bibliotecas Arduino |
| ESP32 Arduino Core | 2.0 | Gestor de placas Arduino |
| WebServer.h | Incluida en el core | — |
| SPIFFS.h | Incluida en el core | — |

---

## 📜 Historial de versiones

| Versión | Fecha | Cambios principales |
|---|---|---|
| v1.0.0 | 10/04/2026         | Versión inicial — grilla 5×5 |
| v2.1.0 | 17/04/2026         | Ampliado a 8 columnas, 20 LEDs por celda, servidor web |
| **v3.0.0** | **13/05/2026** | **Distribución serpenteante por columna — sin cortes de tira. Nueva función `calcularIndiceLed()`. Nombres de variables y funciones en español. Comentarios didácticos en cada línea.** |

---

## 🔌 Esquema Eléctrico

### Índice

1. [Resumen del Sistema](#resumen)
2. [Lista de Componentes](#componentes)
3. [Bloque 1 — Alimentación Principal 5V](#alimentacion5v)
4. [Bloque 2 — Regulador 3.3V para ESP32](#regulador33v)
5. [Bloque 3 — ESP32 DevKit v1](#esp32)
6. [Bloque 4 — Multiplexor CD4051](#mux)
7. [Bloque 5 — Sensores TTP223 (matriz 5×8)](#sensores)
8. [Bloque 6 — Tiras NeoPixel WS2812B](#neopixel)
9. [Bloque 7 — Pulsadores](#pulsadores)
10. [Diagrama de Conexiones Completo](#diagrama)
11. [Tabla Resumen de Pines](#tabla)
12. [Notas de Diseño Robusto](#notas)

---

### 1. Resumen del Sistema {#resumen}

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                    SISTEMA JUEGO CAPACITIVO NEOPIXEL v3.0                       │
│                                                                                  │
│  Tensión principal : 5 V DC / mínimo 15 A                                       │
│  Tensión lógica ESP32 : 3.3 V (regulada)                                        │
│  Grilla sensores  : 5 filas × 8 columnas = 40 celdas                            │
│  LEDs totales     : 800 × WS2812B (20 LEDs/celda)                               │
│  Corriente LEDs   : ≈ 14.4 A (brillo 30%)                                       │
│  Distribución LED : Serpenteante por columna (sin cortes de tira)               │
└─────────────────────────────────────────────────────────────────────────────────┘
```

> ⚠️ **ADVERTENCIA CRÍTICA:** Las entradas del ESP32 operan a **3.3 V máximo**.
> Nunca conectar directamente señales de 5 V a pines GPIO del ESP32.
> Los sensores TTP223 se alimentarán a **3.3 V** para que su salida lógica
> sea compatible directamente con el ESP32 sin necesidad de divisores de tensión.

---

### 2. Lista de Componentes {#componentes}

| # | Componente | Cantidad | Valor / Modelo | Función |
|---|-----------|---------|---------------|---------|
| 1 | ESP32 DevKit v1 | 1 | 38 pines | Microcontrolador principal |
| 2 | Regulador de tensión | 1 | **AMS1117-3.3** o **LM1117-3.3** | Genera los 3.3 V estables para ESP32 y sensores |
| 3 | Capacitor electrolítico | 1 | **10 µF / 16 V** | Filtro entrada regulador 3.3 V |
| 4 | Capacitor electrolítico | 1 | **10 µF / 16 V** | Filtro salida regulador 3.3 V |
| 5 | Capacitor cerámico | 2 | **100 nF (0.1 µF)** | Desacople HF entrada y salida regulador |
| 6 | Capacitor electrolítico | 1 | **1000 µF / 10 V** | Filtro principal tira NeoPixel |
| 7 | Capacitor cerámico | 1 | **100 nF** | Desacople HF línea NeoPixel |
| 8 | Resistor | 1 | **470 Ω / 1/4 W** | Protección línea datos NeoPixel (evita reflexiones) |
| 9 | Resistor | 2 | **10 kΩ / 1/4 W** | Pull-down Fila 0 (GPIO34) y Fila 1 (GPIO35) |
| 10 | Resistor | 2 | **10 kΩ / 1/4 W** | Pull-down botón INICIO (GPIO27) y RESET (GPIO15) |
| 11 | Resistor | 3 | **100 Ω / 1/4 W** | Serie en líneas control MUX (anti-ruido) |
| 12 | Resistor | 5 | **100 Ω / 1/4 W** | Serie en líneas de fila sensores (protección) |
| 13 | Multiplexor | 1 | **CD4051BE** | Selector 8 canales para columnas sensores |
| 14 | Capacitor cerámico | 1 | **100 nF** | Desacople VCC del CD4051 |
| 15 | Sensor capacitivo | 40 | **TTP223** | Detección táctil por celda (5×8 grilla) |
| 16 | Tira LED | 40 | **WS2812B** (20 LEDs c/u) | Iluminación por celda |
| 17 | Pulsador NA | 2 | Normalmente abierto | Botón INICIO y RESET |
| 18 | Fuente de alimentación | 1 | **5 V / 15 A mínimo** | Alimentación principal |
| 19 | Diodo Schottky | 1 | **1N5822 o SS34** | Protección inversión de polaridad en entrada 5V |
| 20 | Capacitor electrolítico | 1 | **470 µF / 10 V** | Filtro adicional 3.3V junto al ESP32 |

---

### 3. Bloque 1 — Alimentación Principal 5 V {#alimentacion5v}

#### 🟡 Explicación didáctica

La fuente de 5 V debe suministrar hasta **15 A** porque los 800 LEDs WS2812B, al brillo del 30%, consumen aproximadamente 14.4 A. Se agrega protección y filtrado para evitar que los transitorios de corriente de los LEDs afecten la lógica del ESP32.

> 💡 **¿Qué es un transitorio de corriente?** Cuando muchos LEDs encienden al mismo tiempo, en un instante muy breve consumen mucha corriente de golpe. Esto puede hacer "caer" la tensión del bus y confundir al ESP32. Los capacitores actúan como una "reserva de energía" que entrega corriente en esos momentos críticos.

```
 FUENTE 5V/15A
 ┌──────────┐
 │  +5V ───►├──[D1: 1N5822]──────┬──────────────────────────► +5V_BUS
 │           │                   │            │
 │           │              ╔════╧════╗    ╔══╧══╗
 │           │              ║C1:1000µF║    ║C2:  ║
 │           │              ║  /10V   ║    ║100nF║
 │           │              ╚════╤════╝    ╚══╤══╝
 │  GND  ───►├───────────────────┴────────────┴────────────── GND_BUS
 └──────────┘

 Consumo estimado:
 ├── LEDs WS2812B: 800 × 60mA × 30% = 14.4 A
 ├── ESP32 + lógica: ≈ 0.3 A
 └── TOTAL: ≈ 14.7 A  →  Fuente MÍNIMA: 5V / 15A
```

---

### 4. Bloque 2 — Regulador de 3.3 V {#regulador33v}

#### 🟢 Explicación

El ESP32 opera internamente a **3.3 V**. Si se conectan los **40 sensores TTP223** también a 3.3 V, la demanda de corriente puede superar la capacidad del regulador interno del DevKit. **Solución:** Agregar un regulador **AMS1117-3.3** externo con filtros adecuados.

```
  REGULADOR AMS1117-3.3

       +5V_BUS
          │
          │   ┌─────────────────────┐
          ├───┤ IN    AMS1117-3.3   ├──── OUT ──────────────────────► +3.3V_BUS
          │  ││                     │         │           │
          │  ││       ADJ/GND  ─────┤─── GND  │           │
          │  │└─────────────────────┘    │   ╔╧╗ C5      ╔╧╗ C6
         ╔╧╗ C3                          │   ║ ║ 10µF    ║ ║ 100nF
         ║ ║ 10µF/16V  ← entrada         │   ║ ║ /10V    ║ ║ cerám.
          │ ╔╧╗ C4                       │   ╚╤╝         ╚╤╝
          │ ║ ║ 100nF ceramic            │    │            │
          │ ╚╤╝                          └────┴────────────┴─────── GND_BUS
          │  │
         GND_BUS

  Corriente de salida AMS1117-3.3:
  ├── 40 × TTP223 @ ≈1mA c/u  = 40 mA
  ├── CD4051                   = 1  mA
  ├── ESP32 (lógica)           = 80 mA (típico en operación WiFi)
  └── TOTAL                    ≈ 121 mA  ← muy por debajo del límite de 1A ✓
```

> 💡 **¿Por qué 4 capacitores en el regulador?** Los capacitores electrolíticos (10 µF) filtran variaciones lentas de tensión. Los cerámicos (100 nF) filtran el ruido de radio frecuencia que podría desestabilizar el regulador y afectar las lecturas del ESP32.

---

### 5. Bloque 3 — ESP32 DevKit v1 {#esp32}

#### 🔵 Pinout y Conexiones ESP32

```
                        ╔══════════════════════════════════╗
                        ║         ESP32 DevKit v1          ║
              +3.3V ───►║ 3V3   [1]           [38] GND     ║◄─── GND_BUS
                        ║ EN    [2]           [37] GPIO23  ║
                        ║ SVP   [3] GPIO36    [36] GPIO22  ║
                        ║ SVN   [4] GPIO39    [35] TX0     ║
GPIO34 FILA_0 ◄────────►║       [5] GPIO34    [34] RX0     ║
GPIO35 FILA_1 ◄────────►║       [6] GPIO35    [33] GPIO21  ║
GPIO32 FILA_2 ◄────────►║       [7] GPIO32    [30] GND     ║◄─── GND_BUS
GPIO33 FILA_3 ◄────────►║       [8] GPIO33    [29] GPIO19  ║
GPIO25 FILA_4 ◄────────►║       [9] GPIO25    [28] GPIO18  ║───► MUX_C (pin C)
GPIO26 NEOPIXEL ───────►║      [10] GPIO26    [27] GPIO5   ║───► MUX_B (pin B)
GPIO27 BTN_INICIO ◄────►║      [11] GPIO27    [26] GPIO17  ║
GPIO14          ────────║      [12] GPIO14    [25] GPIO16  ║
GPIO12          ────────║      [13] GPIO12    [24] GPIO4   ║───► MUX_A (pin A)
              GND ─────►║ GND  [14]           [23] GPIO0   ║
GPIO13          ────────║      [15] GPIO13    [22] GPIO2   ║
GPIO9  (SD2)    ────────║ SD2  [16]           [21] GPIO15  ║◄──── BTN_RESET
GPIO10 (SD3)    ────────║ SD3  [17]           [20] GPIO8   ║
GPIO11 (CMD)    ────────║ CMD  [18]           [19] GPIO7   ║
             +5V ──────►║ 5V   [19]           [18] GPIO6   ║
                        ╚══════════════════════════════════╝
```

---

### 6. Bloque 4 — Multiplexor CD4051 {#mux}

#### 🟠 Explicación didáctica

El **CD4051** es un multiplexor/demultiplexor analógico de 8 canales. Con 3 señales de control (A, B, C) selecciona cuál de los 8 canales (Y0-Y7) se conecta al pin común. Se alimenta a **3.3V** para compatibilidad directa con los pines del ESP32.

```
  CD4051BE — Pinout (DIP-16)

              ┌─────────────────┐
   Y4 ───────►│ 1           16  │◄─── VDD (+3.3V)
   Y6 ───────►│ 2           15  │◄─── A (GPIO4 via R=100Ω)
   Y2 ───────►│ 3           14  │◄─── B (GPIO5 via R=100Ω)
   Y7 ───────►│ 4           13  │◄─── C (GPIO18 via R=100Ω)
   Y3 ───────►│ 5           12  │──── INH → GND (siempre habilitado)
   Y5 ───────►│ 6           11  │◄─── VEE → GND
   Y1 ───────►│ 7           10  │──── Z (salida común — no usado)
   Y0 ───────►│ 8            9  │──── VSS (GND)
              └─────────────────┘

  Desacople VCC:
      VDD (+3.3V) ───  ╔╧╗ C7: 100nF (junto al pin VDD del IC)
                       ╚╤╝
                       GND

  Líneas de control con resistores anti-ruido:
  GPIO4  (ESP32) ──[R_A: 100Ω]──► Pin 15 (A) del CD4051
  GPIO5  (ESP32) ──[R_B: 100Ω]──► Pin 14 (B) del CD4051
  GPIO18 (ESP32) ──[R_C: 100Ω]──► Pin 13 (C) del CD4051
```

---

### 7. Bloque 5 — Sensores TTP223 (Matriz 5×8) {#sensores}

#### 🟣 Explicación didáctica

El **TTP223** es un sensor capacitivo de un solo toque con 3 terminales: **VCC** (3.3 V), **GND** y **SIG** (salida HIGH al tocar). La organización en matriz permite leer 40 sensores con solo 8 pines de fila + CD4051.

```
  Conexión unitaria de un sensor TTP223:

  +3.3V_BUS ─────────────────── VCC (pin 1 del TTP223)
                                  │
                                 ╔╧╗ C_bypass: 100nF (junto a cada sensor)
                                 ╚╤╝
  GND_BUS ──────────────────── GND (pin 2 del TTP223)

  Salida SIG (pin 3) ──[R_serie: 100Ω]──► Pin de FILA del ESP32


  Diagrama de la Matriz Completa 5×8:

                  COL0   COL1   COL2 COL3 COL4   COL5  COL6  COL7
                   Y0     Y1     Y2   Y3   Y4     Y5    Y6    Y7
                    │      │      │   │    │      │     │     │
  FILA 0 (GPIO34) [S00]─[S01]─[S02]─[S03]─[S04]─[S05]─[S06]─[S07]
  FILA 1 (GPIO35) [S10]─[S11]─[S12]─[S13]─[S14]─[S15]─[S16]─[S17]
  FILA 2 (GPIO32) [S20]─[S21]─[S22]─[S23]─[S24]─[S25]─[S26]─[S27]
  FILA 3 (GPIO33) [S30]─[S31]─[S32]─[S33]─[S34]─[S35]─[S36]─[S37]
  FILA 4 (GPIO25) [S40]─[S41]─[S42]─[S43]─[S44]─[S45]─[S46]─[S47]


  Pull-down obligatorio para GPIO34 y GPIO35:

  GPIO34 ──┬──[R_pd0: 10kΩ]── GND_BUS
           └── (señales SIG Fila 0 via R 100Ω)

  GPIO35 ──┬──[R_pd1: 10kΩ]── GND_BUS
           └── (señales SIG Fila 1 via R 100Ω)
```

---

### 8. Bloque 6 — Tiras NeoPixel WS2812B {#neopixel}

#### 🔴 Explicación didáctica

Los **WS2812B** son LEDs RGB con controlador integrado. Se encadenan en serie usando un solo pin de datos del ESP32. En la **distribución serpenteante v3.0**, la tira sigue el recorrido zigzag por columnas sin necesidad de cortes.

```
  Línea de datos NeoPixel:

  GPIO26 (ESP32) ──[R1: 470Ω]──────────────────────► DIN (primer LED WS2812B)

  Encadenamiento serpenteante (v3.0):

  [Celda 0,0]─DOUT─►DIN─[Celda 1,0]─DOUT─►DIN─[Celda 2,0]─►..─[Celda 4,0]
     (Col0↓ baja completa)
        └─►DIN─[Celda 4,1]─DOUT─►DIN─[Celda 3,1]─►..─[Celda 0,1]
              (Col1↑ sube completa)
                 └─►DIN─[Celda 0,2]─►..─[Celda 4,2]
                      (Col2↓ baja completa)
                          ... y así hasta la columna 7


  Alimentación (TODAS las tiras en paralelo desde el bus):

  +5V_BUS ──┬──► VCC Tira Celda[0][0]
  [C8:1000µF] ├──► VCC Tira Celda[1][0]
  [C9:100nF]  │    ...
              └──► VCC Tira Celda[4][7]

  GND_BUS ──┬──► GND Tira Celda[0][0]
             │    ...
             └──► GND Tira Celda[4][7]

  ⚠️ NUNCA conectar las tiras en serie para la alimentación.
  ⚠️ Alimentar cada grupo de ≤80 LEDs con punto de inyección propio.
```

> 💡 **Ventaja del recorrido serpenteante:** Los puntos de doblez de la tira (al cambiar de columna) quedan físicamente accesibles para inyectar corriente al bus 5V, sin necesidad de tender cables largos desde una fuente central.

---

### 9. Bloque 7 — Pulsadores {#pulsadores}

#### ⚪ Explicación didáctica

Pulsadores normalmente abiertos (NA) con pull-down de 10 kΩ. En reposo el pin lee LOW; al presionar, lee HIGH. El firmware aplica debounce de 50 ms por software.

```
  BOTÓN INICIO (GPIO27)

  +3.3V_BUS ─── [PULSADOR SW1] ──► GPIO27 (ESP32)
                                        │
  GND_BUS ──────[R_pulldown: 10kΩ]──── ┘

  BOTÓN RESET (GPIO15) — Idéntico:

  +3.3V_BUS ─── [PULSADOR SW2] ──► GPIO15 (ESP32)
                                        │
  GND_BUS ──────[R_pulldown: 10kΩ]──── ┘
```

---

### 10. Diagrama de Conexiones Completo {#diagrama}

```
╔══════════════════════════════════════════════════════════════════════════════════════╗
║              DIAGRAMA COMPLETO DEL SISTEMA — JuegoCapacitivoNeoPixel v3.0          ║
╚══════════════════════════════════════════════════════════════════════════════════════╝

┌─────────────────────────────────────────────────────────────────────────────────────┐
│ BLOQUE ALIMENTACIÓN                                                                 │
│  [FUENTE 5V/15A]──(+)──[D1:1N5822]──┬──────────────────────────────── +5V_BUS       │
│                                     │                                               │
│                             [C1:1000µF][C2:100nF]                                   │
│                                     │                                               │
│  [FUENTE 5V/15A]──(-) ──────────────┴──────────────────────────────── GND_BUS       │
│                                                                                     │
│  +5V_BUS──[AMS1117-3.3]──┬──────────────────────────────────────── +3.3V_BUS        │
│  [C3:10µF][C4:100nF]──GND │ [C5:10µF][C6:100nF]──GND                                │
└───────────────────────────┼─────────────────────────────────────────────────────────┘
                            │
┌───────────────────────────┼─────────────────────────────────────────────────────────┐
│ ESP32 DevKit v1           │                                                         │
│                      +3.3V_BUS ──► pin 3V3                                          │
│                      +5V_BUS  ──► pin 5V                                            │
│                      GND_BUS  ──► pin GND                                           │
│                                                                                     │
│  pin GPIO4  ──[100Ω]──────────────────────────────► CD4051 pin 15 (A)               │
│  pin GPIO5  ──[100Ω]──────────────────────────────► CD4051 pin 14 (B)               │
│  pin GPIO18 ──[100Ω]──────────────────────────────► CD4051 pin 13 (C)               │
│                                                                                     │
│  pin GPIO34 ──┬──[10kΩ]──GND                                                        │
│               └──[100Ω]──► BUS_FILA_0                                               │
│  pin GPIO35 ──┬──[10kΩ]──GND                                                        │
│               └──[100Ω]──► BUS_FILA_1                                               │
│  pin GPIO32 ──[100Ω]──► BUS_FILA_2 (pull-down interno)                              │
│  pin GPIO33 ──[100Ω]──► BUS_FILA_3 (pull-down interno)                              │
│  pin GPIO25 ──[100Ω]──► BUS_FILA_4 (pull-down interno)                              │
│                                                                                     │
│  pin GPIO26 ──[470Ω]──────────────────────────────► DIN primer WS2812B              │
│                                                                                     │
│  pin GPIO27 ──────────────────────────────────────► BTN_INICIO ──[10kΩ]──GND        │
│  pin GPIO15 ──────────────────────────────────────► BTN_RESET  ──[10kΩ]──GND        │
└─────────────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────────────┐
│ CD4051 MULTIPLEXOR                                                                  │
│  +3.3V_BUS ──[C7:100nF]──► VDD (pin 16)                                             │
│  GND_BUS ──────────────► VSS (pin 9), VEE (pin 11), INH (pin 12)                    │
│  Y0 ──► Enable Columna 0   Y1 ──► Enable Columna 1                                  │
│  Y2 ──► Enable Columna 2   Y3 ──► Enable Columna 3                                  │
│  Y4 ──► Enable Columna 4   Y5 ──► Enable Columna 5                                  │
│  Y6 ──► Enable Columna 6   Y7 ──► Enable Columna 7                                  │
└─────────────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────────────┐
│ TIRAS LED WS2812B — distribución serpenteante v3.0 (40 tiras × 20 LEDs)             │
│                                                                                     │
│  Alimentación (TODAS en paralelo):                                                  │
│  +5V_BUS ──[C8:1000µF][C9:100nF]──► VCC todas las tiras                             │
│  GND_BUS ──────────────────────────► GND todas las tiras                            │
│                                                                                     │
│  Datos (encadenado serpenteante por columna):                                       │
│  GPIO26─[470Ω]─►DIN[0,0]─►[1,0]─►[2,0]─►[3,0]─►[4,0]  ← Col 0 baja                  │
│                                               └─►DIN[4,1]─►[3,1]─►[2,1]─►[1,1]─►[0,1]  ← Col 1 sube │
│                                                                 └─►DIN[0,2]─►...  ← Col 2 baja │
└─────────────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────────────┐
│ PULSADORES                                                                          │
│  +3.3V_BUS ──[SW1: BTN INICIO]──┬──► GPIO27 (ESP32)                                 │
│                                   └──[10kΩ]──► GND_BUS                              │
│  +3.3V_BUS ──[SW2: BTN RESET] ──┬──► GPIO15 (ESP32)                                 │
│                                   └──[10kΩ]──► GND_BUS                              │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

---

### 11. Tabla Resumen de Pines {#tabla}

```
╔══════════════════════╦══════════╦═══════════╦════════════════════════════════════════╗
║ Función              ║ GPIO     ║ Dirección ║ Notas de conexión                      ║
╠══════════════════════╬══════════╬═══════════╬════════════════════════════════════════╣
║ MUX — bit A          ║ GPIO 4   ║ SALIDA    ║ +100Ω serie → pin 15 CD4051            ║
║ MUX — bit B          ║ GPIO 5   ║ SALIDA    ║ +100Ω serie → pin 14 CD4051            ║
║ MUX — bit C          ║ GPIO 18  ║ SALIDA    ║ +100Ω serie → pin 13 CD4051            ║
╠══════════════════════╬══════════╬═══════════╬════════════════════════════════════════╣
║ Fila 0 (lectura)     ║ GPIO 34  ║ ENTRADA   ║ SOLO INPUT — 10kΩ pull-down EXTERNO    ║
║                      ║          ║           ║ + 100Ω serie en línea señal            ║
╠══════════════════════╬══════════╬═══════════╬════════════════════════════════════════╣
║ Fila 1 (lectura)     ║ GPIO 35  ║ ENTRADA   ║ SOLO INPUT — 10kΩ pull-down EXTERNO    ║
║                      ║          ║           ║ + 100Ω serie en línea señal            ║
╠══════════════════════╬══════════╬═══════════╬════════════════════════════════════════╣
║ Fila 2 (lectura)     ║ GPIO 32  ║ ENTRADA   ║ Pull-down INTERNO del ESP32            ║
║                      ║          ║           ║ + 100Ω serie en línea señal            ║
╠══════════════════════╬══════════╬═══════════╬════════════════════════════════════════╣
║ Fila 3 (lectura)     ║ GPIO 33  ║ ENTRADA   ║ Pull-down INTERNO del ESP32            ║
║                      ║          ║           ║ + 100Ω serie en línea señal            ║
╠══════════════════════╬══════════╬═══════════╬════════════════════════════════════════╣
║ Fila 4 (lectura)     ║ GPIO 25  ║ ENTRADA   ║ Pull-down INTERNO del ESP32            ║
║                      ║          ║           ║ + 100Ω serie en línea señal            ║
╠══════════════════════╬══════════╬═══════════╬════════════════════════════════════════╣
║ NeoPixel DIN         ║ GPIO 26  ║ SALIDA    ║ 470Ω serie → DIN primer WS2812B        ║
╠══════════════════════╬══════════╬═══════════╬════════════════════════════════════════╣
║ Botón INICIO         ║ GPIO 27  ║ ENTRADA   ║ 10kΩ pull-down externo a GND           ║
║                      ║          ║           ║ Pulsador a +3.3V                       ║
╠══════════════════════╬══════════╬═══════════╬════════════════════════════════════════╣
║ Botón RESET          ║ GPIO 15  ║ ENTRADA   ║ 10kΩ pull-down externo a GND           ║
║                      ║          ║           ║ Pulsador a +3.3V                       ║
╠══════════════════════╬══════════╬═══════════╬════════════════════════════════════════╣
║ Alimentación lógica  ║ VIN/5V   ║ ENTRADA   ║ Desde +5V_BUS (regulador DevKit)       ║
║ Referencia 3.3V      ║ 3V3      ║ SALIDA    ║ No usar como entrada                   ║
║ Masa                 ║ GND      ║ —         ║ Conectada a GND_BUS                    ║
╚══════════════════════╩══════════╩═══════════╩════════════════════════════════════════╝
```

---

### 12. Notas de Diseño Robusto frente al Ruido Eléctrico {#notas}

#### ⚡ Estrategias Anti-Ruido implementadas

##### 12.1 Separación de planos de masa

```
  CONCEPTO: "Estrella de tierra" o separación funcional

  GND_BUS principal
      │
      ├── GND_POTENCIA (LEDs WS2812B)          ← Alta corriente, mucho ruido
      │       └── Unir al GND_BUS en UN solo punto, cerca de la fuente
      │
      ├── GND_LÓGICA (ESP32, CD4051, sensores)  ← Baja corriente, sensible
      │       └── Unir al GND_BUS en UN solo punto, cerca del regulador 3.3V
      │
      └── Punto de unión único: en los terminales de la fuente de alimentación
```

##### 12.2 Capacitores de desacople — Resumen

| Ubicación | Capacitor | Valor | Propósito |
|-----------|-----------|-------|-----------|
| Entrada regulador AMS1117 | C3 | 10 µF electrolítico | Filtro BF en entrada |
| Entrada regulador AMS1117 | C4 | 100 nF cerámico | Filtro HF en entrada |
| Salida regulador AMS1117 | C5 | 10 µF electrolítico | Filtro BF en salida |
| Salida regulador AMS1117 | C6 | 100 nF cerámico | Filtro HF en salida |
| VCC del CD4051 | C7 | 100 nF cerámico | Desacople del IC |
| Primer LED WS2812B | C8 | 1000 µF / 10V | Reserva de energía para LEDs |
| Primer LED WS2812B | C9 | 100 nF cerámico | Filtro HF línea LEDs |
| Cada sensor TTP223 | C_bypass × 40 | 100 nF cerámico | Desacople individual |

##### 12.3 Resistores de protección y filtrado

```
  R = 470Ω en datos NeoPixel:
  ├── Limita la corriente en caso de cortocircuito transitorio
  ├── Junto con la capacidad parásita del cable, forma un filtro RC
  └── Protege el GPIO26 del ESP32

  R = 100Ω en control MUX (GPIO4, GPIO5, GPIO18):
  ├── Amortigua las oscilaciones (ringing) en líneas largas
  ├── Limita corriente de pico al cambiar estado lógico
  └── Reduce la velocidad de flancos → menos EMI

  R = 100Ω en líneas de fila (GPIO34, GPIO35, GPIO32, GPIO33, GPIO25):
  ├── Forma filtro RC con capacidad de entrada del GPIO
  ├── Protege el pin ante descargas electrostáticas (ESD)
  └── Atenúa ruido de alta frecuencia proveniente de los sensores

  R = 10kΩ pull-down en GPIO34, GPIO35, GPIO27, GPIO15:
  ├── Mantiene el pin en estado definido (LOW) cuando no hay señal
  └── Previene lecturas erráticas por ruido ambiental
```

##### 12.4 Buenas prácticas de PCB / cableado

```
  1. LONGITUD de cables de datos (NeoPixel):
     → Mantener el cable entre GPIO26 y el primer LED < 50 cm
     → Usar cable trenzado (par trenzado) con el GND para reducir EMI

  2. CONDENSADORES lo más CERCA POSIBLE de los ICs:
     → C7 (100nF) a menos de 5 mm del pin VCC del CD4051
     → C8 y C9 a menos de 5 cm del primer LED WS2812B

  3. RUTAS de alimentación de LEDs:
     → Usar cable de sección ≥ 2.5 mm² (AWG13) para 15A
     → Alimentar las tiras en los puntos de doblez (serpenteante)

  4. SEPARAR físicamente:
     → Cableado de datos alejado de cableado de potencia
     → Cruzar cables en 90° si deben cruzarse obligatoriamente

  5. TIERRA COMÚN única:
     → Todos los GND se unen en un solo punto (star ground)
     → El punto de unión es el terminal negativo de la fuente
```

---

### 📋 Lista de Verificación Final antes de Encender

```
  □ Verificar polaridad de todos los capacitores electrolíticos
  □ Confirmar que VCC del AMS1117 = +5V y OUT = +3.3V (medir con multímetro)
  □ Confirmar que los sensores TTP223 tienen VCC = +3.3V (NO 5V)
  □ Verificar que el primer LED WS2812B recibe +5V en VCC y GND en GND
  □ Confirmar resistor de 470Ω presente en la línea DIN del primer LED
  □ Confirmar capacitor de 1000µF junto al primer LED
  □ Verificar pull-down 10kΩ en GPIO34 y GPIO35
  □ Verificar pull-down 10kΩ en GPIO27 (BTN_INICIO) y GPIO15 (BTN_RESET)
  □ Confirmar que el pin INH del CD4051 está conectado a GND (no flotante)
  □ Verificar el recorrido físico de la tira: Col0↓ → Col1↑ → Col2↓ → ...
  □ Confirmar que los dobleces de la tira coinciden con fin de cada columna
  □ Medir corriente en reposo antes de conectar todos los LEDs
  □ La fuente debe ser de 5V / mínimo 15A con protección de sobrecorriente
```

---

> 🎓 **Nota pedagógica final:**
> Este diseño implementa múltiples capas de protección porque trabaja en un
> entorno desafiante: una fuente de alta corriente (15A), 800 LEDs que
> conmutan simultáneamente, 40 sensores capacitivos sensibles, y un
> microcontrolador ESP32 con WiFi activo. Cada resistor, capacitor y
> decisión de diseño tiene una razón específica documentada para facilitar
> el aprendizaje y la reproducibilidad del proyecto.
>
> La distribución serpenteante (v3.0) además simplifica el montaje físico:
> una sola tira continua recorre toda la grilla sin cortes, lo que reduce
> puntos de falla mecánica y facilita la inyección de corriente en los
> puntos de doblez naturales de la tira.

---

## 📜 Licencia

Este proyecto se distribuye bajo la licencia **MIT**. Podés usarlo, modificarlo y distribuirlo libremente con atribución a los autores originales.

```
MIT License — Copyright (c) 2026
```

---

## 👤 Autores

**Juego Capacitivo NeoPixel — ESP32** · versión 3.0.0 · 2026

- **Prof. Matías Aldana**
- **Ing. Diego P. Poletti**
- **Prof. Jonathan Garrido**
