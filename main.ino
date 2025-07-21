// Include library universal
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// --- KONFIGURASI TELEGRAM ---
#define BOT_TELEGRAM "7764599473:AAEmbBdVUvgBaYNA-MxrZGDvpOT8A2-fktU"
#define ADMIN_CHAT_ID "7123768604"

// --- KREDENSIAL WI-FI ---
char home_ssid[] = "Mechatronics";
char home_pass[] = "khususmeka";

// --- KONFIGURASI SENSOR & AP ---
#define DHTPIN 19
#define DHTTYPE DHT11
const char* ap_ssid = "DHT11_Sensor_AP";
const char* ap_pass = "12345678";

// --- OBJEK GLOBAL ---
WiFiClientSecure securedClient;
UniversalTelegramBot bot(BOT_TELEGRAM, securedClient);
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer server(80);

// --- TIMER ---
// Timer untuk membaca data sensor
unsigned long previousMillis = 0;
const long interval = 2000; // Interval pembacaan sensor (2 detik)

// Timer untuk mengirim notifikasi suhu tinggi
unsigned long previousAlertMillis = 0;
const long alertInterval = 60000; // Interval notifikasi (60000 ms = 60 detik)

// Timer untuk memeriksa pesan Telegram
unsigned long lastTimeBotRan;
int botRequestDelay = 1000; // Cek pesan baru setiap 1 detik

// --- VARIABEL GLOBAL ---
// Variabel global untuk menyimpan pembacaan sensor terakhir
float temperature = 0.0;
float humidity = 0.0;

// --- FUNGSI TELEGRAM ---

/**
 * @brief Menangani pesan baru yang masuk dari pengguna Telegram.
 * @param numNewMessages Jumlah pesan baru yang diterima.
 */
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

/**
 * @brief Mengirim pesan peringatan suhu tinggi ke ID chat admin.
 */
void sendHighTempAlert() {
  String message = "🔥 *Peringatan Suhu Tinggi!* 🔥\n\nSuhu saat ini adalah *" + String(temperature, 1) + " °C*, melebihi batas maksimum (30 °C). Harap segera diperiksa!";
  
  if (bot.sendMessage(ADMIN_CHAT_ID, message, "Markdown")) {
    Serial.println("Notifikasi Telegram berhasil dikirim!");
  } else {
    Serial.println("Gagal mengirim notifikasi Telegram.");
  }
}

// --- FUNGSI SENSOR ---

/**
 * @brief Membaca suhu dan kelembapan dari sensor DHT11 dan memperbarui variabel global.
 */
void readSensorData() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Periksa apakah pembacaan gagal dan keluar lebih awal (untuk mencoba lagi).
  if (isnan(h) || isnan(t)) {
    Serial.println("Gagal membaca dari sensor DHT!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sensor Error");
    return;
  }
  
  // Perbarui variabel global dengan pembacaan baru
  humidity = h;
  temperature = t;

  Serial.print("Suhu: ");
  Serial.print(temperature, 1);
  Serial.print(" *C, Kelembapan: ");
  Serial.print(humidity, 1);
  Serial.println(" %");

  // Perbarui tampilan LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Suhu:   ");
  lcd.print(temperature, 1);
  lcd.print((char)223); // Simbol derajat
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("Lembap: ");
  lcd.print(humidity, 1);
  lcd.print(" %");
}

// --- HALAMAN WEB DAN SERVER ---

/**
 * @brief Membuat HTML untuk halaman web server.
 * @return String yang berisi halaman HTML lengkap.
 */
String buildHtmlPage() {
  String html = "<!DOCTYPE html><html lang='en'>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Monitoring Suhu dan Kelembapan</title>";
  html += "<meta http-equiv='refresh' content='2'>"; // Segarkan halaman setiap 2 detik
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

void handleRoot() {
  server.send(200, "text/html", buildHtmlPage());
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not Found");
}

// --- PENGATURAN UTAMA ---
void setup() {
  Serial.begin(115200);
  Serial.println("\nMemulai Sistem Monitoring DHT11...");

  // Izinkan koneksi ke situs dengan sertifikat yang tidak aman
  securedClient.setInsecure();

  // Inisialisasi LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Inisialisasi...");

  // Inisialisasi sensor DHT
  dht.begin();

  // Atur mode WiFi ke Station dan Access Point
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(home_ssid, home_pass);
  Serial.print("Menyambungkan ke Wi-Fi Rumah...");
  lcd.clear();
  lcd.print("Menyambungkan WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nTerhubung ke Wi-Fi Rumah!");
  Serial.print("Alamat IP (STA): ");
  Serial.println(WiFi.localIP());

  // Mulai Access Point
  WiFi.softAP(ap_ssid, ap_pass);
  Serial.print("Access Point Dimulai: ");
  Serial.println(ap_ssid);
  Serial.print("Alamat IP (AP): ");
  Serial.println(WiFi.softAPIP());
  
  // Mulai Web Server
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server dimulai.");

  lcd.clear();
  lcd.print("Sistem Online!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  
  Serial.println("\nMelakukan pembacaan sensor pertama...");
  readSensorData(); // Pembacaan awal agar data siap
}

// --- LOOP UTAMA ---
void loop() {
  // Tangani permintaan web server yang masuk
  server.handleClient();

  unsigned long currentMillis = millis();

  // --- Aksi Berwaktu: Baca Data Sensor (setiap 2 detik) ---
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    readSensorData();
  }

  // --- Aksi Berwaktu: Kirim Peringatan Suhu Tinggi (setiap 60 detik) ---
  // Blok ini hanya berjalan jika suhu di atas ambang batas.
  if (temperature >= 30.0) {
    if (currentMillis - previousAlertMillis >= alertInterval) {
      previousAlertMillis = currentMillis; // Atur ulang timer notifikasi
      Serial.println("Suhu tinggi terdeteksi! Mengirim notifikasi terjadwal...");
      sendHighTempAlert();
    }
  }

  // --- Aksi Berwaktu: Periksa pesan baru Telegram (setiap 1 detik) ---
  if (currentMillis - lastTimeBotRan >= botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = currentMillis;
  }
}
