## Conexiones ESP32 VSPI con ILI9341 (240x320) + XPT2046

| Función              | Pin módulo ILI9341/XPT2046 | Pin ESP32 (VSPI) |
|----------------------|----------------------------|------------------|
| VCC (Alimentación)   | VCC                        | 3.3V             |
| GND (Tierra)         | GND                        | GND              |
| CS (TFT Chip Select) | CS                         | GPIO5            |
| RESET                | RESET                      | GPIO4            |
| DC (Data/Command)    | DC                         | GPIO21           |
| MOSI (SPI)           | MOSI                       | GPIO23           |
| MISO (SPI)           | MISO                       | GPIO19           |
| SCK (SPI Clock)      | SCK                        | GPIO18           |
| LED (Backlight)      | LED                        | 3.3V             |
| T_CS (Touch CS)      | T_CS                       | GPIO2            |
| T_IRQ (Touch IRQ)    | IRQ (opcional)             | No conectado / GPIO27 recomendado |

> ⚠️ Nota: GPIO2 se usa como T_CS en este ejemplo. Si tienes problemas de arranque, usa otro pin (ej. GPIO15 o GPIO27).
