#include "DHT.h"

// --- Pin Definitions ---
const int TRIG_PIN = 5;
const int ECHO_PIN = 18;
const int LED_1 = 2;    // Visual Warning / State Indicator
const int LED_2 = 15;   // High Threat / State Indicator
const int DHTPIN = 4;
const int GAS_PIN = 34; // ADC pin for Gas Sensor
const int BUZZER_PIN = 13;

// --- Parameters & Constants ---
#define DHTTYPE DHT11
const int N = 10;            // Trajectory buffer size
const long dhtInterval = 60000; // 1 sec duty cycle
const int GAS_THRESHOLD = 600;  // Alarm threshold in ppm

// --- Global Variables ---
DHT dht(DHTPIN, DHTTYPE);
float buffer[N];
int idx = 0;
float temperature = 0;
int gasValue = 0;
unsigned long lastDHTUpdate = 0;

enum Intent { IDLE, PASSING_BY, APPROACHING, STANDING };
Intent state = IDLE;

void setup() {
  Serial.begin(115200);
  
  // Security Hardware
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  for(int i=0; i<N; i++) buffer[i] = 400.0;

  // Safety Hardware
  dht.begin();
  pinMode(GAS_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
}

// --- Sensor Helper Functions ---

float readDist() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  // Return high value if pulse times out to maintain IDLE
  return (duration == 0) ? 400.0 : (duration * 0.034 / 2);
}

bool checkMonotonic() {
  for(int i=0; i<N-1; i++) {
    // Check if distance ever increases or stays same
    if(buffer[(idx+i)%N] <= buffer[(idx+i+1)%N]) return false;
  }
  return true;
}

bool checkStable() {
  float first = buffer[0];
  for(int i=1; i<N; i++) {
    if(abs(buffer[i] - first) > 3.0) return false;
  }
  return (buffer[0] < 150.0);
}

bool checkPassing() {
  int mid = N / 2;
  return (buffer[0] > buffer[mid] && buffer[N-1] > buffer[mid]);
}

// --- Main Logic Loop ---

void loop() {
  unsigned long now = millis();

  // 1. Security Logic
  float d = readDist();
  buffer[idx] = d;
  idx = (idx + 1) % N;

  // Determine State
  if(d > 100 || d <= 0) state = IDLE;
  else if(checkStable()) state = STANDING;
  else if(checkMonotonic()) state = APPROACHING;
  else if(checkPassing()) state = PASSING_BY;

  // 2. Safety Logic (DHT11 Duty Cycle)
  // Powered for ~2 seconds per minute (reading takes time, then remains in variable)
  if (now - lastDHTUpdate >= dhtInterval) {
    temperature = dht.readTemperature();
    lastDHTUpdate = now;
  }

  // 3. Gas Monitoring
  gasValue = analogRead(GAS_PIN);
  bool gasAlarm = (gasValue <= GAS_THRESHOLD);

  // 4. Output & Combined Alarm Control
  if (state == STANDING && (gasAlarm || temperature > 40)) {
    if ((now / 100) % 2) {
      tone(BUZZER_PIN, 2500); 
    } else {
      noTone(BUZZER_PIN);
    }
    digitalWrite(LED_1, HIGH);
    digitalWrite(LED_2, HIGH);
  } else {
    if (gasAlarm) {
      tone(BUZZER_PIN, 1500);
    } else {
      noTone(BUZZER_PIN);
    }

    if (state == IDLE || state == PASSING_BY) {
      digitalWrite(LED_1, LOW);
      digitalWrite(LED_2, LOW);
    } else if (state == APPROACHING) {
      digitalWrite(LED_1, (now / 250) % 2);
      digitalWrite(LED_2, LOW);
    } else if (state == STANDING) {
      digitalWrite(LED_1, (now / 500) % 2);
      digitalWrite(LED_2, HIGH);           
    }
  }
  // Serial Debugging
  Serial.print("Dist: "); Serial.print(d);
  Serial.print(" cm | State: "); Serial.print(state);
  Serial.print(" | Temp: "); Serial.print(temperature);
  Serial.print(" C | Gas: "); Serial.println(gasValue);

  delay(200); 
}