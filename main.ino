#include <Servo.h>

Servo servo1;

#define SERVO1_PIN 8

const int trigPin = 5;
const int echoPin = 4;

const int bluePin = 11;
const int greenPin = 10;
const int redPin = 9;

const int redPin2 = 2;
const int greenPin2 = 3;
const int bluePin2 = 12;

const int buzzerPin = 7;

const int WINKEL_OFFEN = 120;
const int WINKEL_ZU = 67;
const int SERVO_SPEED = 35; // ms pro Grad

const int ABSTAND_START = 10;   // cm: Hand vor den Sensor halten zum Schließen
const int ABSTAND_WARNUNG = 7; // cm: Wenn man der geschlossenen Box zu nahe kommt

const unsigned long START_VERZOEGERUNG = 2000; // 3 Sekunden warten vor Start
const unsigned long FOKUS_ZEIT = 20000;        

// --- ZUSTEANDE ---
enum BoxZustand {
  BOX_OFFEN_NEUTRAL,     
  BOX_START_COUNTDOWN,   
  BOX_SCHLIESST,         
  FOKUS_LAEUFT,          
  BOX_OEFFNET,           
  BOX_FERTIG             
};

BoxZustand zustand = BOX_OFFEN_NEUTRAL;
unsigned long timerStartMillis = 0;
unsigned long warnungsStartMillis = 0;
unsigned long letzteWarnungMillis = 0; 
int fehlerZaehler = 0; 

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  pinMode(bluePin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(redPin, OUTPUT);

  pinMode(redPin2, OUTPUT);
  pinMode(greenPin2, OUTPUT);
  pinMode(bluePin2, OUTPUT);
  
  pinMode(buzzerPin, OUTPUT);

  servo1.attach(SERVO1_PIN);
  servo1.write(WINKEL_OFFEN); 
  
  Serial.begin(9600);
}

void setLED(int r, int g, int b) {
  analogWrite(redPin, r);
  analogWrite(greenPin, g);
  analogWrite(bluePin, b);

  analogWrite(redPin2, r);
  analogWrite(greenPin2, g);
  analogWrite(bluePin2, b);
}

float messeAbstand() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000); 
  
  if (duration == 0) return 200.0; 
  return duration * 0.034 / 2;
}

void bewegeServoLangsam(int startWinkel, int zielWinkel) {
  if (startWinkel < zielWinkel) {
    for (int pos = startWinkel; pos <= zielWinkel; pos++) {
      servo1.write(pos); 
      delay(SERVO_SPEED);
    }
  } else {
    for (int pos = startWinkel; pos >= zielWinkel; pos--) {
      servo1.write(pos);
      delay(SERVO_SPEED);
    }
  }
}

void spieleErfolgsMelodie() {
  // Tonabfolge (C5, E5, G5, C6)
  int melodie[] = {523, 659, 784, 1047}; 
  int tonDauer[] = {150, 150, 150, 400}; // Die ersten Töne kurz, der letzte lang

  for (int i = 0; i < 4; i++) {
    tone(buzzerPin, melodie[i]);
    delay(tonDauer[i]);
    noTone(buzzerPin);
    delay(30); 
  }
}

void loop() {
  float abstand = messeAbstand();
  
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 200) {
    Serial.print("Abstand: "); 
    Serial.println(abstand);
    lastPrint = millis();
  }

  switch (zustand) {
    
    case BOX_OFFEN_NEUTRAL:
      setLED(0, 0, 255); // Blau
      
      if (abstand < ABSTAND_START) {
        fehlerZaehler = 0; 
        warnungsStartMillis = millis(); 
        zustand = BOX_START_COUNTDOWN;
      }
      break;

    case BOX_START_COUNTDOWN:
      // Gelb blinken
      if ((millis() / 500) % 2 == 0) {
        setLED(255, 255, 0); 
        tone(buzzerPin, 1000);
      } else {
        setLED(0, 0, 0);    
        noTone(buzzerPin);
      }

      if (abstand > (ABSTAND_START + 5)) {
        fehlerZaehler++;
        if (fehlerZaehler >= 3) {
          noTone(buzzerPin);
          zustand = BOX_OFFEN_NEUTRAL; 
          break;
        }
      } else {
        fehlerZaehler = 0; 
      }

      if (millis() - warnungsStartMillis >= START_VERZOEGERUNG) {
        noTone(buzzerPin);
        zustand = BOX_SCHLIESST; 
      }
      break;

    case BOX_SCHLIESST:
      setLED(255, 255, 0); // Gelb
      bewegeServoLangsam(WINKEL_OFFEN, WINKEL_ZU); 
      
      timerStartMillis = millis(); 
      zustand = FOKUS_LAEUFT;
      break;

    case FOKUS_LAEUFT:
      // Verzögerungs-Logik für klares Rot
      if (abstand < ABSTAND_WARNUNG) {
        letzteWarnungMillis = millis(); // Zeitstempel aktualisieren, solange Hand da ist
      }

      // Wenn die letzte Annäherung weniger als 500ms her ist, erzwinge ROT
      if (millis() - letzteWarnungMillis < 500) {
        setLED(255, 0, 0); // Reines ROT

        if ((millis() / 150) % 2 == 0) {
          tone(buzzerPin, 800); // 800 Hz ist ein klarer, freundlicherer Piepton
        } else {
          noTone(buzzerPin);
        }
        
         
      } else {
        setLED(0, 0, 255); // Blau
        noTone(buzzerPin);
      }
      
      if (millis() - timerStartMillis >= FOKUS_ZEIT) {
        noTone(buzzerPin); 
        zustand = BOX_OEFFNET; 
      }
      break;

    case BOX_OEFFNET:
      setLED(0, 255, 0); // Grün

      spieleErfolgsMelodie();
      
      bewegeServoLangsam(WINKEL_ZU, WINKEL_OFFEN);
      zustand = BOX_FERTIG;
      break;

    case BOX_FERTIG:
      setLED(0, 0, 255); // Blau
      delay(3000); 
      zustand = BOX_OFFEN_NEUTRAL; 
      break;
  }
}
