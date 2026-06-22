/*
 * Automated Car Parking System — Arduino Uno version
 * Simulator: Wokwi (https://wokwi.com) 
 *
 * Arduino Uno pin map:
 *   D2          -> IR Entry sensor (active LOW)
 *   D3          -> IR Exit sensor  (active LOW)
 *   D5          -> Entry servo (PWM-capable pin)
 *   D6          -> Exit servo  (PWM-capable pin)
 *   D7,D8,D9,D10-> Exit keypad buttons (slot 1-4, active LOW, INPUT_PULLUP)
 *                  Slot 5 = press 7+8 together, Slot 6 = press 9+10 together
 *   D11         -> Buzzer
 *   D12         -> LED Green
 *   D13         -> LED Red
 *   A0-A2       -> 7-segment (only 3 bits needed for 0-6, or use full A0-A5)
 *   LCD on 4-bit mode: RS=A3, EN=A4, D4-D7 = A5,... (see lcd object below)
 */

#include <Servo.h>
#include <LiquidCrystal.h>


LiquidCrystal lcd(A3, A4, A5, A2, A1, A0);


// IR sensors
const int PIN_IR_ENTRY = 2;
const int PIN_IR_EXIT  = 3;

// servo 
const int PIN_SERVO_ENTRY = 5;
const int PIN_SERVO_EXIT  = 6;
Servo entryServo;
Servo exitServo;

//exit keypad
const int PIN_KEY1 = 7;
const int PIN_KEY2 = 8;
const int PIN_KEY3 = 9;
const int PIN_KEY4 = 10;

// buzzer , LED : green , LED: red
const int PIN_BUZZER     = 11;
const int PIN_LED_GREEN  = 12;
const int PIN_LED_RED    = 13;


const int TOTAL_SLOTS = 6;
const byte ALL_FREE   = 0x3F;   /* 0011 1111 */
const int GATE_HOLD_MS = 3000;

//bitmask slot allocator
struct ParkingState {
  byte slotMap;
  byte entryCount;
  byte exitCount;
};

ParkingState parking;

byte findFreeSlot(byte map) {
  for (byte i = 0; i < TOTAL_SLOTS; i++) {
    if (map & (1 << i)) return i + 1;
  }
  return 0;
}

void allocateSlot(ParkingState &p, byte slot) {
  p.slotMap &= ~(1 << (slot - 1));
}

void freeSlot(ParkingState &p, byte slot) {
  if (slot >= 1 && slot <= TOTAL_SLOTS)
    p.slotMap |= (1 << (slot - 1));
}

bool slotIsFree(ParkingState &p, byte slot) {
  return (p.slotMap >> (slot - 1)) & 1;
}

byte countFree(byte map) {
   // i have used the brian kernighan algo
  byte count = 0;
  while (map) { map &= (map - 1); count++; }
  return count;
}

bool isFull(ParkingState &p)  { return p.slotMap == 0x00; }
bool isEmpty(ParkingState &p) { return p.slotMap == ALL_FREE; }


void beep(byte times) {
  for (byte i = 0; i < times; i++) {
    digitalWrite(PIN_BUZZER, HIGH); delay(100);
    digitalWrite(PIN_BUZZER, LOW);  delay(100);
  }
}



// keypad , this returns the slot number
byte readExitKey() {
  delay(20);  

  bool k1 = digitalRead(PIN_KEY1) == LOW;
  bool k2 = digitalRead(PIN_KEY2) == LOW;
  bool k3 = digitalRead(PIN_KEY3) == LOW;
  bool k4 = digitalRead(PIN_KEY4) == LOW;

  if (k1 && k2) { while (digitalRead(PIN_KEY1)==LOW || digitalRead(PIN_KEY2)==LOW); return 5; }
  if (k3 && k4) { while (digitalRead(PIN_KEY3)==LOW || digitalRead(PIN_KEY4)==LOW); return 6; }
  if (k1) { while (digitalRead(PIN_KEY1)==LOW); return 1; }
  if (k2) { while (digitalRead(PIN_KEY2)==LOW); return 2; }
  if (k3) { while (digitalRead(PIN_KEY3)==LOW); return 3; }
  if (k4) { while (digitalRead(PIN_KEY4)==LOW); return 4; }

  return 0;
}

byte waitForExitKey() {
  unsigned long start = millis();
  lcd.setCursor(0, 1);
  lcd.print("Press slot key  ");

  while (millis() - start < 10000) {  // 10 second timeout
    byte key = readExitKey();
    if (key != 0) return key;
  }
  return 0;
}

//display
void displayUpdate(ParkingState &p) {
  byte freeCount = countFree(p.slotMap);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Free:");
  lcd.print(freeCount);
  lcd.print("/6 Map:");
  for (byte s = 1; s <= TOTAL_SLOTS; s++)
    lcd.print(slotIsFree(p, s) ? 'F' : 'X');

  lcd.setCursor(0, 1);
  if (isFull(p)) {
    lcd.print(" PARKING FULL  ");
    digitalWrite(PIN_LED_GREEN, LOW);
    digitalWrite(PIN_LED_RED, HIGH);
  } 
  else {
    lcd.print("Status: OPEN   ");
    digitalWrite(PIN_LED_GREEN, HIGH);
    digitalWrite(PIN_LED_RED, LOW);
  }
}

void uartLog(ParkingState &p, const char *event, byte slot) {
  Serial.print("[");
  Serial.print(event);
  Serial.print("] Slot:");
  Serial.print(slot);
  Serial.print(" | Map:");
   
  for (byte s = 1; s <= TOTAL_SLOTS; s++)
    Serial.print(slotIsFree(p, s) ? 'F' : 'X');
   
  Serial.print(" | Free:");
  Serial.print(countFree(p.slotMap));
  Serial.print(" | In:");
  Serial.print(p.entryCount);
  Serial.print(" Out:");
  Serial.println(p.exitCount);
}

// when a vehicle enters 
// sensors detect a vehicle
void handleEntry(ParkingState &p) {
  if (isFull(p)) {
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print(" PARKING FULL  ");
    lcd.setCursor(0, 1); lcd.print(" Please wait...");
    beep(3);
    delay(2000);
    displayUpdate(p);
    Serial.println("[ENTRY DENIED] Parking full");
    return;
  }

  byte slot = findFreeSlot(p.slotMap);
  allocateSlot(p, slot);
  p.entryCount++;

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(" WELCOME!      ");
  lcd.setCursor(0, 1); lcd.print(" Go to Slot ");
  lcd.print(slot);
  beep(1);

  entryServo.write(90);   //open
  delay(GATE_HOLD_MS);
  entryServo.write(0);    //close

  displayUpdate(p);
  uartLog(p, "ENTRY", slot);
}


//when sensor detects a vehicle
// exit of a vehicle
void handleExit(ParkingState &p) {
  if (isEmpty(p)) {
    Serial.println("[EXIT ERROR] No cars parked");
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(" EXIT: Enter   ");

  byte slot = waitForExitKey();

  if (slot == 0) {
    lcd.setCursor(0, 1); lcd.print("Timeout! Retry ");
    delay(1500);
    displayUpdate(p);
    return;
  }

  if (slotIsFree(p, slot)) {
    lcd.setCursor(0, 0);
    lcd.print("Slot ");
    lcd.print(slot);
    lcd.print(" is empty!");
    lcd.setCursor(0, 1);
    lcd.print("Check ticket    ");
    beep(2);
    delay(2000);
    displayUpdate(p);
    Serial.print("[EXIT ERROR] Slot ");
    Serial.print(slot);
    Serial.println(" was already free");
    return;
  }

  freeSlot(p, slot);
  p.exitCount++;

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(" GOODBYE!      ");
  lcd.setCursor(0, 1); lcd.print(" Gate Opening..");
  beep(1);

  exitServo.write(90);
  delay(GATE_HOLD_MS);
  exitServo.write(0);

  displayUpdate(p);
  uartLog(p, "EXIT ", slot);
}



// setup 
void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);

  pinMode(PIN_IR_ENTRY, INPUT_PULLUP);
  pinMode(PIN_IR_EXIT,  INPUT_PULLUP);
  pinMode(PIN_KEY1, INPUT_PULLUP);
  pinMode(PIN_KEY2, INPUT_PULLUP);
  pinMode(PIN_KEY3, INPUT_PULLUP);
  pinMode(PIN_KEY4, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);

  entryServo.attach(PIN_SERVO_ENTRY);
  exitServo.attach(PIN_SERVO_EXIT);
  entryServo.write(0);
  exitServo.write(0);

  parking.slotMap    = ALL_FREE;
  parking.entryCount = 0;
  parking.exitCount  = 0;

  digitalWrite(PIN_LED_GREEN, HIGH);
  digitalWrite(PIN_LED_RED, LOW);

  lcd.setCursor(0, 0); lcd.print("  Car Parking  ");
  lcd.setCursor(0, 1); lcd.print("  System     ");
  beep(3);
  delay(2000);

  displayUpdate(parking);

  Serial.println("=== Car Parking System (Arduino Uno) ===");
  Serial.print("slotMap = 0x");
  Serial.println(parking.slotMap, HEX);
}



// loop
void loop() {
  if (digitalRead(PIN_IR_ENTRY) == LOW) {
    delay(50);
    if (digitalRead(PIN_IR_ENTRY) == LOW) {
      handleEntry(parking);
      while (digitalRead(PIN_IR_ENTRY) == LOW);
      delay(50);
    }
  }

  if (digitalRead(PIN_IR_EXIT) == LOW) {
    delay(50);
    if (digitalRead(PIN_IR_EXIT) == LOW) {
      handleExit(parking);
      while (digitalRead(PIN_IR_EXIT) == LOW);
      delay(50);
    }
  }
}
