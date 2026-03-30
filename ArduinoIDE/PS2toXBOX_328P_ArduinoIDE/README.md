# Proyecto Arduino IDE: PS2toXBOX_328P_ArduinoIDE

Este directorio contiene una versión `.ino` preparada para compilar desde Arduino IDE, usando fuentes locales de V-USB y del adaptador PS2.

## Contenido
- `PS2toXBOX_328P_ArduinoIDE.ino`: entrada principal del sketch.
- `src/`: módulos C (`ps2.c`, `descriptors.c`).
- `include/`: headers del proyecto.
- `usbdrv/`: stack V-USB integrado.

## Dependencias / librerías necesarias
No se requiere instalar librerías externas desde Library Manager para la lógica principal porque el código se incluye localmente.

Sí necesitas tener instalado en Arduino IDE:
- Core AVR (placas Arduino AVR).
- Herramientas de compilación AVR del IDE.

## Configuración recomendada en Arduino IDE
- Board: **Arduino Nano** o **Arduino Uno**.
- Processor: **ATmega328P (5V, 16MHz)**.
- Clock: 16 MHz.

## Importante (niveles lógicos)
Usar **level shifter 5V→3.3V** en señales de salida hacia PS2 (`CMD`, `ATT`, `CLK`).

## Limitación práctica
Aunque el sketch está organizado para Arduino IDE, esta implementación usa V-USB y timing sensible; para builds de producción se recomienda validar también con el flujo `Makefile` en `PS2toXBOX_328P/`.
