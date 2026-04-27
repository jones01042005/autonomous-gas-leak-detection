#include <WiFi.h>
#include <HTTPClient.h>

// 🔥 YOUR WIFI DETAILS
const char* ssid = "Poco X6";
const char* password = "jones007";

// 🔥 YOUR LAPTOP IP
const char* serverName = "https://prefeudal-respectively-britni.ngrok-free.dev/data";

#define MQ2_PIN 34
#define MQ7_PIN 35

#define GREEN_LED 25
#define YELLOW_LED 26
#define RED_LED 32

#define BUZZER 14
#define RELAY 33

// Baseline values (GLOBAL)
int base_mq2 = 450;
int base_mq7 = 850;

// 🔥 EDGE AI MODEL FUNCTION
String predictGas(int mq2, int mq7) {

  float score_danger =
  (0.0149834 * mq2) +
  (0.01188223 * mq7) -
  34.77579807;
  float score_safe =
  (-0.02859949 * mq2) +
  (-0.00691922 * mq7) +
  41.42305698;
  float score_warning =
  (0.01361609 * mq2) +
  (-0.00496301 * mq7) -
  6.6472589;

  if (score_danger > score_safe*0.9 && score_danger > score_warning) {
    return "danger";
  }
  else if (score_safe > score_warning) {
    return "safe";
  }
  else {
    return "warning";
  }
}

void wifiConnectedBlink() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(YELLOW_LED, HIGH);
    digitalWrite(RED_LED, HIGH);
    delay(250);

    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, LOW);
    delay(250);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY, OUTPUT);

  // 🔌 CONNECT TO WIFI (MODIFIED - NON BLOCKING)
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi!");
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());

    // 🔥 BLINK LEDs 3 TIMES AFTER CONNECT
    wifiConnectedBlink();
  } else {
    Serial.println("\nWiFi NOT connected. Running in OFFLINE mode.");
  }

  delay(20000); // warm-up
}

void loop() {

  // 🔁 AUTO RECONNECT WIFI (ADDED)
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastAttempt = 0;

    if (millis() - lastAttempt > 10000) {
      Serial.println("Reconnecting to WiFi...");
      WiFi.begin(ssid, password);
      lastAttempt = millis();
    }
  }

  int mq2 = analogRead(MQ2_PIN);
  int mq7 = analogRead(MQ7_PIN);

  int diff_mq2 = mq2 - base_mq2;
  int diff_mq7 = mq7 - base_mq7;

  int gasLevel = max(diff_mq2, diff_mq7);

  // 🔥 USE EDGE AI MODEL
  String status = predictGas(mq2, mq7);

  Serial.print("MQ2: ");
  Serial.print(mq2);
  Serial.print(" MQ7: ");
  Serial.print(mq7);
  Serial.print(" Level: ");
  Serial.print(gasLevel);
  Serial.print(" AI Status: ");
  Serial.println(status);

  // 🔥 APPLY ACTION BASED ON AI
  if (status == "safe") {

    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER, LOW);
    digitalWrite(RELAY, LOW);
  }

  else if (status == "warning") {

    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, HIGH);
    digitalWrite(RED_LED, LOW);

    digitalWrite(RELAY, HIGH);

    digitalWrite(BUZZER, HIGH);
    delay(200);
    digitalWrite(BUZZER, LOW);
  }

  else { // danger

    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, HIGH);

    digitalWrite(RELAY, HIGH);
    digitalWrite(BUZZER, HIGH);
  }

  // 🔥 SEND DATA TO SERVER (ONLY IF WIFI AVAILABLE)
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    String jsonData = "{";
    jsonData += "\"house_id\":\"house_01\",";
    jsonData += "\"gas_type\":\"LPG\",";
    jsonData += "\"ppm_mq2\":" + String(mq2) + ",";
    jsonData += "\"ppm_mq7\":" + String(mq7) + ",";
    jsonData += "\"alert_level\":\"" + status + "\",";
    jsonData += "\"fan_status\":\"" + String((status != "safe") ? "on" : "off") + "\",";
    jsonData += "\"timestamp\":\"" + String(millis()) + "\"";
    jsonData += "}";

    int httpResponseCode = http.POST(jsonData);

    Serial.print("HTTP Response: ");
    Serial.println(httpResponseCode);

    http.end();
  } else {
    Serial.println("Offline mode: Data not sent");
  }

  delay(1000);
}