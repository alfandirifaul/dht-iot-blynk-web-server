

// Define this to print debug output to Serial
#define BLYNK_PRINT Serial

#define BLYNK_TEMPLATE_ID "TMPL68mZK1j9W"
#define BLYNK_TEMPLATE_NAME "monitoring suhu dan kelembapan"
#define BLYNK_AUTH_TOKEN "Jp-9sx7qKST5m5olUSYFeNbls1CRAs9Z"

// Universal includes
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WebServer.h>
#include <BlynkSimpleEsp32.h>

#define BLYNK_EVENT "suhu-maks"
#define VPIN_SUHU V0
#define VPIN_LEMBAP V1

// --- WI-FI CREDENTIALS ---
char home_ssid[] = "Mechatronics";
char home_pass[] = "khususmeka";

// --- SENSOR & AP CONFIGURATION ---
#define DHTPIN 19
#define DHTTYPE DHT11
const char* ap_ssid = "DHT11_Sensor_AP";
const char* ap_pass = "12345678";

// --- GLOBAL OBJECTS ---
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2); 
WebServer server(80);
BlynkTimer timer; 

// Global variables to store sensor readings
float temperature = 0.0;
float humidity = 0.0;

// --- SENSOR READING AND DATA SENDING FUNCTION ---
void sendSensorData() {
  // Read sensor values
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Check if any reads failed. If so, do nothing.
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sensor Error");
    return; // Keluar dari fungsi jika sensor error
  }
  
  // Update global variables if read was successful
  humidity = h;
  temperature = t;

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" *C, Humidity: ");
  Serial.println(humidity);

  // --- Send data to Blynk ---
  Blynk.virtualWrite(VPIN_SUHU, temperature); // V0 untuk Suhu
  Blynk.virtualWrite(VPIN_LEMBAP, humidity);    // V1 untuk Kelembapan

  // --- Check temperature and trigger event if it exceeds 30°C ---
  if (temperature >= 30.0) {
    Blynk.logEvent(BLYNK_EVENT, "Peringatan: Suhu melebihi 30C!");
    Serial.println("Blynk event triggered.");
  }

  // --- Update the LCD display ---
  lcd.clear();
  // Line 1: Temperature
  lcd.setCursor(0, 0);
  lcd.print("Suhu: ");
  lcd.print(temperature, 2);
  lcd.print((char)223); // Degree symbol
  lcd.print("C");

  // Line 2: Humidity
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

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");

  // Initialize DHT sensor
  dht.begin();

  // --- Wi-Fi Setup (STA+AP Mode) ---
  WiFi.mode(WIFI_AP_STA);
  // Connect to Home Wi-Fi (STA)
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

  // Start Access Point (AP)
  WiFi.softAP(ap_ssid, ap_pass);
  Serial.print("Access Point Started: ");
  Serial.println(ap_ssid);
  Serial.print("IP Address (AP): ");
  Serial.println(WiFi.softAPIP());

  // --- Blynk, Web Server, and Timer Setup ---
  Blynk.config(BLYNK_AUTH_TOKEN);
  
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started.");

  // Setup a timer to run the sendSensorData function every 2 seconds
  timer.setInterval(2000L, sendSensorData);

  lcd.clear();
  lcd.print("System Online!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP()); // Show IP for home network
}

// --- MAIN LOOP ---
void loop() {
  Blynk.run();      // Handles Blynk connection
  server.handleClient(); // Handles incoming web requests
  timer.run();      // Runs tasks scheduled by BlynkTimer
}
