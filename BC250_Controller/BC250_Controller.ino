/*
  BC250 Auto-Off Controller
  Hardware: LOLIN/WEMOS ESP32-S2 Mini + 2N7000

  GPIO5: momentary user button to GND (INPUT_PULLUP)
  GPIO7: PSU hold control -> 100 ohm -> 2N7000 Gate
  GPIO9: BC-250 state input from TPMS1 pin 9

  Behavior:
  - Boot safe: GPIO7 LOW.
  - Short button press from POWER_OFF -> GPIO7 HIGH, WAITING_FOR_BC250.
  - GPIO9 HIGH stable 500 ms -> BC250_RUNNING, AUTO-OFF armed.
  - In BC250_RUNNING, GPIO9 LOW continuous 3000 ms -> PSU off.
  - GPIO9 HIGH before 3000 ms cancels shutdown.
  - New continuous 5000 ms button hold while PSU is on -> manual force off.
*/

#include <Arduino.h>

static constexpr uint8_t PIN_BUTTON = 5;
static constexpr uint8_t PIN_PSU_HOLD = 7;
static constexpr uint8_t PIN_BC250_STATE = 9;

static constexpr uint32_t DEBOUNCE_MS = 40;
static constexpr uint32_t BC250_ON_STABLE_MS = 500;
static constexpr uint32_t BC250_OFF_STABLE_MS = 3000;
static constexpr uint32_t FORCE_OFF_HOLD_MS = 5000;

enum class ControllerState : uint8_t {
  POWER_OFF,
  WAITING_FOR_BC250,
  BC250_RUNNING
};

ControllerState state = ControllerState::POWER_OFF;

bool rawButton = HIGH;
bool stableButton = HIGH;
uint32_t rawButtonChangedAt = 0;
uint32_t buttonPressedAt = 0;
bool buttonPressStartedWhilePowered = false;
bool forceOffHandled = false;

uint32_t bc250HighSince = 0;
uint32_t bc250LowSince = 0;

void setPsu(bool on) {
  digitalWrite(PIN_PSU_HOLD, on ? HIGH : LOW);
}

void powerOff() {
  setPsu(false);
  state = ControllerState::POWER_OFF;
  bc250HighSince = 0;
  bc250LowSince = 0;
  buttonPressStartedWhilePowered = false;
  forceOffHandled = false;
}

void setup() {
  pinMode(PIN_PSU_HOLD, OUTPUT);
  digitalWrite(PIN_PSU_HOLD, LOW); // Critical safe boot state.

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_BC250_STATE, INPUT);

  rawButton = digitalRead(PIN_BUTTON);
  stableButton = rawButton;
  rawButtonChangedAt = millis();

  state = ControllerState::POWER_OFF;
}

void loop() {
  const uint32_t now = millis();

  // Debounce GPIO5.
  const bool sampledButton = digitalRead(PIN_BUTTON);
  if (sampledButton != rawButton) {
    rawButton = sampledButton;
    rawButtonChangedAt = now;
  }

  if ((now - rawButtonChangedAt) >= DEBOUNCE_MS && stableButton != rawButton) {
    stableButton = rawButton;

    if (stableButton == LOW) {
      // A new physical press begins.
      buttonPressedAt = now;
      forceOffHandled = false;
      buttonPressStartedWhilePowered = (state != ControllerState::POWER_OFF);

      if (state == ControllerState::POWER_OFF) {
        setPsu(true);
        state = ControllerState::WAITING_FOR_BC250;
        bc250HighSince = 0;
        bc250LowSince = 0;

        // This same startup press must never become a force-off hold.
        buttonPressStartedWhilePowered = false;
      }
    } else {
      // Button released.
      buttonPressedAt = 0;
      buttonPressStartedWhilePowered = false;
      forceOffHandled = false;
    }
  }

  // Manual force-off only for a NEW press that began while already powered.
  if (stableButton == LOW &&
      buttonPressStartedWhilePowered &&
      !forceOffHandled &&
      state != ControllerState::POWER_OFF &&
      (now - buttonPressedAt) >= FORCE_OFF_HOLD_MS) {
    forceOffHandled = true;
    powerOff();
    return;
  }

  const bool bc250On = (digitalRead(PIN_BC250_STATE) == HIGH);

  switch (state) {
    case ControllerState::POWER_OFF:
      setPsu(false);
      bc250HighSince = 0;
      bc250LowSince = 0;
      break;

    case ControllerState::WAITING_FOR_BC250:
      // Keep PSU on even while GPIO9 is LOW.
      setPsu(true);
      bc250LowSince = 0;

      if (bc250On) {
        if (bc250HighSince == 0) {
          bc250HighSince = now;
        } else if ((now - bc250HighSince) >= BC250_ON_STABLE_MS) {
          state = ControllerState::BC250_RUNNING;
          bc250HighSince = 0;
          bc250LowSince = 0;
        }
      } else {
        bc250HighSince = 0;
      }
      break;

    case ControllerState::BC250_RUNNING:
      setPsu(true);
      bc250HighSince = 0;

      if (!bc250On) {
        if (bc250LowSince == 0) {
          bc250LowSince = now;
        } else if ((now - bc250LowSince) >= BC250_OFF_STABLE_MS) {
          powerOff();
          return;
        }
      } else {
        // Cancel a pending shutdown if the state signal recovers.
        bc250LowSince = 0;
      }
      break;
  }
}
