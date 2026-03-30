# PS2toXBOX_328P (ATmega328P + V-USB)

Implementación para adaptar mandos PS1/PS2 a Original Xbox (XID) usando ATmega328P.

## Estado de compilación
- **Versión de compilación:** ver `VERSION` y `build/build_version.txt`.
- **Artefacto principal:** `build/PS2toXBOX_328P.hex`.
- **Artefacto versionado:** `build/PS2toXBOX_328P_<VERSION>.hex`.

## Arquitectura del firmware
- `src/main.c`: manejo USB/XID (`usbFunctionSetup`, `usbFunctionWrite`), temporizador 10ms para ventana de enumeración y bucle principal no bloqueante.
- `src/ps2.c`: interfaz SPI con mando PS2, lectura de estados, traducción de botones/ejes y configuración de rumble.
- `src/descriptors.c`: descriptors USB/XID requeridos por OG Xbox.
- `include/*.h`: estructuras de reportes y mapping.
- `usbdrv/*`: stack V-USB (control endpoint + endpoint de interrupción para reportes XID).

## Referencia de hardware usada
- MCU objetivo: **ATmega328P versión 5V / 16MHz**.
- Se usa **level shifter 5V→3.3V** en señales de salida del MCU hacia PS2: `CMD`, `ATT`, `CLK`.
- `DAT` (PS2→MCU) debe entrar a nivel seguro y con tierra común.

## Pinout detallado

### PS2 ↔ ATmega328P
| Señal PS2 | Pin AVR | Pin Arduino UNO/Nano | Dirección | Observación |
|---|---:|---:|---|---|
| `DAT` | `PB4` | `D12/MISO` | PS2 → MCU | Lectura de datos del mando |
| `CMD` | `PB3` | `D11/MOSI` | MCU → PS2 | Requiere level shifter |
| `ATT` | `PB2` | `D10/SS` | MCU → PS2 | Requiere level shifter |
| `CLK` | `PB5` | `D13/SCK` | MCU → PS2 | Requiere level shifter |
| `VCC` | `3.3V` | `3V3` | — | Alimentación lógica PS2 |
| `GND` | `GND` | `GND` | — | Tierra común |

### USB (Xbox cable) ↔ ATmega328P (V-USB)
| Señal USB | Pin AVR | Pin Arduino UNO/Nano | Observación |
|---|---:|---:|---|
| `D+` | `PD2` | `D2/INT0` | Línea de interrupción obligatoria |
| `D-` | `PD4` | `D4` | Línea de datos LS |
| `5V` | `VCC` | `5V` | Alimentación MCU |
| `GND` | `GND` | `GND` | Tierra común |

## Toolchain y dependencias
- `gcc-avr`
- `avr-libc`
- `binutils-avr`
- `avrdude` (para flasheo)

## Compilación
```bash
make clean
make hex
make artifact
```

El target `artifact` genera:
- `build/PS2toXBOX_328P.hex`
- `build/PS2toXBOX_328P_<VERSION>.hex`
- `build/build_version.txt`
- `build/build_size.txt`
- `build/build_timestamp_utc.txt`

## Flasheo
Editar `AVRDUDE` en `Makefile` según tu programador y luego ejecutar:
```bash
make flash
```
