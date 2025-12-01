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

// -------- Global variables --------
int gasBaseline = 0;                 // baseline value for gas sensor
unsigned long bananaStartMillis = 0; // time when we start monitoring (ms)

// -------- Function declarations --------
void   connectToWiFi();
int    calculateRiskLevel(float fracChange);
float  estimateMaxShelfHours(float t, float h);
String buildPage(float t, float h, int gasRaw, int gasDelta,
                 int riskLevel, int baseline,
                 float bananaHours, float remainDays);
void   handleClient(WiFiClient &client, float t, float h,
                    int gasRaw, int gasDelta, int riskLevel,
                    float bananaHours, float remainDays);

// ------------------- Setup ------------------- //
void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for Serial Monitor
  }

  Serial.println("Banana Freshness Monitor (MKR WiFi 1010)");
  Serial.println("----------------------------------------");

  // Start DHT22
  dht.begin();

  // MQ-135 baseline (sensor warming up a bit)
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

  // set banana start time AFTER we measure baseline
  bananaStartMillis = millis();

  // Connect to WiFi
  connectToWiFi();

  // Start web server
  server.begin();
  Serial.println("Web server started.");
  Serial.print("Open this IP in your browser: ");
  Serial.println(WiFi.localIP());
}

// ------------------- Loop ------------------- //
void loop() {
  // Read sensors
  float temperature = dht.readTemperature(); // Celsius
  float humidity    = dht.readHumidity();
  int   gasRaw      = analogRead(GAS_PIN);

  int   gasDelta = gasRaw - gasBaseline;
  if (gasDelta < 0) gasDelta = 0;

  // Time since start (hours)
  unsigned long nowMillis = millis();
  unsigned long elapsedMs = nowMillis - bananaStartMillis;
  float bananaHours = elapsedMs / 3600000.0;  // ms to hours

  // Relative change: how much more gas than baseline (percentage)
  float fracChange = 0.0;
  if (gasBaseline > 0) {
    fracChange = (float)gasDelta / (float)gasBaseline; // e.g. 0.2 = +20%
  }

  // Compute freshness risk from relative gas change
  int riskLevel = calculateRiskLevel(fracChange);

  // Estimate total shelf life (hours) based on current T/H
  float maxShelfHours = estimateMaxShelfHours(temperature, humidity);

  // Map fracChange to "progress" between fresh (0) and spoiled (1)
  const float spoilFrac = 0.70; // 70% increase ~ spoiled (more tolerant)
  float progress = 0.0;
  if (spoilFrac > 0.0) {
    progress = fracChange / spoilFrac;
  }
  if (progress < 0.0) progress = 0.0;
  if (progress > 1.0) progress = 1.0;

  float remainHours = (1.0 - progress) * maxShelfHours;
  if (remainHours < 0) remainHours = 0;
  float remainDays = remainHours / 24.0;

  // Debug print: can be exported for plotting
  Serial.print("age_h=");
  Serial.print(bananaHours, 2);
  Serial.print(", T=");
  Serial.print(temperature);
  Serial.print("C, H=");
  Serial.print(humidity);
  Serial.print("%, gasRaw=");
  Serial.print(gasRaw);
  Serial.print(", gasDelta=");
  Serial.print(gasDelta);
  Serial.print(" (");
  Serial.print(fracChange * 100.0, 1);
  Serial.print("% over baseline)");
  Serial.print(", risk=");
  Serial.print(riskLevel);
  Serial.print(", remainDays=");
  Serial.println(remainDays, 2);

  // Handle HTTP client
  WiFiClient client = server.available();
  if (client) {
    handleClient(client, temperature, humidity,
                 gasRaw, gasDelta, riskLevel,
                 bananaHours, remainDays);
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

// -------- Helper: risk level from relative change -------- //
// fracChange = gasDelta / gasBaseline
// < 0.15  → fresh
// 0.15–0.40 → medium (use soon)
// > 0.40  → high (probably spoiled)
int calculateRiskLevel(float fracChange) {
  if (fracChange < 0.15) {
    return 0; // low risk / fresh
  } else if (fracChange < 0.40) {
    return 1; // medium risk (use soon)
  } else {
    return 2; // high risk (probably spoiled)
  }
}

// -------- Helper: estimate max shelf hours from T, H -------- //
// Simple heuristic model for the project:
// Base: ~96h (~4 days) at 18–26°C and 40–80% RH.
// Very warm / very cold / very humid / very dry → shorter shelf life.
float estimateMaxShelfHours(float t, float h) {
  float baseHours = 96.0; // about 4 days at comfortable temperature

  // Temperature effect
  if (t > 30.0) {
    baseHours = 48.0; // very warm, spoils faster
  } else if (t > 26.0) {
    baseHours = 72.0;
  } else if (t < 18.0) {
    baseHours = 72.0; // cooler than ideal for bananas
  }

  // Humidity effect (small adjustment)
  if (h > 80.0) {
    baseHours *= 0.8; // very humid, faster spoilage
  } else if (h < 40.0) {
    baseHours *= 0.9; // very dry, small reduction
  }

  if (baseHours < 12.0) baseHours = 12.0; // avoid zero
  return baseHours;
}

// -------- Helper: build HTML page using template -------- //
String buildPage(float t, float h, int gasRaw, int gasDelta,
                 int riskLevel, int baseline,
                 float bananaHours, float remainDays) {
  String riskText;
  String riskColor;

  if (riskLevel == 0) {
    riskText = "Low (Fresh)";
    riskColor = "#2ecc71"; // green
  } else if (riskLevel == 1) {
    riskText = "Medium (Use Soon)";
    riskColor = "#f1c40f"; // yellow
  } else {
    riskText = "High (Risk / Spoiled)";
    riskColor = "#e74c3c"; // red
  }

  // Decide emoji + message based on remaining days & risk
  String emoji;
  String stateText;

  if (riskLevel == 0 && remainDays > 3.0) {
    emoji = "🍌😋";
    stateText = "Very fresh – you can keep it for a while.";
  } else if (remainDays > 1.0 && riskLevel <= 1) {
    emoji = "🍌🙂";
    stateText = "Good to eat now.";
  } else if (remainDays > 0.0 && riskLevel >= 1) {
    emoji = "🍌😬";
    stateText = "Eat today or it may spoil.";
  } else {
    emoji = "🍌💀";
    stateText = "Probably spoiled – do not eat.";
  }

  // copy template
  String page = PAGE_TEMPLATE;

  // replace placeholders
  page.replace("{{TEMP}}",         String(t, 1));
  page.replace("{{HUM}}",          String(h, 1));
  page.replace("{{GAS_RAW}}",      String(gasRaw));
  page.replace("{{GAS_DELTA}}",    String(gasDelta));
  page.replace("{{BASELINE}}",     String(baseline));
  page.replace("{{RISK_TEXT}}",    riskText);
  page.replace("{{RISK_COLOR}}",   riskColor);
  page.replace("{{BANANA_HOURS}}", String(bananaHours, 1));
  page.replace("{{REMAIN_DAYS}}",  String(remainDays, 1));
  page.replace("{{EMOJI}}",        emoji);
  page.replace("{{STATE_TEXT}}",   stateText);

  return page;
}

// -------- Helper: handle HTTP client -------- //
void handleClient(WiFiClient &client, float t, float h,
                  int gasRaw, int gasDelta, int riskLevel,
                  float bananaHours, float remainDays) {
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
  client.print(buildPage(t, h, gasRaw, gasDelta,
                         riskLevel, gasBaseline,
                         bananaHours, remainDays));

  delay(1);
  client.stop();
}
