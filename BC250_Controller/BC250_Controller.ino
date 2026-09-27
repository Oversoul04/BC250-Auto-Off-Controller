#include <Arduino.h>

static const uint8_t GPIO5_PIN = 5;
static const uint8_t GPIO7_PIN = 7;
static const uint8_t GPIO9_PIN = 9;
static const uint8_t LED_PIN = 15;

enum ControllerState { POWER_OFF, WAITING_FOR_BC250, BC250_RUNNING };
ControllerState state = POWER_OFF;

const uint32_t DEBOUNCE_MS = 40;
const uint32_t ARM_HIGH_MS = 500;
const uint32_t AUTO_OFF_LOW_MS = 3000;
const uint32_t MANUAL_OFF_MS = 5000;

bool rawButton = HIGH;
bool stableButton = HIGH;
uint32_t buttonChangedAt = 0;
uint32_t activePressStartedAt = 0;
bool activePressIsNew = false;

uint32_t gpio9HighSince = 0;
uint32_t gpio9LowSince = 0;

const char *stateName() {
  switch (state) {
    case POWER_OFF: return "POWER_OFF";
    case WAITING_FOR_BC250: return "WAITING_FOR_BC250";
    case BC250_RUNNING: return "BC250_RUNNING";
  }
  return "UNKNOWN";
}

void setPowerHold(bool on) {
  digitalWrite(GPIO7_PIN, on ? HIGH : LOW);
}

void enterPowerOff() {
  setPowerHold(false);
  state = POWER_OFF;
  gpio9HighSince = 0;
  gpio9LowSince = 0;
  activePressStartedAt = 0;
  activePressIsNew = false;
}

void updateButton(uint32_t now) {
  const bool reading = digitalRead(GPIO5_PIN);
  if (reading != rawButton) {
    rawButton = reading;
    buttonChangedAt = now;
  }
  if ((now - buttonChangedAt) < DEBOUNCE_MS || reading == stableButton) {
    return;
  }

  stableButton = reading;
  if (stableButton == LOW) {
    activePressStartedAt = now;
    activePressIsNew = (state != POWER_OFF);
    if (state == POWER_OFF) {
      setPowerHold(true);
      state = WAITING_FOR_BC250;
      activePressIsNew = false;
    }
  } else if (activePressStartedAt != 0) {
    activePressStartedAt = 0;
    activePressIsNew = false;
  }
}

void updateManualOff(uint32_t now) {
  if (state == POWER_OFF || stableButton != LOW || activePressStartedAt == 0 || !activePressIsNew) {
    return;
  }
  if ((now - activePressStartedAt) >= MANUAL_OFF_MS) {
    enterPowerOff();
  }
}

void updateBc250(uint32_t now) {
  const bool bc250High = digitalRead(GPIO9_PIN) == HIGH;
  if (state == WAITING_FOR_BC250) {
    gpio9LowSince = 0;
    if (bc250High) {
      if (gpio9HighSince == 0) gpio9HighSince = now;
      if ((now - gpio9HighSince) >= ARM_HIGH_MS) {
        state = BC250_RUNNING;
        gpio9LowSince = 0;
      }
    } else {
      gpio9HighSince = 0;
    }
    return;
  }

  if (state == BC250_RUNNING) {
    gpio9HighSince = 0;
    if (bc250High) {
      gpio9LowSince = 0;
    } else {
      if (gpio9LowSince == 0) gpio9LowSince = now;
      if ((now - gpio9LowSince) >= AUTO_OFF_LOW_MS) {
        enterPowerOff();
      }
    }
  }
}

void setup() {
  pinMode(GPIO7_PIN, OUTPUT);
  digitalWrite(GPIO7_PIN, LOW);
  pinMode(GPIO5_PIN, INPUT_PULLUP);
  pinMode(GPIO9_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.begin(115200);
  Serial.println("BC250_CONTROLLER");
  Serial.println("GPIO7_INITIAL = LOW");
}

void loop() {
  const uint32_t now = millis();
  updateButton(now);
  updateManualOff(now);
  updateBc250(now);
  digitalWrite(LED_PIN, state == POWER_OFF ? LOW : HIGH);
  Serial.print("STATE = ");
  Serial.print(stateName());
  Serial.print(" | GPIO9 = ");
  Serial.print(digitalRead(GPIO9_PIN));
  Serial.print(" | GPIO5 = ");
  Serial.print(digitalRead(GPIO5_PIN));
  Serial.print(" | GPIO7 = ");
  Serial.println(digitalRead(GPIO7_PIN));
  delay(5);
}
