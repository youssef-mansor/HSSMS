// Pin Definitions
const int TRIG_PIN = 5;
const int ECHO_PIN = 18;
const int LED_WARN = 2; // Blinks at 2Hz for APPROACHING

// Parameters
const int N = 5; // Buffer size 
float distanceBuffer[N];
int bufferIndex = 0;

enum IntentState { IDLE, PASSING_BY, APPROACHING, STANDING };
IntentState currentState = IDLE;

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_WARN, OUTPUT);
  
  // Initialize buffer with a high value (no object)
  for(int i = 0; i < N; i++) distanceBuffer[i] = 400.0;
}

float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2; // Convert to cm [cite: 38]
}

bool isApproaching() {
  // Check for a monotonic decrease across the buffer [cite: 18, 19]
  for (int i = 0; i < N - 1; i++) {
    int current = (bufferIndex + i) % N;
    int next = (bufferIndex + i + 1) % N;
    if (distanceBuffer[next] >= distanceBuffer[current]) return false;
  }
  return true;
}

void loop() {
  float dist = getDistance();
  distanceBuffer[bufferIndex] = dist;
  bufferIndex = (bufferIndex + 1) % N;

  // Logic for IDLE vs APPROACHING [cite: 13, 18]
  if (dist > 100 || dist <= 0) {
    currentState = IDLE;
    digitalWrite(LED_WARN, LOW);
  } 
  else if (isApproaching()) {
    currentState = APPROACHING;
    // 2Hz Blink: 250ms ON, 250ms OFF 
    digitalWrite(LED_WARN, (millis() / 250) % 2); 
  }

  Serial.print("Distance: ");
  Serial.print(dist);
  Serial.print(" cm | State: ");
  Serial.println(currentState == APPROACHING ? "APPROACHING" : "IDLE");

  delay(200); // Sampling rate 
}