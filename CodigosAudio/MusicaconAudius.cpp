/*
 * ESP32 - Test de Comunicación con Servidor (SIN AUDIO)
 * Basado en código que funciona con ngrok
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ========== CONFIGURACIÓN WIFI ==========
const char* WIFI_SSID = "Juanma";
const char* WIFI_PASS = "38814831";

// ========== CONFIGURACIÓN SERVIDOR ==========
const char* BASE_URL = "http://choreal-kalel-directed.ngrok-free.dev";
const char* DEVICE_ID = "esp32-01";

// ========== VARIABLES GLOBALES ==========
unsigned long tPoll = 0;
const unsigned long POLL_INTERVAL = 2000; // Polling cada 2 segundos

String currentTrackId = "";
String currentTitle = "";
String currentArtist = "";
String currentStreamUrl = "";
bool isPlaying = false;
int currentVolume = 80;

// ========== DECLARACIONES DE FUNCIONES ==========
void handlePlayMusic(JsonObject cmd);
void handleStop();
void handlePause();
void handleResume();
void handleVolume(JsonObject cmd);
void handleSeek(JsonObject cmd);
void handleFrase(JsonObject cmd);
void handleConversacion(JsonObject cmd);
void handleLoro(JsonObject cmd);

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n======================================");
  Serial.println("ESP32 - Test Servidor (Sin Audio)");
  Serial.println("======================================\n");

  // --- WiFi ---
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  Serial.print("🔌 Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  
  Serial.printf(" ✅\n📡 IP: %s  RSSI: %d dBm\n", 
                WiFi.localIP().toString().c_str(), 
                WiFi.RSSI());

  // --- Registro de dispositivo ---
  {
    HTTPClient http;
    http.setReuse(true);
    http.setTimeout(8000);
    String url = String(BASE_URL) + "/esp32/register";
    
    if (http.begin(url)) {
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      
      String postData = "device_id=" + String(DEVICE_ID) + 
                       "&nombre=ESP32-Test" +
                       "&ubicacion=Lab";
      
      int code = http.POST(postData);
      String resp = http.getString();
      http.end();
      
      Serial.printf("📝 Registro -> %d %s\n", code, resp.c_str());
    } else {
      Serial.println("❌ No se pudo iniciar HTTP para registro");
    }
  }

  Serial.println("\n📱 Sistema listo. Esperando comandos...\n");
  Serial.println("💡 TIP: Ahora podés enviar comandos desde Flutter o Postman");
  Serial.println("    Ejemplo: POST /control/musica_buscar");
  Serial.println("             POST /control/musica_reproducir\n");
}

// ========== LOOP PRINCIPAL ==========
void loop() {
  // Verificar WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ WiFi desconectado, reconectando...");
    WiFi.reconnect();
    delay(1000);
    return;
  }

  // Poll cada 2 segundos
  if (millis() - tPoll > POLL_INTERVAL) {
    tPoll = millis();

    HTTPClient http;
    http.setReuse(true);          // keep-alive
    http.setTimeout(5000);
    String url = String(BASE_URL) + "/esp32/poll/" + String(DEVICE_ID);

    if (!http.begin(url)) {
      Serial.println("⚠️ Poll: begin() falló");
      return;
    }

    int code = http.GET();
    
    if (code == HTTP_CODE_OK) {
      // Parseo directo desde el stream (maneja chunked)
      DynamicJsonDocument doc(4096);
      DeserializationError e = deserializeJson(doc, http.getStream());
      http.end();

      if (e) {
        Serial.printf("❌ JSON poll error: %s\n", e.c_str());
      } else {
        JsonArray comandos = doc["comandos"].as<JsonArray>();
        
        if (!comandos.isNull() && comandos.size() > 0) {
          Serial.println("\n==================================================");
          Serial.printf("📨 Recibidos %d comandos nuevos\n", comandos.size());
          Serial.println("==================================================");
          
          for (JsonObject cmd : comandos) {
            String tipo = cmd["tipo"].as<String>();
            
            Serial.println("\n🎯 Tipo: " + tipo);
            
            if (tipo == "reproducir_musica") {
              handlePlayMusic(cmd);
            } 
            else if (tipo == "musica_detener") {
              handleStop();
            }
            else if (tipo == "musica_pausa") {
              handlePause();
            }
            else if (tipo == "musica_continuar") {
              handleResume();
            }
            else if (tipo == "musica_volumen") {
              handleVolume(cmd);
            }
            else if (tipo == "musica_seek") {
              handleSeek(cmd);
            }
            else if (tipo == "reproducir_frase") {
              handleFrase(cmd);
            }
            else if (tipo == "reproducir_conversacion") {
              handleConversacion(cmd);
            }
            else if (tipo == "reproducir_loro") {
              handleLoro(cmd);
            }
            else {
              Serial.println("⚠️ Comando desconocido: " + tipo);
            }
            
            Serial.println("--------------------------------------------------");
          }
        }
      }
    } else if (code > 0) {
      Serial.printf("⚠️ Poll -> %d\n", code);
      http.end();
    } else {
      Serial.printf("⚠️ Poll error: %d\n", code);
      http.end();
    }
  }

  delay(10);
}

// ========== HANDLERS DE COMANDOS ==========

void handlePlayMusic(JsonObject cmd) {
  currentStreamUrl = cmd["url"].as<String>();
  currentTitle = cmd["titulo"].as<String>();
  currentArtist = cmd["artista"].as<String>();
  
  Serial.println("🎵 REPRODUCIR MÚSICA:");
  Serial.println("   📝 Título: " + currentTitle);
  Serial.println("   🎤 Artista: " + currentArtist);
  Serial.println("   🔗 URL: " + currentStreamUrl);
  
  isPlaying = true;
  
  Serial.println("✅ [SIMULADO] Reproducción iniciada");
  Serial.println("💡 Cuando agregues la librería de audio, aquí se reproducirá realmente");
}

void handleStop() {
  Serial.println("⏹️ DETENER REPRODUCCIÓN");
  isPlaying = false;
  currentTitle = "";
  currentArtist = "";
  currentStreamUrl = "";
  Serial.println("✅ [SIMULADO] Reproducción detenida");
}

void handlePause() {
  Serial.println("⏸️ PAUSAR REPRODUCCIÓN");
  Serial.println("✅ [SIMULADO] Reproducción pausada");
}

void handleResume() {
  Serial.println("▶️ REANUDAR REPRODUCCIÓN");
  Serial.println("✅ [SIMULADO] Reproducción reanudada");
}

void handleVolume(JsonObject cmd) {
  int volume = cmd["volume"].as<int>();
  currentVolume = volume;
  
  Serial.printf("🔊 AJUSTAR VOLUMEN: %d%%\n", volume);
  Serial.println("✅ [SIMULADO] Volumen ajustado");
}

void handleSeek(JsonObject cmd) {
  int position_ms = cmd["position_ms"].as<int>();
  int position_sec = position_ms / 1000;
  
  Serial.printf("⏩ BUSCAR POSICIÓN: %d segundos\n", position_sec);
  Serial.println("✅ [SIMULADO] Posición ajustada");
}

void handleFrase(JsonObject cmd) {
  String audio_id = cmd["audio_id"].as<String>();
  String nombre = cmd["nombre"].as<String>();
  
  Serial.println("💬 REPRODUCIR FRASE:");
  Serial.println("   📝 Nombre: " + nombre);
  Serial.println("   🆔 Audio ID: " + audio_id);
  Serial.println("✅ [SIMULADO] Frase reproducida");
}

void handleConversacion(JsonObject cmd) {
  String audio_id = cmd["audio_id"].as<String>();
  String texto_usuario = cmd["texto_usuario"].as<String>();
  String texto_robot = cmd["texto_robot"].as<String>();
  
  Serial.println("🤖 CONVERSACIÓN:");
  Serial.println("   👤 Usuario dijo: " + texto_usuario);
  Serial.println("   🤖 Robot responde: " + texto_robot);
  Serial.println("   🆔 Audio ID: " + audio_id);
  Serial.println("✅ [SIMULADO] Respuesta reproducida");
}

void handleLoro(JsonObject cmd) {
  String audio_id = cmd["audio_id"].as<String>();
  String texto = cmd["texto"].as<String>();
  String efecto = cmd["efecto"].as<String>();
  
  Serial.println("🦜 MODO LORO:");
  Serial.println("   📝 Texto: " + texto);
  Serial.println("   🎚️ Efecto: " + efecto);
  Serial.println("   🆔 Audio ID: " + audio_id);
  Serial.println("✅ [SIMULADO] Texto repetido con efecto");
}