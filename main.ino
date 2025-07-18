// Define this to print debug output to Serial
#define BLYNK_PRINT Serial // Tetap bisa digunakan untuk Serial, atau dihapus

// Universal includes
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#define BOT_TELEGRAM "7764599473:AAEmbBdVUvgBaYNA-MxrZGDvpOT8A2-fktU"
#define ADMIN_CHAT_ID "7123768604" 

// --- WI-FI CREDENTIALS ---
char home_ssid[] = "Mechatronics";
char home_pass[] = "khususmeka";

// --- SENSOR & AP CONFIGURATION ---
#define DHTPIN 19
#define DHTTYPE DHT11
const char* ap_ssid = "DHT11_Sensor_AP";
const char* ap_pass = "12345678";

// --- GLOBAL OBJECTS ---
WiFiClientSecure securedClient;
UniversalTelegramBot bot(BOT_TELEGRAM, securedClient);
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2); 
WebServer server(80);

// --- PENGGANTI BLYNKTIMER ---
unsigned long previousMillis = 0;
const long interval = 2000; // Interval pembacaan sensor (2 detik)

// Variabel untuk Telegram
int botRequestDelay = 1000; // Cek pesan baru setiap 1 detik
unsigned long lastTimeBotRan; // Waktu terakhir bot cek pesan

// Global variables to store sensor readings
float temperature = 0.0;
float humidity = 0.0;

// --- FUNGSI UNTUK MENANGANI PERINTAH DARI USER ---
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;

    if (text == "/check") {
      String message = "✅ *Laporan Status Terkini*\n\n";
      message += "🌡️ *Suhu*: " + String(temperature, 1) + " °C\n";
      message += "💧 *Kelembapan*: " + String(humidity, 1) + " %\n\n";
      message += "_Data diambil secara real-time._";
      
      bot.sendMessage(chat_id, message, "Markdown");
    }
  }
}

// Fungsi untuk mengirim notifikasi suhu tinggi ke admin
void sendHighTempAlert() {
  String message = "🔥 *Peringatan Suhu Tinggi!* 🔥\n\nSuhu saat ini adalah *" + String(temperature) + " °C*, melebihi batas maksimum (30 °C). Harap segera diperiksa!";
  
  if (bot.sendMessage(ADMIN_CHAT_ID, message, "Markdown")) {
    Serial.println("Telegram alert sent successfully!");
  } else {
    Serial.println("Failed to send Telegram alert.");
  }
}

// --- FUNGSI PEMBACAAN SENSOR ---
void sendSensorData() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sensor Error");
    return;
  }
  
  humidity = h;
  temperature = t;

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" *C, Humidity: ");
  Serial.println(humidity);

  // Cek suhu dan kirim notifikasi jika melebihi batas
  if (temperature >= 30.0) {
    Serial.println("Suhu tinggi terdeteksi! Mengirim notifikasi...");
    sendHighTempAlert();
  }

  // Update tampilan LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Suhu: ");
  lcd.print(temperature, 2);
  lcd.print((char)223);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("Lembap: ");
  lcd.print(humidity, 2);
  lcd.print(" %");
}

// --- WEB PAGE HTML ---
String buildHtmlPage() {
  String html = "<!DOCTYPE html><html lang='en'>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Monitoring Suhu dan Kelembapan</title>";
  html += "<meta http-equiv='refresh' content='1'>"; 
  html += "<style>";
  html += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #f0f2f5; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }";
  html += ".container { background-color: #ffffff; padding: 2rem 3rem; border-radius: 12px; box-shadow: 0 4px 20px rgba(0,0,0,0.1); text-align: center; }";
  html += "h1 { color: #333; margin-bottom: 2rem; }";
  html += ".card-container { display: flex; gap: 2rem; justify-content: center; flex-wrap: wrap; }";
  html += ".card { background-color: #f8f9fa; padding: 1.5rem; border-radius: 10px; border: 1px solid #dee2e6; width: 150px; }";
  html += ".card h2 { margin-top: 0; color: #495057; font-size: 1.2rem; }";
  html += ".card .value { font-size: 2.5rem; font-weight: bold; color: #007bff; }";
  html += ".card .unit { font-size: 1rem; color: #6c757d; }";
  html += "footer { margin-top: 2rem; font-size: 0.8rem; color: #adb5bd; }";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class='container'>";
  html += "<h1>Monitoring Suhu dan Kelembapan</h1>";
  html += "<div class='card-container'>";
  html += "<div class='card'><h2>Suhu</h2><p class='value'>" + String(temperature, 1) + "<span class='unit'> &deg;C</span></p></div>";
  html += "<div class='card'><h2>Kelembapan</h2><p class='value'>" + String(humidity, 1) + "<span class='unit'> %</span></p></div>";
  html += "</div>";
  html += "<footer>Tampilan data suhu dan kelembapan secara real-time.</footer>";
  html += "</div>";
  html += "</body>";
  html += "</html>";
  return html;
}

// --- WEB SERVER HANDLERS ---
void handleRoot() {
  server.send(200, "text/html", buildHtmlPage());
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not Found");
}

// --- MAIN SETUP ---
void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting DHT11 Monitoring System...");

  securedClient.setInsecure();

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");

  dht.begin();

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(home_ssid, home_pass);
  Serial.print("Connecting to Home Wi-Fi...");
  lcd.clear();
  lcd.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected to Home Wi-Fi!");
  Serial.print("IP Address (STA): ");
  Serial.println(WiFi.localIP());

  WiFi.softAP(ap_ssid, ap_pass);
  Serial.print("Access Point Started: ");
  Serial.println(ap_ssid);
  Serial.print("IP Address (AP): ");
  Serial.println(WiFi.softAPIP());
  
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started.");

  lcd.clear();
  lcd.print("System Online!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  
  Serial.println("\nMelakukan pembacaan sensor pertama...");
  sendSensorData();
}

// --- MAIN LOOP ---
void loop() {
  // Menangani permintaan Web Server
  server.handleClient();

  // BARU: Logika waktu untuk membaca sensor setiap 2 detik
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    sendSensorData();
  }

  // Cek pesan baru dari Telegram secara berkala
  if (millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}
