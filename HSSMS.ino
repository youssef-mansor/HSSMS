const int TRIG_PIN = 5;
const int ECHO_PIN = 18;
const int LED_1 = 2;  
const int LED_2 = 15; 

const int N = 10; 
float buffer[N];
int idx = 0;

enum Intent { IDLE, PASSING_BY, APPROACHING, STANDING };
Intent state = IDLE;

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  for(int i=0; i<N; i++) buffer[i] = 400.0;
}

float readDist() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  return pulseIn(ECHO_PIN, HIGH) * 0.034 / 2;
}

bool checkMonotonic() {
  for(int i=0; i<N-1; i++) {
    if(buffer[(idx+i)%N] <= buffer[(idx+i+1)%N]) return false;
  }
  return true;
}

bool checkStable() {
  float first = buffer[0];
  for(int i=1; i<N; i++) {
    if(abs(buffer[i] - first) > 3.0) return false;
  }
  return (buffer[0] < 70.0);
}

bool checkPassing() {
  int mid = N / 2;
  bool decreasing = buffer[0] > buffer[mid];
  bool increasing = buffer[N-1] > buffer[mid];
  return (decreasing && increasing);
}

void updateOutputs() {
  unsigned long now = millis();
  switch(state) {
    case IDLE:
    case PASSING_BY:
      digitalWrite(LED_1, LOW);
      digitalWrite(LED_2, LOW);
      break;
    case APPROACHING:
      digitalWrite(LED_1, (now / 250) % 2); // 2Hz
      digitalWrite(LED_2, LOW);
      break;
    case STANDING:
      digitalWrite(LED_1, (now / 500) % 2); // 1Hz
      digitalWrite(LED_2, HIGH);           // Solid
      break;
  }
}

void loop() {
  float d = readDist();
  if(d > 0 && d < 400) {
    buffer[idx] = d;
    idx = (idx + 1) % N;
  }

  if(d > 100 || d <= 0) state = IDLE;
  else if(checkStable()) state = STANDING;
  else if(checkMonotonic()) state = APPROACHING;
  else if(checkPassing()) state = PASSING_BY;

  updateOutputs();
  
  Serial.print("Dist: "); Serial.print(d);
  Serial.print(" | State: ");
  if(state == IDLE) Serial.println("IDLE");
  if(state == PASSING_BY) Serial.println("PASSING BY");
  if(state == APPROACHING) Serial.println("APPROACHING");
  if(state == STANDING) Serial.println("STANDING");

  delay(200); 
}