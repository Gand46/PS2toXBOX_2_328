# Validación de resistencias (esquema Dimitris) y cómo aplican aquí

Fecha: 2026-03-30

## Contexto
El esquema de Dimitris incluye una red de resistencias en USB y en la interfaz PS2. Esta nota explica su función y cómo trasladarla de forma segura a esta implementación.

## Resistencias del lado USB (V-USB)

### R5/R6 = 68Ω (en serie con D+ y D-)
**Función:**
- Limitar picos de corriente en flancos.
- Mejorar integridad de señal/EMI en enlaces USB low-speed por software (V-USB).

**Estado recomendado en este proyecto:**
- **Recomendadas** también para estabilidad eléctrica.

---

### R2 = 2.2k (pull-up de línea USB)
**Función en el esquema de Dimitris:**
- Implementar la polarización de la línea para identificación low-speed (según topología V-USB usada en su circuito).

**Cómo aplicarlo aquí:**
- Es **obligatorio** respetar la topología de pull-up que exige V-USB para la línea configurada.
- El valor exacto depende del esquema final (con/sin clamp por diodos y tensión efectiva vista por la línea).

---

### R1 = 1M (bias/descarga)
**Función:**
- Proveer camino de descarga/bias débil para estabilizar estado en reposo en esa topología concreta.

**Estado recomendado en este proyecto:**
- **Opcional**, útil si reproduces exactamente el circuito de Dimitris.

## Resistencias del lado PS2

### R4 = 4.7k (pull-up en DATA)
**Función:**
- Mantener línea DATA en nivel alto estable cuando el bus está en reposo.

**Estado recomendado en este proyecto:**
- **Recomendable** si tu cableado/controlador lo necesita para estabilidad.

---

### R3 = 2.2k (serie en CMD en esquema Dimitris)
**Función:**
- Limitación de corriente y adaptación en su topología concreta.

**Estado recomendado en este proyecto:**
- Si usas **level shifter dedicado 5V→3.3V** para `CMD/ATT/CLK`, esta resistencia serie puede no ser necesaria.
- Si no hay level shifter, esa resistencia **no reemplaza** una conversión de nivel robusta.

## Conclusión práctica para este repositorio
1. Mantener enfoque principal: **ATmega328P a 5V + PS2 a 3.3V + level shifter en salidas a PS2**.
2. En USB V-USB, conservar red recomendada de serie (68Ω) y pull-up conforme a la topología validada.
3. Evitar asumir que una sola resistencia en serie equivale a level shifting seguro.
