#include <SoftwareSerial.h>
#include <IRremote.hpp>

SoftwareSerial BTSerial(10, 11); // RX, TX

// ===================== PINS =====================
const int lampPin = 7;
const int lampButtonPin = 4;
const int buzzerPin = 5;

#define IR_RECEIVE_PIN 2
#define IR_SEND_PIN 3

// Physical buttons
const int tvToggleButtonPin = A1;
const int buzzerToggleButtonPin = A2;
const int tvMuteButtonPin = 8;     // D8: TV mute physical button

// ===================== IR REMOTE CODES =====================
const uint8_t KIT_VOL_UP_CMD        = 0x18;
const uint8_t KIT_VOL_DOWN_CMD      = 0x52;
const uint8_t KIT_OC_CMD            = 0x1C; // KIT power button 1
const uint8_t KIT_OC_CMD2           = 0x47; // KIT power button 2
const uint8_t KIT_LAMP_CMD          = 0x45;
const uint8_t KIT_RGB_CMD           = 0x46;
const uint8_t KIT_BUZZER_TOGGLE_CMD = 0x19;
const uint8_t KIT_MUTE_CMD          = 0x16;
const uint8_t KIT_MUTE_CMD2         = 0xFF;

// Simple IR debounce / lockout
unsigned long irBlockUntil = 0;
const unsigned long IR_ACTION_GAP = 250;

// Samsung TV codes
const uint16_t SAMSUNG_ADDR     = 0x7;
const uint16_t SAMSUNG_VOL_UP   = 0x7;
const uint16_t SAMSUNG_VOL_DOWN = 0xB;
const uint16_t SAMSUNG_POWER    = 0x2;
const uint16_t SAMSUNG_MUTE     = 0xF;

// ===================== TV HOLD =====================
bool tvHoldActive = false;
uint16_t tvHoldCmd = 0;
unsigned long tvNextHoldAt = 0;

// ===================== LAMP / TIMER =====================
bool lampState = true;
bool timerActive = false;
bool turnOnLater = false;
unsigned long targetTime = 0;

// ===================== BUZZER RANDOM MODE =====================
const unsigned long BUZZ_MIN_INTERVAL = 20UL * 60UL * 1000UL;
const unsigned long BUZZ_MAX_INTERVAL = 60UL * 60UL * 1000UL;
const unsigned long BUZZ_MIN_DURATION = 120;
const unsigned long BUZZ_MAX_DURATION = 450;

bool buzzerMasterEnabled = true;
bool randomBuzzEnabled = true;
bool buzzerOn = false;
unsigned long nextBuzzAt = 0;
unsigned long buzzerOffAt = 0;

bool testPending = false;
bool testInProgress = false;
unsigned long testRemainingToNext = 0;

// ===================== BUZZER ALARM MODE =====================
bool alarmArmed = false;
bool alarmRinging = false;
unsigned long alarmTargetAt = 0;
unsigned long alarmPatternNextAt = 0;
bool alarmOutputState = false;

// ===================== HELPERS =====================
void sendStrongSamsung(uint16_t addr, uint16_t cmd) {
  for (int i = 0; i < 3; i++) {
    IrSender.sendSamsung(addr, cmd, 0);
    delay(25);
  }
}

void executeLamp(bool turnOn) {
  lampState = turnOn;
  digitalWrite(lampPin, turnOn ? HIGH : LOW);
  timerActive = false;
}

void startTimer(bool onLater, long seconds) {
  if (seconds < 0) seconds = 0;
  targetTime = millis() + (unsigned long)seconds * 1000UL;
  turnOnLater = onLater;
  timerActive = true;
}

void scheduleNextBuzz() {
  unsigned long interval = (unsigned long)random((long)BUZZ_MIN_INTERVAL, (long)BUZZ_MAX_INTERVAL + 1L);
  nextBuzzAt = millis() + interval;
}

void stopAlarmOnly() {
  alarmArmed = false;
  alarmRinging = false;
  alarmOutputState = false;
  if (!buzzerOn) digitalWrite(buzzerPin, LOW);
}

void stopAllBuzzerActivity() {
  randomBuzzEnabled = false;
  testPending = false;
  testInProgress = false;
  alarmArmed = false;
  alarmRinging = false;
  alarmOutputState = false;
  buzzerOn = false;
  digitalWrite(buzzerPin, LOW);
}

void enableRandomMode() {
  randomBuzzEnabled = true;
  stopAlarmOnly();
  if (nextBuzzAt == 0 || (long)(millis() - nextBuzzAt) >= 0) scheduleNextBuzz();
}

void armAlarmSeconds(unsigned long seconds) {
  alarmArmed = true;
  alarmRinging = false;
  alarmOutputState = false;
  alarmTargetAt = millis() + seconds * 1000UL;
  alarmPatternNextAt = 0;
}

void toggleBuzzerMasterDirect() {
  buzzerMasterEnabled = !buzzerMasterEnabled;

  if (buzzerMasterEnabled) {
    alarmArmed = false;
    alarmRinging = false;
    alarmOutputState = false;
    digitalWrite(buzzerPin, LOW);

    randomBuzzEnabled = true;
    testPending = false;
    testInProgress = false;
    scheduleNextBuzz();
    BTSerial.println("BUZ:1");
  } else {
    stopAllBuzzerActivity();
    BTSerial.println("BUZ:0");
  }
}

bool pressedOnce(int pin) {
  static bool initialized[32];
  static bool lastRead[32];
  static bool stable[32];
  static unsigned long lastChange[32];

  uint8_t idx = (pin <= 19) ? pin : 0;
  if (idx >= 32) idx = 0;

  if (!initialized[idx]) {
    initialized[idx] = true;
    lastRead[idx] = HIGH;
    stable[idx] = HIGH;
    lastChange[idx] = 0;
  }

  const unsigned long debounceMs = 35;
  bool r = digitalRead(pin);

  if (r != lastRead[idx]) {
    lastRead[idx] = r;
    lastChange[idx] = millis();
  }

  if ((millis() - lastChange[idx]) > debounceMs && r != stable[idx]) {
    stable[idx] = r;
    if (stable[idx] == LOW) return true;
  }

  return false;
}

// ===================== HANDLERS =====================
void handleLampButton() {
  if (pressedOnce(lampButtonPin)) {
    executeLamp(!lampState);
    BTSerial.println(lampState ? "STATE:1" : "STATE:0");
  }
}

void handleTvButton() {
  if (pressedOnce(tvToggleButtonPin)) {
    sendStrongSamsung(SAMSUNG_ADDR, SAMSUNG_POWER);
  }
}

void handleTvMuteButton() {
  if (pressedOnce(tvMuteButtonPin)) {
    sendStrongSamsung(SAMSUNG_ADDR, SAMSUNG_MUTE);
  }
}

void handleBuzzerToggleButton() {
  if (pressedOnce(buzzerToggleButtonPin)) {
    toggleBuzzerMasterDirect();
  }
}

void handleTimer() {
  if (!timerActive) return;
  if ((long)(millis() - targetTime) >= 0) {
    executeLamp(turnOnLater);
    timerActive = false;
    BTSerial.println(lampState ? "STATE:1" : "STATE:0");
  }
}

void handleTvHold() {
  if (!tvHoldActive) return;
  unsigned long now = millis();
  if ((long)(now - tvNextHoldAt) >= 0) {
    sendStrongSamsung(SAMSUNG_ADDR, tvHoldCmd);
    tvNextHoldAt = now + 90;
  }
}

void handleIrReceiverForward() {
  if (!IrReceiver.decode()) return;

  unsigned long now = millis();
  uint8_t cmd = IrReceiver.decodedIRData.command;

  if (now < irBlockUntil) { IrReceiver.resume(); return; }

  if (cmd == KIT_VOL_UP_CMD) {
    sendStrongSamsung(SAMSUNG_ADDR, SAMSUNG_VOL_UP);
    irBlockUntil = now + IR_ACTION_GAP;
  }
  else if (cmd == KIT_VOL_DOWN_CMD) {
    sendStrongSamsung(SAMSUNG_ADDR, SAMSUNG_VOL_DOWN);
    irBlockUntil = now + IR_ACTION_GAP;
  }
  else if (cmd == KIT_OC_CMD || cmd == KIT_OC_CMD2) {
    sendStrongSamsung(SAMSUNG_ADDR, SAMSUNG_POWER);
    irBlockUntil = now + IR_ACTION_GAP;
  }
  else if (cmd == KIT_LAMP_CMD) {
    executeLamp(!lampState);
    BTSerial.println(lampState ? "STATE:1" : "STATE:0");
    irBlockUntil = now + IR_ACTION_GAP;
  }
  else if (cmd == KIT_RGB_CMD) {
    if (buzzerMasterEnabled && randomBuzzEnabled && !buzzerOn && !testPending) {
      testPending = true;
    }
    irBlockUntil = now + IR_ACTION_GAP;
  }
  else if (cmd == KIT_BUZZER_TOGGLE_CMD) {
    toggleBuzzerMasterDirect();
    irBlockUntil = now + IR_ACTION_GAP;
  }
  else if (cmd == KIT_MUTE_CMD || cmd == KIT_MUTE_CMD2) {
    sendStrongSamsung(SAMSUNG_ADDR, SAMSUNG_MUTE);
    irBlockUntil = now + IR_ACTION_GAP;
  }

  IrReceiver.resume();
}

void handleBuzzer() {
  unsigned long now = millis();

  if (!buzzerMasterEnabled) {
    digitalWrite(buzzerPin, LOW);
    buzzerOn = false;
    return;
  }

  if (alarmRinging) {
    if ((long)(now - alarmPatternNextAt) >= 0) {
      alarmOutputState = !alarmOutputState;
      digitalWrite(buzzerPin, alarmOutputState ? HIGH : LOW);
      alarmPatternNextAt = now + (alarmOutputState ? 220 : 180);
    }
    return;
  }

  if (alarmArmed && (long)(now - alarmTargetAt) >= 0) {
    alarmArmed = false;
    alarmRinging = true;
    alarmOutputState = false;
    alarmPatternNextAt = 0;
    BTSerial.println("BUZALARM:RINGING");
    return;
  }

  if (testPending && !buzzerOn) {
    testPending = false;
    testInProgress = true;
    if ((long)(nextBuzzAt - now) > 0) testRemainingToNext = nextBuzzAt - now;
    else testRemainingToNext = 0;
    unsigned long dur = (unsigned long)random((long)BUZZ_MIN_DURATION, (long)BUZZ_MAX_DURATION + 1L);
    digitalWrite(buzzerPin, HIGH);
    buzzerOn = true;
    buzzerOffAt = now + dur;
    return;
  }

  if (!randomBuzzEnabled) {
    if (buzzerOn) {
      digitalWrite(buzzerPin, LOW);
      buzzerOn = false;
      testInProgress = false;
    }
    return;
  }

  if (buzzerOn) {
    if ((long)(now - buzzerOffAt) >= 0) {
      digitalWrite(buzzerPin, LOW);
      buzzerOn = false;
      if (testInProgress) {
        testInProgress = false;
        nextBuzzAt = millis() + testRemainingToNext;
      } else {
        scheduleNextBuzz();
      }
    }
    return;
  }

  if ((long)(now - nextBuzzAt) >= 0) {
    unsigned long dur = (unsigned long)random((long)BUZZ_MIN_DURATION, (long)BUZZ_MAX_DURATION + 1L);
    digitalWrite(buzzerPin, HIGH);
    buzzerOn = true;
    buzzerOffAt = now + dur;
  }
}

void handleBluetooth() {
  if (!BTSerial.available()) return;

  String command = BTSerial.readStringUntil('\n');
  command.trim();

  if (command == "1") {
    executeLamp(true);
    BTSerial.println("STATE:1");
  }
  else if (command == "0") {
    executeLamp(false);
    BTSerial.println("STATE:0");
  }
  else if (command.startsWith("ON:")) {
    startTimer(true, command.substring(3).toInt());
  }
  else if (command.startsWith("OFF:")) {
    startTimer(false, command.substring(4).toInt());
  }
  else if (command == "STATE?") {
    BTSerial.println(lampState ? "STATE:1" : "STATE:0");
  }
  else if (command == "BUZ:0") {
    buzzerMasterEnabled = false;
    stopAllBuzzerActivity();
    BTSerial.println("BUZ:0");
  }
  else if (command == "BUZ:1") {
    buzzerMasterEnabled = true;
    randomBuzzEnabled = true;
    if (nextBuzzAt == 0 || (long)(millis() - nextBuzzAt) >= 0) scheduleNextBuzz();
    BTSerial.println("BUZ:1");
  }
  else if (command == "BUZ:TEST") {
    if (buzzerMasterEnabled && randomBuzzEnabled) testPending = true;
    BTSerial.println("BUZ:TEST");
  }
  else if (command == "BUZ:ALARM:STOP") {
    stopAlarmOnly();
    BTSerial.println("BUZALARM:STOPPED");
  }
  else if (command.startsWith("BUZ:ALARM:")) {
    unsigned long sec = (unsigned long)command.substring(10).toInt();
    if (buzzerMasterEnabled && sec > 0) {
      armAlarmSeconds(sec);
      BTSerial.println("BUZALARM:ARMED");
    }
  }
  else if (command == "TV:UP") {
    sendStrongSamsung(SAMSUNG_ADDR, SAMSUNG_VOL_UP);
  }
  else if (command == "TV:DOWN") {
    sendStrongSamsung(SAMSUNG_ADDR, SAMSUNG_VOL_DOWN);
  }
  else if (command == "TV:OC") {
    sendStrongSamsung(SAMSUNG_ADDR, SAMSUNG_POWER);
  }
  else if (command == "TV:MUTE") {
    sendStrongSamsung(SAMSUNG_ADDR, SAMSUNG_MUTE);
  }
  else if (command == "TV:HOLD:UP") {
    tvHoldActive = true;
    tvHoldCmd = SAMSUNG_VOL_UP;
    tvNextHoldAt = 0;
  }
  else if (command == "TV:HOLD:DOWN") {
    tvHoldActive = true;
    tvHoldCmd = SAMSUNG_VOL_DOWN;
    tvNextHoldAt = 0;
  }
  else if (command == "TV:HOLD:STOP") {
    tvHoldActive = false;
  }
}

void setup() {
  pinMode(lampPin, OUTPUT);
  pinMode(lampButtonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  pinMode(tvToggleButtonPin, INPUT_PULLUP);
  pinMode(buzzerToggleButtonPin, INPUT_PULLUP);
  pinMode(tvMuteButtonPin, INPUT_PULLUP);  // D8 mute button

  digitalWrite(buzzerPin, LOW);
  executeLamp(true);

  randomSeed(analogRead(A0));
  scheduleNextBuzz();

  BTSerial.begin(9600);
  BTSerial.setTimeout(10);
  Serial.begin(9600);

  IrReceiver.begin(IR_RECEIVE_PIN);
  IrSender.begin(IR_SEND_PIN);

  BTSerial.println("STATE:1");
  BTSerial.println("BUZ:1");
}

void loop() {
  handleLampButton();
  handleTvButton();
  handleTvMuteButton();        // D8 mute button
  handleBuzzerToggleButton();

  handleBluetooth();
  handleTimer();
  handleBuzzer();

  handleTvHold();
  handleIrReceiverForward();
}