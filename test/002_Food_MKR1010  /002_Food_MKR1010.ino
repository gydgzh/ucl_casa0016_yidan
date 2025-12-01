#include <SPI.h>
#include <WiFiNINA.h>
#include "arduino_secrets.h"

#include "DHT.h"
#include "web_page.h"

// -------- WiFi settings --------
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

WiFiServer server(80);

// -------- Sensor settings --------
#define DHTPIN 2        // DHT22 data pin connected to D2
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

const int GAS_PIN = A0; // MQ-135 AO pin connected to A0 (3.3V system)

int gasBaseline = 0;    // baseline value for gas sensor

// ----------- Function declarations ----------- //
void connectToWiFi();
int  calculateRiskLevel(int gasDelta);
String buildPage(float t, float h, int gasRaw, int gasDelta, int riskLevel, int baseline);
void handleClient(WiFiClient &client, float t, float h, int gasRaw, int gasDelta, int riskLevel);

// ----------- Setup ----------- //
void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for Serial Monitor
  }

  Serial.println("Food Freshness Monitor (MKR WiFi 1010)");
  Serial.println("---------------------------------------");

  // Start DHT22
  dht.begin();

  // MQ-135 baseline (sensor warming up)
  Serial.println("Warming up MQ-135 and reading baseline...");
  delay(2000); // basic warm-up time

  long sum = 0;
  const int samples = 50;
  for (int i = 0; i < samples; i++) {
    int v = analogRead(GAS_PIN);
    sum += v;
    delay(50);
  }
  gasBaseline = sum / samples;
  Serial.print("Gas baseline = ");
  Serial.println(gasBaseline);

  // Connect to WiFi
  connectToWiFi();

  // Start web server
  server.begin();
  Serial.println("Web server started.");
  Serial.print("Open this IP in your browser: ");
  Serial.println(WiFi.localIP());
}

// ----------- Loop ----------- //
void loop() {
  // Read sensors
  float temperature = dht.readTemperature(); // Celsius
  float humidity    = dht.readHumidity();
  int   gasRaw      = analogRead(GAS_PIN);
  int   gasDelta    = gasRaw - gasBaseline;
  if (gasDelta < 0) gasDelta = 0;

  // Simple error handling for DHT
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
  }

  // Compute freshness risk from gasDelta
  int riskLevel = calculateRiskLevel(gasDelta);

  // Debug print
  Serial.print("T = ");
  Serial.print(temperature);
  Serial.print(" °C, H = ");
  Serial.print(humidity);
  Serial.print(" %, gasRaw = ");
  Serial.print(gasRaw);
  Serial.print(", gasDelta = ");
  Serial.print(gasDelta);
  Serial.print(", risk = ");
  Serial.println(riskLevel);

  // Handle HTTP client
  WiFiClient client = server.available();
  if (client) {
    handleClient(client, temperature, humidity, gasRaw, gasDelta, riskLevel);
  }

  delay(1000); // 1 second loop
}

// -------- Helper: connect WiFi -------- //
void connectToWiFi() {
  int status = WL_IDLE_STATUS;

  // Check WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true) { }
  }

  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);

  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid, pass);
    delay(5000);
  }

  Serial.println("Connected to WiFi.");
}

// -------- Helper: risk level from gasDelta -------- //
// 你后面可以根据自己实验结果调整这几个阈值
int calculateRiskLevel(int gasDelta) {
  if (gasDelta < 80) {
    return 0; // low risk / fresh
  } else if (gasDelta < 250) {
    return 1; // medium risk
  } else {
    return 2; // high risk
  }
}

// -------- Helper: build HTML page using template -------- //
String buildPage(float t, float h, int gasRaw, int gasDelta, int riskLevel, int baseline) {
  String riskText;
  String riskColor;

  if (riskLevel == 0) {
    riskText = "Low (Fresh)";
    riskColor = "#2ecc71"; // green
  } else if (riskLevel == 1) {
    riskText = "Medium (Use Soon)";
    riskColor = "#f1c40f"; // yellow
  } else {
    riskText = "High (Risk)";
    riskColor = "#e74c3c"; // red
  }

  // copy template
  String page = PAGE_TEMPLATE;

  // replace placeholders
  page.replace("{{TEMP}}",     String(t, 1));
  page.replace("{{HUM}}",      String(h, 1));
  page.replace("{{GAS_RAW}}",  String(gasRaw));
  page.replace("{{GAS_DELTA}}",String(gasDelta));
  page.replace("{{BASELINE}}", String(baseline));
  page.replace("{{RISK_TEXT}}",riskText);
  page.replace("{{RISK_COLOR}}",riskColor);

  return page;
}

// -------- Helper: handle HTTP client -------- //
void handleClient(WiFiClient &client, float t, float h, int gasRaw, int gasDelta, int riskLevel) {
  // Wait for data from client (with timeout)
  unsigned long timeout = millis() + 2000;
  while (!client.available() && millis() < timeout) {
    delay(1);
  }

  if (!client.available()) {
    client.stop();
    return;
  }

  // Read first line of request (we ignore its content)
  String req = client.readStringUntil('\r');

  // Flush rest of request
  while (client.available()) {
    client.read();
  }

  // Send HTTP header
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();

  // Send HTML body
  client.print(buildPage(t, h, gasRaw, gasDelta, riskLevel, gasBaseline));

  delay(1);
  client.stop();
}
