# Auditoría de lógica y voltajes (referencia: dimitris-lagos)

Fecha de auditoría: 2026-03-30

## Objetivo
Validar que la implementación `PS2toXBOX_328P` no introduzca inconsistencias eléctricas o de lógica respecto a la referencia `dimitris-lagos/PlayStation-to-XBox-controller-adapter`.

## Hallazgos de referencia (dimitris-lagos)
1. MCU objetivo trabajando a **5V/16MHz** con V-USB.
2. En USB D+/D- se advierte que zeners de alta capacitancia pueden degradar/romper comunicación.
3. Flujo XID usa endpoint/reportes de 20 bytes y secuencia de enumeración con temporización.
4. El enlace PS2 se maneja por SPI a ~500kHz.

## Validación sobre este proyecto

### A) Lógica firmware
- `src/main.c` mantiene la misma idea base de enumeración acotada y loop no bloqueante con `usbPoll()` + watchdog.
- `src/ps2.c` mantiene SPI y traducción PS2→XID compatibles con reporte de 20 bytes.
- No se detectaron contradicciones funcionales críticas con la referencia.

### B) Voltajes y niveles lógicos
- Este proyecto define explícitamente: **ATmega328P a 5V/16MHz**.
- Para PS2, se define salida de señales MCU→PS2 (`CMD`, `ATT`, `CLK`) con **level shifter 5V→3.3V**.
- `DAT` desde PS2 (3.3V) hacia AVR 5V es eléctricamente válido para entrada digital de ATmega328P (margen de nivel alto suficiente), manteniendo tierra común.
- PS2 debe alimentarse a **3.3V**.

### C) Riesgos residuales y control
- Riesgo #1: cableado largo/ruidoso en líneas USB o SPI.
  - Mitigación: cables cortos, tierra sólida y ruta limpia.
- Riesgo #2: uso de diodos USB inadecuados (alta capacitancia).
  - Mitigación: evitar zeners lentos/alta C en D+/D- o validar componentes equivalentes de baja C.
- Riesgo #3: alimentar mando PS2 a 5V.
  - Mitigación: mantener rail PS2 en 3.3V.

## Conclusión de auditoría
Con la configuración documentada actual (**MCU 5V + level shifter en salidas a PS2 + PS2 a 3.3V**), la implementación es coherente con la lógica del proyecto de referencia y no presenta inconsistencia eléctrica crítica identificada en revisión documental/código.
