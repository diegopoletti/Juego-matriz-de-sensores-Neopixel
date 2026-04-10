# Esquema Eléctrico - Juego Capacitivo NeoPixel ESP32

## 📋 Índice

1. [Visión General](#visión-general)
2. [Sistema de Alimentación](#sistema-de-alimentación)
3. [Esquema del Multiplexor CD4051](#esquema-del-multiplexor-cd4051)
4. [Matriz de Sensores TTP223](#matriz-de-sensores-ttp223)
5. [LEDs NeoPixel WS2812B](#leds-neopixel-ws2812b)
6. [Pulsadores de Control](#pulsadores-de-control)
7. [Asignación de Pines GPIO](#asignación-de-pines-gpio)
8. [Consideraciones de Diseño PCB](#consideraciones-de-diseño-pcb)
9. [Lista de Materiales (BOM)](#lista-de-materiales-bom)

---

## Visión General

### Componentes Principales

| Componente | Cantidad | Descripción |
|------------|----------|-------------|
| ESP32 DevKit v1 | 1 | Microcontrolador principal |
| CD4051 | 1 | Multiplexor analógico 8 canales |
| TTP223 | 30 | Sensores capacitivos (matriz 5×6) |
| WS2812B | 480 | LEDs RGB direccionables (30 tiras × 16 LEDs) |
| AMS1117-3.3 | 1 | Regulador de voltaje 3.3V |

### Requisitos de Alimentación

| Línea | Voltaje | Corriente Máxima | Fuente |
|-------|---------|------------------|--------|
| **5V (NeoPixel)** | 5.0V | ~9A (brillo 30%) | Fuente conmutada 5V/10A mínimo |
| **3.3V (ESP32)** | 3.3V | ~500mA | Regulador AMS1117 desde 5V |
| **GND** | 0V | - | Común a todas las líneas |

### ⚠️ Protecciones Implementadas

```
┌─────────────────────────────────────────────────────────────────┐
│  ⚡ PROTECCIÓN DE ENTRADAS DEL ESP32                            │
├─────────────────────────────────────────────────────────────────┤
│  ✓ Todos los sensores TTP223 alimentados a 3.3V                 │
│  ✓ Salidas TTP223: 0V (LOW) a 3.3V (HIGH) - SEGURO para ESP32  │
│  ✓ Resistencias pull-down 10kΩ externas en GPIO34 y GPIO35     │
│  ✓ Resistencias pull-down 10kΩ en pulsadores                   │
│  ✓ Sin señales de 5V en ninguna entrada del ESP32              │
└─────────────────────────────────────────────────────────────────┘
```

---

## Sistema de Alimentación

### Diagrama de Bloques

```
                        ┌──────────────────────────────────────────────────┐
                        │              FUENTE EXTERNA 5V/10A               │
                        └─────────────────────┬────────────────────────────┘
                                              │
                               ┌──────────────┴──────────────┐
                               │                             │
                               ▼                             ▼
              ┌────────────────────────────────┐   ┌─────────────────────┐
              │      LÍNEA 5V PRINCIPAL        │   │   LÍNEA 3.3V        │
              │   (Para LEDs NeoPixel)         │   │  (Para ESP32)       │
              └───────────────┬────────────────┘   └──────────┬──────────┘
                              │                               │
                    ┌─────────┴─────────┐          ┌──────────┴──────────┐
                    │   C1: 1000µF/10V  │          │   REGULADOR         │
                    │   Electrolítico   │          │   AMS1117-3.3       │
                    │   (Alimentación   │          │                     │
                    │    NeoPixel)      │          │   VIN ─── VOUT      │
                    └───────────────────┘          │    │       │        │
                                                   │   GND     3.3V     │
                                                   │    │       │        │
                                                   └────┼───────┼────────┘
                                                        │       │
                                              ┌─────────┴─┐   ┌─┴─────────┐
                                              │   GND     │   │ C2: 22µF  │
                                              │  Común    │   │ Tántalo   │
                                              └───────────┘   └───────────┘
```

### Esquema Detallado del Regulador 3.3V

```
                              +5V (DESDE FUENTE EXTERNA)
                                         │
                                         │
                              ┌──────────┴──────────┐
                              │                     │
                              │   ┌─────────────┐   │
                         ┌────┴───┤  C_IN       │   │
                         │    │   │  10µF/16V   │   │
                         │    │   │ Electrolít. │   │
                         │    │   └─────────────┘   │
                         │    │                     │
                         │    │   ┌─────────────┐   │
                         │    └───┤ VIN     VOUT├───┼───────────── +3.3V REGULADOS
                         │        │             │   │
                         │        │  AMS1117    │   │
                         │        │   -3.3      │   │
                         │        │             │   │
                         │        │    GND      │   │
                         │        └──────┬──────┘   │
                         │               │          │
                         │               │          │
                         │    ┌──────────┴───────┐  │   ┌─────────────┐
                         │    │   C_OUT          │  └───┤  C_OUT2     │
                         │    │   22µF/16V       │      │  100nF      │
                         │    │   Tántalo        │      │  Cerámico   │
                         │    │   (ESR bajo)     │      │  (Desacople)│
                         │    └──────────────────┘      └─────────────┘
                         │               │                     │
                         │               │                     │
                         └───────────────┴─────────────────────┘
                                         │
                                        GND
```

### Estimación de Consumo

| Componente | Cantidad | Consumo Unitario | Consumo Total |
|------------|----------|------------------|---------------|
| ESP32 (WiFi AP activo) | 1 | ~160mA | 160mA |
| CD4051 | 1 | ~1mA | 1mA |
| TTP223 | 30 | ~1.5mA | 45mA |
| NeoPixel (30% brillo) | 480 | ~18mA/LED × 0.3 | ~2.6A |
| **Total 5V** | - | - | **~2.8A** |
| **Total 3.3V** | - | - | **~210mA** |

---

## Esquema del Multiplexor CD4051

### Conexiones del CD4051

```
                              CD4051 (Multiplexor 8:1)
                            ┌───────────────────────────┐
                            │                           │
               +3.3V ───────┤ VCC (pin 16)              │
                            │                           │
                     Y0 ────┤ Y0 (pin 13)    NC (pin 8) ├─── NC
                            │                           │
                     Y1 ────┤ Y1 (pin 14)    C (pin 11) ├─── GPIO 18 (MUX_C)
                            │                           │
                     Y2 ────┤ Y2 (pin 15)    B (pin 10) ├─── GPIO 5  (MUX_B)
                            │                           │
                     Y3 ────┤ Y3 (pin 12)    A (pin 9)  ├─── GPIO 4  (MUX_A)
                            │                           │
                     Y4 ────┤ Y4 (pin 1)               │
                            │                           │
                     Y5 ────┤ Y5 (pin 5)               │
                            │                           │
                     Y6 ────┤ Y6 (pin 2)    INH (pin 6)├─── GND (Siempre activo)
                            │                           │
                     Y7 ────┤ Y7 (pin 4)     Z (pin 3)  ├─── NC (No usado)
                            │                           │
                      GND ──┤ VEE (pin 7)    GND (pin 8)├─── GND
                            │                           │
                            └───────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│  TABLA DE VERDAD CD4051                                             │
├─────────────────────────────────────────────────────────────────────┤
│  INH │ C (GPIO18) │ B (GPIO5) │ A (GPIO4) │ CANAL ACTIVO │ COLUMNA │
│──────┼────────────┼───────────┼───────────┼──────────────┼─────────│
│  0   │     0      │     0     │     0     │     Y0       │    0    │
│  0   │     0      │     0     │     1     │     Y1       │    1    │
│  0   │     0      │     1     │     0     │     Y2       │    2    │
│  0   │     0      │     1     │     1     │     Y3       │    3    │
│  0   │     1      │     0     │     0     │     Y4       │    4    │
│  0   │     1      │     0     │     1     │     Y5       │    5    │
│  0   │     1      │     1     │     0     │     Y6       │   NC    │
│  0   │     1      │     1     │     1     │     Y7       │   NC    │
│  1   │     X      │     X     │     X     │   NINGUNO    │   OFF   │
└─────────────────────────────────────────────────────────────────────┘
```

### Conexión de Alimentación al Multiplexor

```
                    +3.3V (Desde regulador AMS1117)
                           │
                  ┌────────┴────────┐
                  │   C_DES: 100nF  │
                  │   Cerámico      │
                  │   (Desacople)   │
                  └────────┬────────┘
                           │
         ┌─────────────────┼─────────────────┐
         │                 │                 │
         │                 │                 │
    ┌────┴────┐      ┌─────┴─────┐     ┌─────┴─────┐
    │ CD4051  │      │   TTP223  │     │   ESP32   │
    │  VCC    │      │   VCC     │     │   3.3V    │
    └─────────┘      └───────────┘     └───────────┘
         │                 │                 │
         └─────────────────┼─────────────────┘
                           │
                          GND
```

---

## Matriz de Sensores TTP223

### Diagrama de la Matriz 5×6

```
                           MATRIZ DE SENSORES TTP223
     ┌──────────────────────────────────────────────────────────────────┐
     │                                                                  │
     │     COL 0      COL 1      COL 2      COL 3      COL 4      COL 5│
     │        │          │          │          │          │          │  │
     │        Y0         Y1         Y2         Y3         Y4         Y5 │
     │        │          │          │          │          │          │  │
     │   ┌────┴────┐ ┌───┴────┐ ┌───┴────┐ ┌───┴────┐ ┌───┴────┐ ┌───┴────┐
     │   │ [0,0]   │ │ [0,1]  │ │ [0,2]  │ │ [0,3]  │ │ [0,4]  │ │ [0,5]  │
     │   │ TTP223  │ │ TTP223 │ │ TTP223 │ │ TTP223 │ │ TTP223 │ │ TTP223 │
FILA 0───┤ OUT─────┼─┼────────┼─┼────────┼─┼────────┼─┼────────┼─┼────────┼──► GPIO 34
     │   │         │ │        │ │        │ │        │ │        │ │        │
     │   └─────────┘ └────────┘ └────────┘ └────────┘ └────────┘ └────────┘
     │        │          │          │          │          │          │
     │   ┌────┴────┐ ┌───┴────┐ ┌───┴────┐ ┌───┴────┐ ┌───┴────┐ ┌───┴────┐
     │   │ [1,0]   │ │ [1,1]  │ │ [1,2]  │ │ [1,3]  │ │ [1,4]  │ │ [1,5]  │
     │   │ TTP223  │ │ TTP223 │ │ TTP223 │ │ TTP223 │ │ TTP223 │ │ TTP223 │
FILA 1───┤ OUT─────┼─┼────────┼─┼────────┼─┼────────┼─┼────────┼─┼────────┼──► GPIO 35
     │   │         │ │        │ │        │ │        │ │        │ │        │
     │   └─────────┘ └────────┘ └────────┘ └────────┘ └────────┘ └────────┘
     │        │          │          │          │          │          │
     │   ┌────┴────┐ ┌───┴────┐ ┌───┴────┐ ┌───┴────┐ ┌───┴────┐ ┌───┴────┐
     │   │ [2,0]   │ │ [2,1]  │ │ [2,2]  │ │ [2,3]  │ │ [2,4]  │ │ [2,5]  │
     │   │ TTP223  │ │ TTP223 │ │ TTP223 │ │ TTP223 │ │ TTP223 │ │ TTP223 │
FILA 2───┤ OUT─────┼─┼────────┼─┼────────┼─┼────────┼─┼────────┼─┼────────┼──► GPIO 32
     │   │         │ │        │ │        │ │        │ │        │ │        │
     │   └─────────┘ └────────┘ └────────┘ └────────┘ └────────┘ └────────┘
     │        │          │          │          │          │          │
     │   ┌────┴────┐ ┌───┴────┐ ┌───┴────┐ ┌───┴────┐ ┌───┴────┐ ┌───┴────┐
     │   │ [3,0]   │ │ [3,1]  │ │ [3,2]  │ │ [3,3]  │ │ [3,4]  │ │ [3,5]  │
     │   │ TTP223  │ │ TTP223 │ │ TTP223 │ │ TTP223 │ │ TTP223 │ │ TTP223 │
FILA 3───┤ OUT─────┼─┼────────┼─┼────────┼─┼────────┼─┼────────┼─┼────────┼──► GPIO 33
     │   │         │ │        │ │        │ │        │ │        │ │        │
     │   └─────────┘ └────────┘ └────────┘ └────────┘ └────────┘ └────────┘
     │        │          │          │          │          │          │
     │   ┌────┴────┐ ┌───┴────┐ ┌───┴────┐ ┌───┴────┐ ┌───┴────┐ ┌───┴────┐
     │   │ [4,0]   │ │ [4,1]  │ │ [4,2]  │ │ [4,3]  │ │ [4,4]  │ │ [4,5]  │
     │   │ TTP223  │ │ TTP223 │ │ TTP223 │ │ TTP223 │ │ TTP223 │ │ TTP223 │
FILA 4───┤ OUT─────┼─┼────────┼─┼────────┼─┼────────┼─┼────────┼─┼────────┼──► GPIO 25
     │   │         │ │        │ │        │ │        │ │        │ │        │
     │   └─────────┘ └────────┘ └────────┘ └────────┘ └────────┘ └────────┘
     │        │          │          │          │          │          │
     └────────┼──────────┼──────────┼──────────┼──────────┼──────────┼────────┘
              │          │          │          │          │          │
              └──────────┴──────────┴──────────┴──────────┴──────────┘
                                   │
                           SALIDA COMÚN
                         DEL CD4051 (Z)
                          (No usada)
```

### Conexión Individual de Cada TTP223

```
                    CONEXIÓN TÍPICA DE SENSOR TTP223
                    (Repetir para cada uno de los 30 sensores)

     +3.3V ────────────────────────────────────────────────────────
                │
                │
         ┌──────┴──────┐
         │   C1        │
         │   100nF     │    ◄── Capacitor de desacople
         │   Cerámico  │         (cerca del sensor)
         └──────┬──────┘
                │
         ┌──────┴──────┐
         │   TTP223    │
         │             │
         │  VCC    GND ├─── GND
         │             │
         │         OUT ├──────────────────────────────────► A FILA GPIO
         │             │                                    (con pull-down)
         │         Q   ├─── NC (No conectado)
         │             │
         └─────────────┘
         
         
     IMPORTANTE: Alimentar TTP223 con 3.3V, NO con 5V
     ─────────────────────────────────────────────────
     • Si se alimenta con 5V, la salida OUT será ~5V
     • 5V en GPIO del ESP32 = DAÑO PERMANENTE
     • Con 3.3V, OUT = 0V a 3.3V (SEGURO para ESP32)
```

### Circuito de Entrada con Pull-Down

```
                    ENTRADA GPIO CON PULL-DOWN EXTERNO
                    (Requerido para GPIO34 y GPIO35)

     +3.3V                                                         
        │                                                          
        │                                                          
   ┌────┴────┐                                                     
   │  10kΩ   │                                                     
   │ R_PULL  │                                                     
   └────┬────┘                                                     
        │                                                          
        ├─────────────────────────────────────────────► A GPIO (34-35)
        │                                                          
        │      ┌─────────────┐                                     
        │      │             │                                     
        └──────┤   TTP223    │                                     
               │    OUT      │                                     
               │             │                                     
               │    VCC  GND ├────── GND                           
               └─────────────┘                                     
                                                                    
     FUNCIÓN:                                                      
     • Cuando TTP223 NO detecta toque → OUT flotante              
     • R_PULL mantiene GPIO en LOW (0V)                            
     • Cuando TTP223 detecta toque → OUT = 3.3V                    
     • GPIO lee HIGH (3.3V)                                        


                    ENTRADA GPIO CON PULL-DOWN INTERNO
                    (Disponible para GPIO32, 33, 25)

                              GPIO (32, 33, 25)
                                    │
                                    │
        ┌─────────────┐             │
        │   TTP223    │             │
        │    OUT ─────┼─────────────┘
        │             │
        │    VCC  GND ├────── GND
        └─────────────┘
        
        NOTA: Configurar en código:
        pinMode(pin, INPUT_PULLDOWN);
```

---

## LEDs NeoPixel WS2812B

### Diagrama de Conexión General

```
                    CONEXIÓN DE TIRAS NEOPIXEL (480 LEDs)
                    
     +5V (FUENTE 10A) ────┬────────────────────────────────────────────
                          │
                  ┌───────┴───────┐
                  │   1000µF      │
                  │   Electrolít. │    ◄── Capacitor de filtro
                  │   10V mín.    │         (cerca del primer LED)
                  └───────┬───────┘
                          │
     ┌────────────────────┼────────────────────────────────────────┐
     │                    │                                        │
     │  NEOPIXEL 1        │  NEOPIXEL 2          NEOPIXEL 30       │
     │  (Celda 0,0)       │  (Celda 0,1)         (Celda 4,5)       │
     │  16 LEDs           │  16 LEDs             16 LEDs           │
     │                    │                                        │
     │  ┌──────────┐      │  ┌──────────┐       ┌──────────┐       │
     │  │ VDD  GND │      │  │ VDD  GND │       │ VDD  GND │       │
     │  │  │    │  │      │  │  │    │  │       │  │    │  │       │
     │  └──┼────┼──┘      │  └──┼────┼──┘       └──┼────┼──┘       │
     │     │    │         │     │    │             │    │          │
     │     │    │         │     │    │             │    │          │
     │  ┌──┴────┴──┐      │  ┌──┴────┴──┐      ┌──┴────┴──┐       │
     │  │ DIN  DOUT├──────┼──┤ DIN  DOUT├──...─┤ DIN  DOUT├── NC  │
     │  │          │      │  │          │      │          │       │
     │  └──────────┘      │  └──────────┘      └──────────┘       │
     │        ▲           │                                        │
     │        │           │                                        │
     └────────┼───────────┴────────────────────────────────────────┘
              │
              │    ┌─────────┐
              └────┤  470Ω   ├──────────────────────► GPIO 26
                   │ Resist. │
                   └─────────┘
                   
                   GND ──────────────────────────── GND COMÚN
```

### Detalle de Conexión al ESP32

```
                    INTERFAZ NEOPIXEL - ESP32

     ESP32                                          PRIMER LED WS2812B
     ┌─────────────────┐                           ┌─────────────────┐
     │                 │                           │                 │
     │         GPIO 26 ├───────────┬───────────────┤ DIN             │
     │                 │           │               │                 │
     │                 │     ┌─────┴─────┐         │ VDD ── +5V      │
     │                 │     │   470Ω    │         │                 │
     │                 │     │  Resistor │         │ GND ── GND      │
     │                 │     │  1/4W     │         │                 │
     │                 │     └───────────┘         │ DOUT ── A SIG.   │
     │                 │                           │                 │
     │            GND  ├───────────────────────────┤ GND             │
     │                 │                           │                 │
     └─────────────────┘                           └─────────────────┘


     PROTECCIONES:
     ───────────────────────────────────────────────────────────
     • 470Ω en serie en DIN: Protege contra picos de corriente
                            y mejora integridad de señal
     • 1000µF en VDD: Filtra transitorios de corriente
     • Brillo limitado al 30%: Reduce consumo a ~8.6A máx
```

### Mapa de Índices de LEDs

```
                    MAPEO DE LEDs POR CELDA
                    (16 LEDs por celda, 480 total)

     ┌────────────┬────────────┬────────────┬────────────┬────────────┬────────────┐
     │  Celda     │  Celda     │  Celda     │  Celda     │  Celda     │  Celda     │
     │  [0,0]     │  [0,1]     │  [0,2]     │  [0,3]     │  [0,4]     │  [0,5]     │
     │  LEDs      │  LEDs      │  LEDs      │  LEDs      │  LEDs      │  LEDs      │
     │  0-15      │  16-31     │  32-47     │  48-63     │  64-79     │  80-95     │
     ├────────────┼────────────┼────────────┼────────────┼────────────┼────────────┤
     │  Celda     │  Celda     │  Celda     │  Celda     │  Celda     │  Celda     │
     │  [1,0]     │  [1,1]     │  [1,2]     │  [1,3]     │  [1,4]     │  [1,5]     │
     │  LEDs      │  LEDs      │  LEDs      │  LEDs      │  LEDs      │  LEDs      │
     │  96-111    │  112-127   │  128-143   │  144-159   │  160-175   │  176-191   │
     ├────────────┼────────────┼────────────┼────────────┼────────────┼────────────┤
     │  Celda     │  Celda     │  Celda     │  Celda     │  Celda     │  Celda     │
     │  [2,0]     │  [2,1]     │  [2,2]     │  [2,3]     │  [2,4]     │  [2,5]     │
     │  LEDs      │  LEDs      │  LEDs      │  LEDs      │  LEDs      │  LEDs      │
     │  192-207   │  208-223   │  224-239   │  240-255   │  256-271   │  272-287   │
     ├────────────┼────────────┼────────────┼────────────┼────────────┼────────────┤
     │  Celda     │  Celda     │  Celda     │  Celda     │  Celda     │  Celda     │
     │  [3,0]     │  [3,1]     │  [3,2]     │  [3,3]     │  [3,4]     │  [3,5]     │
     │  LEDs      │  LEDs      │  LEDs      │  LEDs      │  LEDs      │  LEDs      │
     │  288-303   │  304-319   │  320-335   │  336-351   │  352-367   │  368-383   │
     ├────────────┼────────────┼────────────┼────────────┼────────────┼────────────┤
     │  Celda     │  Celda     │  Celda     │  Celda     │  Celda     │  Celda     │
     │  [4,0]     │  [4,1]     │  [4,2]     │  [4,3]     │  [4,4]     │  [4,5]     │
     │  LEDs      │  LEDs      │  LEDs      │  LEDs      │  LEDs      │  LEDs      │
     │  384-399   │  400-415   │  416-431   │  432-447   │  448-463   │  464-479   │
     └────────────┴────────────┴────────────┴────────────┴────────────┴────────────┘

     FÓRMULA: Índice_Base = (fila × 6 + columna) × 16
```

---

## Pulsadores de Control

### Diagrama de Conexión

```
                    PULSADORES DE INICIO Y RESET


     ┌───────────────────────────── PULSADOR INICIO ─────────────────────────────┐
     │                                                                           │
     │     +3.3V                                                                 │
     │        │                                                                   │
     │        │                                                                   │
     │   ┌────┴────┐                                                              │
     │   │  10kΩ   │     R_PULL (Pull-down externo)                              │
     │   │         │                                                              │
     │   └────┬────┘                                                              │
     │        │                                                                   │
     │        ├──────────────────────────────────────► GPIO 27                   │
     │        │                                                                   │
     │        │    ┌─────────────────────┐                                        │
     │        └────┤  PULSADOR NA        │                                        │
     │             │  (Normalmente       │                                        │
     │             │   Abierto)          │                                        │
     │             └──────────┬──────────┘                                        │
     │                        │                                                   │
     │                        │                                                   │
     │                       GND                                                  │
     │                                                                           │
     └───────────────────────────────────────────────────────────────────────────┘


     ┌───────────────────────────── PULSADOR RESET ──────────────────────────────┐
     │                                                                           │
     │     +3.3V                                                                 │
     │        │                                                                   │
     │        │                                                                   │
     │   ┌────┴────┐                                                              │
     │   │  10kΩ   │     R_PULL (Pull-down externo)                              │
     │   │         │                                                              │
     │   └────┬────┘                                                              │
     │        │                                                                   │
     │        ├──────────────────────────────────────► GPIO 15                   │
     │        │                                                                   │
     │        │    ┌─────────────────────┐                                        │
     │        └────┤  PULSADOR NA        │                                        │
     │             │  (Normalmente       │                                        │
     │             │   Abierto)          │                                        │
     │             └──────────┬──────────┘                                        │
     │                        │                                                   │
     │                        │                                                   │
     │                       GND                                                  │
     │                                                                           │
     └───────────────────────────────────────────────────────────────────────────┘


     ESTADOS:
     ─────────────────────────────────────────────────
     • Pulsador NO presionado: GPIO lee LOW (0V)
     • Pulsador PRESIONADO: GPIO lee HIGH (3.3V)
     
     DEBOUNCE SOFTWARE: 50ms (definido en código)
```

### Protección contra Rebote

```
                    CIRCUITO OPCIONAL DE DEBOUNCE HARDWARE

                                      +3.3V
                                         │
                                         │
                                    ┌────┴────┐
                                    │  10kΩ   │
                                    │ R_PULL  │
                                    └────┬────┘
                                         │
     PULSADOR ─────────┬─────────────────┼────────────────────► GPIO
                        │                 │
                        │            ┌────┴────┐
                        │            │  100nF  │
                        │            │ C_DEBOU │    ◄── Filtra rebotes
                        │            └────┬────┘
                        │                 │
                        └─────────────────┘
                                          │
                                         GND

     NOTA: El código implementa debounce por software (50ms),
           pero este circuito puede mejorar la robusteza.
```

---

## Asignación de Pines GPIO

### Tabla Completa de Conexiones

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                        TABLA DE ASIGNACIÓN DE PINES GPIO                        │
├───────────────────┬───────────┬─────────────────────────────────────────────────┤
│ Función           │ GPIO      │ Notas                                           │
├───────────────────┼───────────┼─────────────────────────────────────────────────┤
│ MUX A (bit 0)     │ GPIO 4    │ Salida digital - Control CD4051                │
│ MUX B (bit 1)     │ GPIO 5    │ Salida digital - Control CD4051                │
│ MUX C (bit 2)     │ GPIO 18   │ Salida digital - Control CD4051                │
├───────────────────┼───────────┼─────────────────────────────────────────────────┤
│ Fila 0            │ GPIO 34   │ ⚠️ Solo entrada, SIN pull-up/down interno      │
│ Fila 1            │ GPIO 35   │ ⚠️ Solo entrada, SIN pull-up/down interno      │
│ Fila 2            │ GPIO 32   │ Entrada con pull-down interno disponible       │
│ Fila 3            │ GPIO 33   │ Entrada con pull-down interno disponible       │
│ Fila 4            │ GPIO 25   │ Entrada con pull-down interno disponible       │
├───────────────────┼───────────┼─────────────────────────────────────────────────┤
│ NeoPixel DIN      │ GPIO 26   │ Salida digital, resistor 470Ω en serie         │
├───────────────────┼───────────┼─────────────────────────────────────────────────┤
│ Pulsador INICIO   │ GPIO 27   │ Entrada, pull-down 10kΩ externo                │
│ Pulsador RESET    │ GPIO 15   │ Entrada, pull-down 10kΩ externo                │
├───────────────────┼───────────┼─────────────────────────────────────────────────┤
│ Alimentación      │ VIN/5V    │ Entrada 5V (desde USB o regulador externo)     │
│ Alimentación      │ 3.3V      │ Salida 3.3V regulada (máx 500mA)               │
│ Tierra            │ GND       │ Referencia común de tierra                     │
└───────────────────┴───────────┴─────────────────────────────────────────────────┘
```

### Diagrama de Conexiones del ESP32

```
                              ESP32 DevKit v1
                    ┌───────────────────────────────────┐
                    │                                   │
           ┌────────┤ 3.3V                              │
           │        │                                   │
     +3.3V ┼────────┤                                   │
     (reg) │        │                                   │
           │   ┌────┤ GPIO 34 (Fila 0)  ◄── TTP223 [*,0]│
           │   │    │                                   │
           │   │ ┌──┤ GPIO 35 (Fila 1)  ◄── TTP223 [*,1]│
           │   │ │  │                                   │
           │   │ │ ┌┤ GPIO 32 (Fila 2)  ◄── TTP223 [*,2]│
           │   │ │ ││                                   │
           │   │ │ │├┤ GPIO 33 (Fila 3)  ◄── TTP223 [*,3]│
           │   │ │ │││                                   │
           │   │ │ ││├┤ GPIO 25 (Fila 4)  ◄── TTP223 [*,4]│
           │   │ │ ││││                                   │
           │   │ │ │││├┤ GPIO 26 (NeoPixel) ──► WS2812B DIN│
           │   │ │ │││││                                   │
           │   │ │ ││││├┤ GPIO 27 (BTN_INICIO)            │
           │   │ │ ││││││                                   │
           │   │ │ │││││├┤ GPIO 15 (BTN_RESET)             │
           │   │ │ │││││││                                   │
           │   │ │ ││││││├┤ GPIO 4  (MUX_A) ───► CD4051 A  │
           │   │ │ ││││││││                                   │
           │   │ │ │││││││├┤ GPIO 5  (MUX_B) ───► CD4051 B  │
           │   │ │ │││││││││                                   │
           │   │ │ ││││││││├┤ GPIO 18 (MUX_C) ───► CD4051 C │
           │   │ │ ││││││││││                                   │
     +5V ──┼───┼─┼─┼─┼─┼─┼─┼─┼─┼─┤ VIN                               │
     (USB) │   │ │ ││││││││││                                   │
           │   │ │ │││││││││├┤ GND ──────────────── TIERRA COMÚN │
           │   │ │ ││││││││││                                   │
           │   │ │ │││││││││└───────────────────────────────────┤
           │   │ │ │││││││││                 │                   │
           │   │ │ │││││││││                 │                   │
           └───┴─┴─┴┴┴┴┴┴┴┴┴─────────────────┴───────────────────┘
               │ │ ││││││││
               │ │ │││││││└── A sensores (3.3V)
               │ │ ││││││└─── A NeoPixel (5V)
               │ │ │││││└──── A pull-downs (GND)
               │ │ ││││└───── A CD4051
               │ │ │││└────── A pulsadores
               │ │ ││└─────── 
               └─┴─┴┴──────── A otras señales
```

### Pines con Restricciones Especiales

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│  ⚠️ GPIO CON RESTRICCIONES ESPECIALES                                          │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│  GPIO 34, 35, 36, 39:                                                          │
│  ─────────────────────                                                          │
│  • Solo entradas (GPIO input only)                                              │
│  • NO tienen resistencias pull-up/pull-down internas                           │
│  • REQUIEREN resistencia externa pull-down de 10kΩ a GND                       │
│  • Voltaje máximo: 3.3V (NO conectar 5V)                                        │
│                                                                                 │
│  GPIO 6-11:                                                                     │
│  ────────────                                                                   │
│  • Conectados a flash SPI interna                                               │
│  • NO USAR para conexiones externas                                            │
│                                                                                 │
│  GPIO 0:                                                                        │
│  ─────────                                                                      │
│  • Conectado al botón BOOT                                                      │
│  • Usar con precaución (afecta el modo de arranque)                            │
│                                                                                 │
│  GPIO 2:                                                                        │
│  ─────────                                                                      │
│  • LED integrado en la placa                                                    │
│  • Debe estar en estado alto durante el arranque                               │
│                                                                                 │
│  GPIO 12:                                                                       │
│  ──────────                                                                     │
│  • Selecciona voltaje de flash durante arranque                                │
│  • Mantener en LOW para flash de 3.3V                                          │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## Consideraciones de Diseño PCB

### Disposición Recomendada

```
                    LAYOUT RECOMENDADO DE PCB (Vista Superior)

     ┌─────────────────────────────────────────────────────────────────────────┐
     │                                                                         │
     │   ┌─────────────────────────────────────────────────────────────────┐   │
     │   │                     MATRIZ DE SENSORES                          │   │
     │   │                     TTP223 (5×6 = 30 unidades)                  │   │
     │   │                                                                  │   │
     │   │   ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐                     │   │
     │   │   │    │ │    │ │    │ │    │ │    │ │    │                     │   │
     │   │   └────┘ └────┘ └────┘ └────┘ └────┘ └────┘                     │   │
     │   │   ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐                     │   │
     │   │   │    │ │    │ │    │ │    │ │    │ │    │   NEOPIXEL         │   │
     │   │   └────┘ └────┘ └────┘ └────┘ └────┘ └────┘   BAJO CADA         │   │
     │   │   ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐   SENSOR           │   │
     │   │   │    │ │    │ │    │ │    │ │    │ │    │   (16 LEDs)        │   │
     │   │   └────┘ └────┘ └────┘ └────┘ └────┘ └────┘                     │   │
     │   │   ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐                     │   │
     │   │   │    │ │    │ │    │ │    │ │    │ │    │                     │   │
     │   │   └────┘ └────┘ └────┘ └────┘ └────┘ └────┘                     │   │
     │   │   ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐                     │   │
     │   │   │    │ │    │ │    │ │    │ │    │ │    │                     │   │
     │   │   └────┘ └────┘ └────┘ └────┘ └────┘ └────┘                     │   │
     │   │                                                                  │   │
     │   └─────────────────────────────────────────────────────────────────┘   │
     │                                                                         │
     │   ┌──────────────┐  ┌──────────────┐  ┌────────────────────────────┐    │
     │   │    CD4051    │  │   AMS1117    │  │          ESP32             │    │
     │   │  Multiplexor │  │   Regulador  │  │        DevKit v1           │    │
     │   │              │  │    3.3V      │  │                            │    │
     │   └──────────────┘  └──────────────┘  └────────────────────────────┘    │
     │                                                                         │
     │   ┌──────────────┐  ┌──────────────┐                                   │
     │   │   PULSADOR   │  │   PULSADOR   │    ┌────────────────────────┐     │
     │   │   INICIO     │  │    RESET     │    │   CAPACITORES          │     │
     │   │              │  │              │    │   1000µF + desacoples  │     │
     │   └──────────────┘  └──────────────┘    └────────────────────────┘     │
     │                                                                         │
     │   ┌──────────────────────────────────────────────────────────────────┐  │
     │   │                        BORNE DE ALIMENTACIÓN                     │  │
     │   │                        5V/10A  +  GND                            │  │
     │   └──────────────────────────────────────────────────────────────────┘  │
     │                                                                         │
     └─────────────────────────────────────────────────────────────────────────┘
```

### Protección contra Ruido EMI/EMC

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│  MEDIDAS DE PROTECCIÓN CONTRA RUIDO ELÉCTRICO                                  │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│  1. DESENTRAMADO DE TIERRA (Ground Plane)                                       │
│     • Usar plano de tierra continuo en capa inferior de PCB                    │
│     • Conectar todos los GND a este plano                                      │
│                                                                                 │
│  2. CAPACITORES DE DESACOPLE                                                    │
│     • 100nF cerámico junto a cada TTP223 (VCC-GND)                             │
│     • 100nF cerámico junto a CD4051 (VCC-GND)                                  │
│     • 100µF electrolítico + 100nF cerámico en entrada de ESP32                │
│     • 1000µF electrolítico en línea de NeoPixel                                │
│                                                                                 │
│  3. FILTRADO DE LÍNEAS DE SEÑAL                                                 │
│     • Resistencia 470Ω en línea DIN NeoPixel (reduce reflexiones)              │
│     • Mantener trazas de señal cortas (<10cm si es posible)                    │
│                                                                                 │
│  4. SEPARACIÓN DE ALIMENTACIONES                                                │
│     • Trazas separadas para 5V (NeoPixel) y 3.3V (lógica)                     │
│     • Punto único de conexión a tierra (star ground)                           │
│                                                                                 │
│  5. PROTECCIÓN DE ENTRADAS                                                      │
│     • GPIO34/35: Pull-down 10kΩ externo                                        │
│     • Pulsadores: Pull-down 10kΩ + capacitor 100nF opcional                    │
│                                                                                 │
│  6. RECOMENDACIONES DE ROUTING                                                  │
│     • Trazas de alimentación: mínimo 0.5mm de ancho                            │
│     • Trazas de tierra: máximo ancho posible                                    │
│     • Evitar trazas paralelas largas de señal                                   │
│     • Ruteo en "diente de sierra" para señales sensibles                       │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### Capacitores de Desacople - Ubicación

```
                    UBICACIÓN DE CAPACITORES DE DESACOPLE

     ┌─────────────────────────────────────────────────────────────────────────┐
     │                                                                          │
     │   CADA TTP223:                                                          │
     │   ┌────────────────┐                                                    │
     │   │ 100nF cerámico │◄── Lo más cerca posible del pin VCC               │
     │   │  (0805 o 1206) │     Conectar directamente VCC-GND                  │
     │   └────────────────┘                                                    │
     │                                                                          │
     │   CD4051:                                                               │
     │   ┌────────────────┐                                                    │
     │   │ 100nF cerámico │◄── Entre pin 16 (VCC) y pin 8 (GND)               │
     │   └────────────────┘                                                    │
     │                                                                          │
     │   ESP32:                                                                │
     │   ┌────────────────┐  ┌────────────────┐                                │
     │   │ 100µF electrol.│  │  100nF cerámico│◄── Cerca del pin 3.3V         │
     │   └────────────────┘  └────────────────┘                                │
     │                                                                          │
     │   AMS1117-3.3:                                                          │
     │   ┌────────────────┐  ┌────────────────┐                                │
     │   │  10µF entrada  │  │  22µF salida   │◄── Tántalo o cerámico         │
     │   └────────────────┘  └────────────────┘    (ESR bajo)                  │
     │                                                                          │
     │   LÍNEA NEOPIXEL:                                                       │
     │   ┌────────────────┐                                                    │
     │   │ 1000µF electrol│◄── En la alimentación del primer LED              │
     │   │   10V mínimo   │     Reduce transitorios de corriente              │
     │   └────────────────┘                                                    │
     │                                                                          │
     └─────────────────────────────────────────────────────────────────────────┘
```

---

## Lista de Materiales (BOM)

### Componentes Principales

| Ref | Componente | Especificación | Cantidad | Observaciones |
|-----|------------|----------------|----------|---------------|
| U1 | ESP32 DevKit v1 | 30 pines, 4MB Flash | 1 | Cualquier variante compatible |
| U2 | CD4051 | Multiplexor 8:1 analógico | 1 | DIP-16 o SOIC-16 |
| U3 | AMS1117-3.3 | Regulador LDO 3.3V | 1 | Alternativa: LM1117-3.3 |
| U4-U33 | TTP223 | Sensor capacitivo táctil | 30 | SOT-23-6 |
| LED1-480 | WS2812B | LED RGB direccionable | 480 | 30 tiras × 16 LEDs |

### Componentes Pasivos

| Ref | Componente | Especificación | Cantidad | Observaciones |
|-----|------------|----------------|----------|---------------|
| C1 | Capacitor electrolítico | 1000µF, 10V | 1 | Filtrado NeoPixel |
| C2 | Capacitor tántalo | 22µF, 10V | 1 | Salida AMS1117 |
| C3 | Capacitor electrolítico | 10µF, 16V | 1 | Entrada AMS1117 |
| C4-C33 | Capacitor cerámico | 100nF, 50V, 0805 | 30 | Desacople TTP223 |
| C34 | Capacitor cerámico | 100nF, 50V | 1 | Desacople CD4051 |
| C35 | Capacitor electrolítico | 100µF, 10V | 1 | Desacople ESP32 |
| R1 | Resistor | 470Ω, 1/4W | 1 | Línea DIN NeoPixel |
| R2-R31 | Resistor | 10kΩ, 1/4W | 30 | Pull-down GPIO34/35 y otros |

### Componentes Electromecánicos

| Ref | Componente | Especificación | Cantidad | Observaciones |
|-----|------------|----------------|----------|---------------|
| SW1 | Pulsador NA | Táctil 6×6mm | 1 | Inicio |
| SW2 | Pulsador NA | Táctil 6×6mm | 1 | Reset |
| J1 | Bornero | 2 pines, 5.08mm | 1 | Alimentación 5V |
| J2 | Conector header | 2×15 pines, 2.54mm | 1 | Para ESP32 DevKit |

### Fuente de Alimentación

| Componente | Especificación | Observaciones |
|------------|----------------|---------------|
| Fuente conmutada | 5V DC, mínimo 10A | Para NeoPixel + lógica |
| Alternativa | 5V DC, 12A o más | Si se desea brillo >30% |

### Resumen de Costos Estimados

| Categoría | Cantidad | Costo Estimado (USD) |
|-----------|----------|---------------------|
| Microcontrolador | 1 | $8-15 |
| Multiplexor + Regulador | 2 | $2-4 |
| Sensores TTP223 | 30 | $6-12 |
| LEDs NeoPixel (480) | 30 tiras | $40-80 |
| Componentes pasivos | Varios | $5-10 |
| Pulsadores y conectores | Varios | $2-5 |
| PCB (doble cara) | 1 | $10-30 |
| Fuente de alimentación | 1 | $15-25 |
| **TOTAL** | - | **$88-181** |

---

## Notas Finales

### Verificaciones Previas al Primer Encendido

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│  ✓ CHECKLIST DE VERIFICACIÓN                                                   │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│  □ Verificar continuidad de tierras (todos los GND conectados entre sí)       │
│                                                                                 │
│  □ Verificar que NO hay cortocircuitos entre 5V y GND                         │
│                                                                                 │
│  □ Verificar que NO hay cortocircuitos entre 3.3V y GND                       │
│                                                                                 │
│  □ Verificar que los sensores TTP223 están alimentados a 3.3V (NO 5V)        │
│                                                                                 │
│  □ Verificar resistencias pull-down en GPIO34 y GPIO35                        │
│                                                                                 │
│  □ Verificar resistor 470Ω en línea DIN de NeoPixel                           │
│                                                                                 │
│  □ Verificar capacitor 1000µF en alimentación de NeoPixel                     │
│                                                                                 │
│  □ Verificar polaridad de capacitores electrolíticos                          │
│                                                                                 │
│  □ Medir voltaje de salida del regulador 3.3V antes de conectar ESP32        │
│                                                                                 │
│  □ Conectar fuente de 5V y verificar voltajes correctos                       │
│                                                                                 │
│  □ Cargar código de prueba simple para verificar NeoPixel                     │
│                                                                                 │
│  □ Probar cada sensor TTP223 individualmente                                  │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### Pines del ESP32 DevKit v1

```
                    ESQUEMA DE PINES - ESP32 DevKit v1 (30 pines)
                    
                    ┌─────────────────────────────────┐
                    │               │                 │
               EN  ─┤               │                 ├─ D23
               VP  ─┤               │                 ├─ D22
               VN  ─┤               │       │         ├─ TX0
               D34 ─┤   FRENTE      │  USB  │         ├─ RX0
               D35 ─┤               │       │         ├─ D21
               D32 ─┤               │                 ├─ GND
               D33 ─┤               │                 ├─ D19
               D25 ─┤               │                 ├─ D18  ─── MUX_C
               D26 ─┤               │                 ├─ D5   ─── MUX_B
               D27 ─┤               │                 ├─ TX2
               D14 ─┤               │                 ├─ RX2
               D12 ─┤               │                 ├─ D4   ─── MUX_A
               D13 ─┤               │                 ├─ D2
               GND ─┤               │                 ├─ D15  ─── BTN_RESET
               VIN ─┤               │                 ├─ GND
                    │               │                 ├─ 3V3
                    └─────────────────────────────────┘

     GPIO USADOS EN ESTE PROYECTO:
     ─────────────────────────────
     • GPIO 4  ─── MUX_A
     • GPIO 5  ─── MUX_B
     • GPIO 18 ─── MUX_C
     • GPIO 34 ─── FILA 0 (pull-down externo)
     • GPIO 35 ─── FILA 1 (pull-down externo)
     • GPIO 32 ─── FILA 2
     • GPIO 33 ─── FILA 3
     • GPIO 25 ─── FILA 4
     • GPIO 26 ─── NEOPIXEL DIN
     • GPIO 27 ─── BTN_INICIO
     • GPIO 15 ─── BTN_RESET
```

---

## Contacto y Soporte

Este documento fue generado como parte del análisis del código `JuegoCapacitivoNeoPixel.ino`.

**Referencias:**
- Documentación ESP32: https://docs.espressif.com/projects/esp-idf/
- Librería Adafruit NeoPixel: https://github.com/adafruit/Adafruit_NeoPixel
- Datasheet TTP223: https://www.tontek.com.tw/uploads/product/TP223.pdf
- Datasheet CD4051: Texas Instruments CD4051B
- Datasheet WS2812B: Worldsemi WS2812B

---

*Documento generado para uso didáctico - Proyecto ESP32 NeoPixel Game*
*Versión del esquema: 1.0*
*Fecha: 2026*
