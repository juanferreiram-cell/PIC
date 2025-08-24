/*
  ESP32 + ILI9341 (SPI) + XPT2046 (touch)
  Pines según tu diagrama (VSPI) y T_CS en GPIO2.
  Toca la pantalla para cambiar el color de fondo.
*/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

// ========= PINES (ESP32 VSPI) =========
// ILI9341
#define TFT_CS   5      // CS → GPIO5
#define TFT_DC   21     // DC → GPIO21  (como indicas)
#define TFT_RST  4      // RST → GPIO4
// SPI VSPI por defecto: MOSI=23, MISO=19, SCK=18 (no hace falta redefinir)

// XPT2046 (touch)
#define TOUCH_CS   2    // T_CS → GPIO2 (como usas)
#define TOUCH_IRQ  -1   // sin IRQ (si lo tienes, pon su GPIO, ej. 27)

// (Opcional) retroiluminación si la conectas a un GPIO con PWM
//#define TFT_BL   32

// ========= OBJETOS =========
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
// Si NO usas IRQ:
XPT2046_Touchscreen ts(TOUCH_CS);      
// Si usas IRQ, comenta la línea de arriba y descomenta esta:
// XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

// ========= CALIBRACIÓN RÁPIDA (ajústala si hace falta) =========
#define TS_MINX 200
#define TS_MINY 200
#define TS_MAXX 3800
#define TS_MAXY 3800

// Colores de ejemplo
uint16_t colors[] = {
  ILI9341_BLACK, ILI9341_RED, ILI9341_GREEN, ILI9341_BLUE,
  ILI9341_CYAN, ILI9341_MAGENTA, ILI9341_YELLOW, ILI9341_WHITE, ILI9341_ORANGE
};
const uint8_t NUM_COLORS = sizeof(colors)/sizeof(colors[0]);
uint8_t colorIndex = 0;

bool touchedLast = false;
unsigned long lastTouchTime = 0;
const unsigned long debounceMs = 150;

void setup() {
  Serial.begin(115200);

  // Por claridad, forzamos VSPI (los pines por defecto ya son 23/19/18)
  SPI.begin(18, 19, 23, TFT_CS);  // SCK, MISO, MOSI, SS

  tft.begin();
  tft.setRotation(1);          // Landscape (cambia si te conviene)
  tft.fillScreen(colors[colorIndex]);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 10);
  tft.println("Toca para cambiar color");

  // Iniciar touch en el mismo bus SPI
  if (!ts.begin(SPI)) {
    Serial.println("No se detecta XPT2046. Revisa conexiones.");
  }
  // Si tienes IRQ, puedes configurar el pin:
  if (TOUCH_IRQ >= 0) pinMode(TOUCH_IRQ, INPUT);

  // (Opcional) retroiluminación por PWM
  // pinMode(TFT_BL, OUTPUT);
  // ledcAttachPin(TFT_BL, 0); ledcSetup(0, 20000, 8); ledcWrite(0, 255); // brillo máx
}

void loop() {
  bool isTouched = ts.touched();

  if (isTouched && !touchedLast && (millis() - lastTouchTime > debounceMs)) {
    lastTouchTime = millis();
    TS_Point p = ts.getPoint();  // lectura cruda

    // Mapeo para rotation=1 (landscape)
    int16_t x = map(p.y, TS_MINY, TS_MAXY, 0, tft.width());
    int16_t y = map(p.x, TS_MINX, TS_MAXX, 0, tft.height());

    if (x >= 0 && x < tft.width() && y >= 0 && y < tft.height()) {
      colorIndex = (colorIndex + 1) % NUM_COLORS;
      tft.fillScreen(colors[colorIndex]);

      tft.setCursor(10, 10);
      tft.setTextSize(2);
      tft.setTextColor((colors[colorIndex] == ILI9341_BLACK) ? ILI9341_WHITE : ILI9341_BLACK);
      tft.print("Color #");
      tft.print(colorIndex);
    }

    // (Útil para calibrar: descomenta para ver crudos)
    // Serial.printf("raw x=%d y=%d z=%d -> map x=%d y=%d\n", p.x, p.y, p.z, x, y);
  }

  touchedLast = isTouched;
}
