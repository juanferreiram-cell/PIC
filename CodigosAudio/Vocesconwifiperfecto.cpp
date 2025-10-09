/****************************************************
 * Robot NAO - Cliente ESP32 (WAV por streaming)
 * - Poll de comandos (200 ms, HTTP/1.1 keep-alive)
 * - Reproduce /esp32/audio_raw/{id} directo por red
 * - Salida por DAC interno: GPIO25 (L) y GPIO26 (R)
 ****************************************************/
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ===== Audio (ESP8266Audio) =====
#include <AudioFileSourceHTTPStream.h>   // streaming HTTP
// #include <AudioFileSourceHTTPSStream.h> // si alguna vez usás https
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2SNoDAC.h>         // DAC interno ESP32 (GPIO25/26)

// Para habilitar ambos DAC internos (25 y 26)
extern "C" {
  #include "driver/i2s.h"
  #include "driver/dac.h"
}

// ====== CONFIG WIFI / SERVER (tal cual pediste) ======
const char* WIFI_SSID = "Juanma";
const char* WIFI_PASS = "38814831";  // <- comillas cerradas
const char* BASE_URL  = "http://choreal-kalel-directed.ngrok-free.dev";
const char* DEVICE_ID = "esp32_1";

// ====== Audio objects ======
AudioGeneratorWAV*          wav       = nullptr;
AudioFileSourceHTTPStream*  file_http = nullptr;
// AudioFileSourceHTTPSStream* file_https = nullptr; // si usás https
AudioOutputI2SNoDAC*        out       = nullptr;

// ====== Helpers ======
void stopAudio() {
  if (wav)        { wav->stop(); delete wav; wav = nullptr; }
  if (file_http)  { delete file_http; file_http = nullptr; }
  // if (file_https) { delete file_https; file_https = nullptr; }
  if (out)        { delete out; out = nullptr; }
}

// Fuerza ambos canales del DAC interno (25 y 26)
static inline void enableBothDACChannels() {
  i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
}

// Reproduce WAV por streaming HTTP -> DAC interno
bool playWavStream(const String& url) {
  stopAudio();

  // Fuente HTTP con reconexión (HTTP/1.1 por defecto, keep-alive)
  file_http = new AudioFileSourceHTTPStream(url.c_str());
  // NO llamar useHTTP10(); // eso forzaría HTTP/1.0
  file_http->SetReconnect(3, 200);  // tries=3, delay=200ms

  // Salida por DAC interno, duplicando a L/R
  out = new AudioOutputI2SNoDAC();
  out->SetOutputModeMono(true);   // mono a ambos canales
  out->SetGain(0.6);              // menor ganancia = menos clip/ruido

  // Decodificador WAV
  wav = new AudioGeneratorWAV();
  if (!wav->begin(file_http, out)) {
    Serial.println("❌ WAV begin (stream) falló");
    stopAudio();
    return false;
  }

  // Asegurar ambos DAC habilitados
  enableBothDACChannels();

  Serial.println("▶️ Streaming audio...");
  while (wav->isRunning()) {
    if (!wav->loop()) break;
    delay(1);
  }
  Serial.println("✅ Fin de reproducción (stream)");

  stopAudio();
  return true;
}

bool confirmPlayback(const String& audio_id, const String& status) {
  HTTPClient http;
  http.setReuse(true);
  http.setTimeout(8000);
  String url = String(BASE_URL) + "/esp32/confirmar/" + DEVICE_ID;

  if (!http.begin(url)) return false;
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String body = "audio_id=" + audio_id + "&status=" + status;
  int code = http.POST(body);
  http.end();
  Serial.printf("Confirmar %s -> %d\n", status.c_str(), code);
  return (code >= 200 && code < 300);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nRobot NAO - Cliente ESP32 (Streaming + DAC interno L=GPIO25 / R=GPIO26)");

  // --- WiFi ---
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(250); Serial.print("."); }
  Serial.printf(" ✅ IP: %s  RSSI: %d dBm\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());

  // --- Registro de dispositivo ---
  {
    HTTPClient http;
    http.setReuse(true);
    http.setTimeout(8000);
    String url = String(BASE_URL) + "/esp32/register";
    if (http.begin(url)) {
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      int code = http.POST(String("device_id=") + DEVICE_ID);
      String resp = http.getString();
      http.end();
      Serial.printf("Registro -> %d %s\n", code, resp.c_str());
    } else {
      Serial.println("No se pudo iniciar HTTP para registro");
    }
  }
}

unsigned long tPoll = 0;

void loop() {
  // Poll cada 200 ms para baja latencia
  if (millis() - tPoll > 200) {
    tPoll = millis();

    HTTPClient http;
    http.setReuse(true);          // intentar keep-alive
    http.setTimeout(5000);        // poll rápido
    String url = String(BASE_URL) + "/esp32/poll/" + DEVICE_ID;

    if (!http.begin(url)) {
      Serial.println("Poll: begin() falló");
      return;
    }

    int code = http.GET();
    if (code == HTTP_CODE_OK) {
      // Parseo directo desde el stream (maneja chunked)
      DynamicJsonDocument doc(2048);
      DeserializationError e = deserializeJson(doc, http.getStream());
      http.end();

      if (e) {
        Serial.printf("JSON poll error: %s\n", e.c_str());
      } else {
        JsonArray cmds = doc["comandos"].as<JsonArray>();
        if (!cmds.isNull() && cmds.size() > 0) {
          JsonObject c = cmds[0];
          const char* tipo     = c["tipo"]     | "";
          const char* audio_id = c["audio_id"] | "";

          Serial.printf("CMD: %s audio_id=%s\n", tipo, audio_id);

          if (audio_id && *audio_id) {
            String wavURL = String(BASE_URL) + "/esp32/audio_raw/" + audio_id;

            bool played = playWavStream(wavURL);

            // Confirmar al server (success/error)
            confirmPlayback(audio_id, played ? "success" : "error");
          }
        }
      }
    } else {
      Serial.printf("Poll -> %d\n", code);
      http.end();
    }
  }

  // Mantener audio corriendo si algo quedó en background (seguro)
  if (wav && wav->isRunning()) {
    if (!wav->loop()) {
      stopAudio();
    }
  }

  delay(1);
}
