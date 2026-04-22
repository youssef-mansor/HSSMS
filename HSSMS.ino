#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>

const char* ssid = "Adham";
const char* password = "12345678";

const int TRIG_PIN = 5;
const int ECHO_PIN = 18;
const int LED_1 = 2;    
const int LED_2 = 15;   
const int DHTPIN = 4;
const int GAS_PIN = 34; 
const int BUZZER_PIN = 13;

#define DHTTYPE DHT11
const int N = 10;            
const long dhtInterval = 60000; 
const int GAS_THRESHOLD = 600;  

DHT dht(DHTPIN, DHTTYPE);
AsyncWebServer server(80);

float buffer[N];
int idx = 0;
float temperature = 0;
int gasValue = 0;
unsigned long lastDHTUpdate = 0;

enum Intent { IDLE, PASSING_BY, APPROACHING, STANDING };
Intent state = IDLE;

// FIXED: All CSS percentages now use %% to prevent the blank page error
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="refresh" content="1">
  <title>HSSMS</title>
  <style>
    html { font-family: Arial; display: inline-block; text-align: center; background-color: #f4f7f6; }
    .card { background: white; border-radius: 10px; padding: 20px; margin: 15px auto; width: 85%%; max-width: 400px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }
    .state { font-size: 1.5rem; font-weight: bold; color: #e74c3c; }
  </style>
</head>
<body>
  <h2>HSSMS Monitor</h2>
  <div class="card"><div>System State:</div><div class="state">%STATE%</div></div>
  <div class="card">
    <div>Distance: %DISTANCE% cm</div>
    <div>Temp: %TEMP% C</div>
    <div>Gas: %GAS%</div>
  </div>
</body>
</html>)rawliteral";

String processor(const String& var) {
  if (var == "STATE") {
    if (state == IDLE) return "IDLE";
    if (state == PASSING_BY) return "PASSING";
    if (state == APPROACHING) return "APPROACHING";
    if (state == STANDING) return "STANDING";
  }
  if (var == "DISTANCE") return String(buffer[(idx + N - 1) % N], 1);
  if (var == "TEMP") return isnan(temperature) ? "..." : String(temperature, 1);
  if (var == "GAS") return String(gasValue);
  return String();
}

float readDist() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 26000); 
  return (duration == 0) ? 400.0 : (duration * 0.034 / 2);
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT); pinMode(ECHO_PIN, INPUT);
  pinMode(LED_1, OUTPUT); pinMode(LED_2, OUTPUT);
  pinMode(GAS_PIN, INPUT); pinMode(BUZZER_PIN, OUTPUT);
  for(int i=0; i<N; i++) buffer[i] = 400.0;
  dht.begin();

  Serial.println("\n--- SYSTEM BOOTING ---");
  Serial.print("Connecting to: "); Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  // Try for 10 seconds only
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nSUCCESS! IP: "); Serial.println(WiFi.localIP());
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", index_html, processor);
    });
    server.begin();
  } else {
    Serial.println("\nWiFi Failed. Hotspot not found or 5GHz issue.");
    Serial.println("Running in OFFLINE mode for now...");
  }
}

void loop() {
  unsigned long now = millis();
  float d = readDist();
  buffer[idx] = d;
  idx = (idx + 1) % N;

  if(d > 100 || d <= 0) state = IDLE;
  else if ([&](){ float f=buffer[0]; for(int i=1;i<N;i++) if(abs(buffer[i]-f)>3.0) return false; return buffer[0]<150.0; }()) state = STANDING;
  else if ([&](){ for(int i=0;i<N-1;i++) if(buffer[(idx+i)%N]<=buffer[(idx+i+1)%N]) return false; return true; }()) state = APPROACHING;
  else if (buffer[0] > buffer[N/2] && buffer[N-1] > buffer[N/2]) state = PASSING_BY;

  if (now - lastDHTUpdate >= dhtInterval) { float t = dht.readTemperature(); if (!isnan(t)) temperature = t; lastDHTUpdate = now; }
  gasValue = analogRead(GAS_PIN);
  bool gasActive = (gasValue <= GAS_THRESHOLD);

  if (state == STANDING && (gasActive || temperature > 40)) {
    if ((now / 100) % 2) tone(BUZZER_PIN, 2500); else noTone(BUZZER_PIN);
    digitalWrite(LED_1, HIGH); digitalWrite(LED_2, HIGH);
  } else {
    if (gasActive) tone(BUZZER_PIN, 1500); else noTone(BUZZER_PIN);
    if (state == IDLE || state == PASSING_BY) { digitalWrite(LED_1, LOW); digitalWrite(LED_2, LOW); }
    else if (state == APPROACHING) { digitalWrite(LED_1, (now / 250) % 2); digitalWrite(LED_2, LOW); }
    else if (state == STANDING) { digitalWrite(LED_1, (now / 500) % 2); digitalWrite(LED_2, HIGH); }
  }
  
  // Minimal serial output so it doesn't "glitch"
  if (now % 1000 < 200) { 
    Serial.print("Dist: "); Serial.print(d); 
    Serial.print(" State: "); Serial.println(state); 
  }
  
  delay(200); 
}