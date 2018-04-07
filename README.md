# ldr-reader

Light Dependent Resistor (CdS) reader

[Typical circuit](circuit.png)

Above circuit diagram is a low cost way of detecting ambient light using a GPIO pin. It works by setting the GPIO pin to output to drain the capacitor, then set the GPIO to input to let the capacitor charge. The amount of time needed to charge the capacitor is determined by the resistance of the CdS sensor.

This program reads the CdS sensor and debounces the sensor measurement. It has configurable hysteresis thresholds to prevent excessive state transitions. When a state transition occurs, it can be configured to perform multiple actions like set output GPIO pins and run a command.
