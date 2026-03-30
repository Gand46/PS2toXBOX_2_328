# PS2toXBOX_328P (implementación propia)

Firmware para adaptar mandos PS1/PS2 a **Original Xbox** usando **ATmega328P @16MHz** + **V-USB**.

## Objetivos de esta versión
- Estructura de código más limpia y mantenible.
- Bucle principal no bloqueante con watchdog activo.
- Manejo de enumeración Xbox más robusto (timeouts acotados).
- Traducción PS2→XID separada y con reporte neutral seguro ante fallos de lectura.
- Soporte de rumble Xbox→PS2 (motores izquierdo/derecho).

## Estructura
- `src/main.c`: USB/XID, enumeración y loop principal.
- `src/ps2.c`: SPI/PS2 + traducción de entradas.
- `src/descriptors.c`: descriptores USB/XID.
- `include/*.h`: estructuras y contratos públicos.
- `usbdrv/*`: stack V-USB requerido por ATmega328P.

## Hardware (ATmega328P)
- D+ USB → PD2 (INT0)
- D- USB → PD4
- PS2 ATT/CS → PB2
- PS2 CMD/MOSI → PB3
- PS2 DAT/MISO → PB4
- PS2 CLK/SCK → PB5

> Importante: respetar niveles eléctricos del mando PS2 (3.3V en señales del control) usando conversión de nivel cuando corresponda.

## Build (AVR-GCC)
Este proyecto conserva `Makefile` para compilar/flashear con toolchain AVR.

Ejemplo:
```bash
make
make flash
```

## Notas técnicas
- Se mantiene endpoint de entrada de 20 bytes para reporte XID (compatibilidad OG Xbox).
- En ausencia de lectura válida del control, se envía reporte neutral (evita bloqueos/reinicios forzados).


## Referencia de hardware usada en este proyecto
- MCU objetivo: **ATmega328P versión 5V / 16MHz**.
- Interfaz PS2: se usa **level shifter 5V→3.3V** para las señales que salen del 328P hacia el control PS2 (`CMD`, `ATT`, `CLK`).
- Señal `DAT` (PS2→MCU) debe leerse dentro de nivel seguro para el ATmega328P y compartir tierra común.

## Pinout detallado

### Señales PS2 hacia ATmega328P
| Señal PS2 | Pin ATmega328P | Pin Arduino UNO/Nano | Dirección | Nota |
|---|---:|---:|---|---|
| `DAT` | `PB4` | `D12 (MISO)` | PS2 → MCU | Datos del mando |
| `CMD` | `PB3` | `D11 (MOSI)` | MCU → PS2 | Comandos al mando |
| `ATT` | `PB2` | `D10 (SS)` | MCU → PS2 | Chip select (activo en bajo) |
| `CLK` | `PB5` | `D13 (SCK)` | MCU → PS2 | Reloj SPI (~500kHz) |
| `VCC` | `3.3V` | `3V3` | — | Alimentación mando PS2 |
| `GND` | `GND` | `GND` | — | Tierra común |

### Señales USB/Xbox en V-USB
| Señal USB | Pin ATmega328P | Pin Arduino UNO/Nano | Nota |
|---|---:|---:|---|
| `D+` | `PD2` | `D2 / INT0` | Línea obligatoria de interrupción para V-USB |
| `D-` | `PD4` | `D4` | Línea de datos USB low-speed |
| `VBUS 5V` | `VCC` | `5V` | Alimentación del microcontrolador |
| `GND` | `GND` | `GND` | Tierra común con consola |

### Recomendaciones eléctricas
- Usa adaptación de nivel para señales del mando PS2 si tu placa está a 5V.
- Mantén cableado corto en `D+`/`D-` y en líneas SPI del PS2.
- Comparte siempre `GND` entre Xbox, MCU y mando PS2.
