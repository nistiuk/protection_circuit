#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD address to 0x20
LiquidCrystal_I2C lcd(0x20, 16, 2);

// Pin Definitions
const int VOLTAGE_SENSOR_PIN = A0;   // sensed voltage
const int V_POT_PIN          = A1;   // Voltage threshold potentiometer (0 - 2.5V)
const int I_POT_PIN          = A2;   // Current threshold potentiometer (0 - 2.5V)

const int RELAY_PIN          = 7;    // Relay cut-off output
const int FAULT_LED_PIN      = 2;    // Fault LED (RED)
const int NORMAL_LED_PIN     = 3;    // Normal LED (GREEN)
const int BUZZER_PIN         = 4;    // Buzzer pin

float threshold_voltage = 0.0; 
float threshold_current = 0.0; 

unsigned long previousMillis = 0;
bool ledState = LOW;

// Blink intervals 
const long NORMAL_INTERVAL = 500; // Normal state
const long FAULT_INTERVAL  = 100; // Fault state

void setup() {
  // Output Pins
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(FAULT_LED_PIN, OUTPUT);
  pinMode(NORMAL_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Input Pins
  pinMode(VOLTAGE_SENSOR_PIN, INPUT);
  pinMode(V_POT_PIN, INPUT);
  pinMode(I_POT_PIN, INPUT);
  
  // FORCE INITIAL LINE CUT-OFF ON BOOT
  digitalWrite(RELAY_PIN, HIGH); 
  digitalWrite(FAULT_LED_PIN, LOW);  
  digitalWrite(NORMAL_LED_PIN, LOW); 
  digitalWrite(BUZZER_PIN, LOW);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();

  Serial.begin(9600);
  Serial.println("System Initialized (Voltage & Current Protection Logic Active).");
}

void loop() {
  // Read A1 (0 - 2.5V expected) and scale to 0 - 12V threshold
  float vPotVoltage = analogRead(V_POT_PIN) * (5.00 / 1023.0);
  threshold_voltage = (vPotVoltage / 2.5) * 12.0;
  if (threshold_voltage > 12.0) threshold_voltage = 12.0;

  // Read A2 (0 - 2.5V expected) and scale to 0 - 420mA threshold 
  float iPotVoltage = analogRead(I_POT_PIN) * (5.00 / 1023.0);
  threshold_current = (iPotVoltage / 2.5) * 420.0;
  if (threshold_current > 420.0) threshold_current = 420.0;

  // Read System Voltage 
  float raw_voltage = analogRead(VOLTAGE_SENSOR_PIN);
  float voltage = raw_voltage * (5.00 / 1023.0);

  float current_mA = 0.0;

  Serial.print("Load Voltage: ");
  Serial.print(voltage, 2);
  Serial.print("V (Limit: ");
  Serial.print(threshold_voltage, 2);
  Serial.print("V) | Current Feed: ");
  Serial.print(current_mA, 1);
  Serial.print("mA (Limit: ");
  Serial.print(threshold_current, 1);
  Serial.println("mA)");

  // Check Fault Conditions 
  bool voltageFault = (voltage >= threshold_voltage);
  bool currentFault = (current_mA >= threshold_current); 
  bool isFault      = (voltageFault || currentFault);

  // LED blinking
  long currentInterval = isFault ? FAULT_INTERVAL : NORMAL_INTERVAL;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= currentInterval) {
    previousMillis = currentMillis;
    ledState = !ledState;
  }

  // Relay, LED, and Buzzer Outputs
  if (isFault) {
    digitalWrite(RELAY_PIN, HIGH);          // Relay HIGH (Cut-off)
    digitalWrite(FAULT_LED_PIN, ledState);  
    digitalWrite(NORMAL_LED_PIN, LOW);      
    digitalWrite(BUZZER_PIN, HIGH);         // Buzzer HIGH during fault
  } else {
    digitalWrite(RELAY_PIN, LOW);           // Relay LOW 
    digitalWrite(FAULT_LED_PIN, LOW);       
    digitalWrite(NORMAL_LED_PIN, ledState); 
    digitalWrite(BUZZER_PIN, LOW);          // Buzzer OFF during normal condition
  }

  // LCD Display 
  lcd.setCursor(0, 0);
  lcd.print("Vt:");
  if (threshold_voltage < 10.0) lcd.print("");
  lcd.print(threshold_voltage, 1);
  lcd.print("V ");
  
  lcd.print("It:");
  if (threshold_current < 100.0) lcd.print(" ");
  if (threshold_current < 10.0) lcd.print(" ");
  lcd.print((int)threshold_current);
  lcd.print("mA");

  // Row 1: Print Status Condition
  lcd.setCursor(0, 1);
  if (voltageFault && currentFault) {
    lcd.print("OVER-V & OVER-I ");
  } else if (voltageFault) {
    lcd.print("OVERVOLTAGE     ");
  } else if (currentFault) {
    lcd.print("OVERCURRENT     ");
  } else {
    lcd.print("NORMAL          ");
  }

  delay(200);
}
