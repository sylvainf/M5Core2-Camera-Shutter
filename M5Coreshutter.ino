/*
 * M5Core2 Camera Shutter Controller
 * 
 * Large format camera shutter controller using servo motor
 * Features: Timed exposures, manual mode, 10s timer, battery indicator
 * 
 * Hardware: M5Stack Core2 (ESP32)
 * Servo: Connected to Port A - Pin 32 (white wire) or 33 (yellow wire)
 * 
 * Author: Sylvain Ferrand
 * Version: 1.0
 * Date: February 2026
 * License: MIT
 * 
 * GitHub: https://github.com/sylvainf/M5Core2-Camera-Shutter
 */

#include <M5Unified.h>
#include <ESP32Servo.h>

// --- SERVO CONFIGURATION ---
Servo myServo;
const int servoPin = 32;

const int SERVO_MIN = 0;   // Minimum angle (closed)
const int SERVO_MAX = 180; // Maximum angle (open)

// --- DURATION VALUES ---
float durations[] = {0.2, 0.3, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0};
int durationsCount = 8;
int selectedIndex = 3; // Default selection (1.0 second)

// --- MANUAL MODE STATE ---
bool manualIsOpen = false; // false = closed, true = open

// --- TIMER STATE ---
unsigned long timerStartTime = 0;
bool timerActive = false;

// --- BATTERY DISPLAY ---
unsigned long lastBatteryUpdate = 0;
const unsigned long BATTERY_UPDATE_INTERVAL = 5000; // Update every 5 seconds

// --- INTERFACE COORDINATES ---
int gridX = 10;
int gridY = 30; // Moved down to make room for header
int btnWidth = 70;
int btnHeight = 50;
int gap = 5;

int trigX = 10;
int trigY = 160;
int trigW = 145;
int trigH = 70;

// Manual button
int manualX = 165;
int manualY = 160;
int manualW = 145;
int manualH = 32;

// Timer button
int timerX = 165;
int timerY = 198;
int timerW = 145;
int timerH = 32;

// --- FUNCTION DECLARATIONS ---
void drawInterface();
void drawHeader();
void drawBatteryIndicator();
void drawTriggerButton();
void drawManualButton();
void drawTimerButton();
void checkDurationButtons(int x, int y);
void checkTriggerButton(int x, int y);
void checkManualButton(int x, int y);
void checkTimerButton(int x, int y);
void activateServo();
void toggleManual();
void startTimer();

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  // Allocate timers for ESP32Servo
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  myServo.setPeriodHertz(50); 

  // Initialization at startup: Set to 0, then detach
  myServo.attach(servoPin, 500, 2400);
  myServo.write(SERVO_MIN);
  delay(500); // Give the servo time to reach position 0
  myServo.detach(); // Cut the signal for silence

  M5.Display.fillScreen(BLACK);
  drawInterface();
}

void loop() {
  M5.update(); 

  // Battery update (every 5 seconds)
  if (millis() - lastBatteryUpdate > BATTERY_UPDATE_INTERVAL) {
    lastBatteryUpdate = millis();
    drawBatteryIndicator();
  }

  // Timer management
  if (timerActive) {
    unsigned long elapsed = millis() - timerStartTime;

    if (elapsed >= 10000) {
      // Timer finished - trigger shutter
      timerActive = false;
      activateServo();
      drawTimerButton(); // Redraw button after activation
    } else {
      // Update countdown display
      drawTimerButton();
    }
  }

  if (M5.Touch.getCount() > 0) {
    auto t = M5.Touch.getDetail(0);

    if (t.wasPressed()) {
      checkDurationButtons(t.x, t.y);
      checkTriggerButton(t.x, t.y);
      checkManualButton(t.x, t.y);
      checkTimerButton(t.x, t.y);
    }
  }

  delay(100); // 100ms for smoother countdown display
}

// --- LOGIC FUNCTIONS ---

void activateServo() {
  // 1. Visual feedback
  M5.Display.fillRect(trigX, trigY, trigW, trigH, ORANGE);
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(2);
  M5.Display.drawCenterString("TRIGGERING", trigX + trigW/2, trigY + 20);

  // 2. Attach servo to control it
  if (!myServo.attached()) {
    myServo.attach(servoPin, 500, 2400);
  }

  // 3. Action: Go to Max
  myServo.write(SERVO_MAX);

  // 4. Wait for selected duration
  long waitTime = (long)(durations[selectedIndex] * 1000);
  delay(waitTime);

  // 5. Action: Return to Min
  myServo.write(SERVO_MIN);

  // 6. Wait 1 second 
  delay(500);

  // 7. Cut signal (detach)
  myServo.detach();

  // 8. Restore interface
  drawTriggerButton();
}

void toggleManual() {
  // Attach servo if needed
  if (!myServo.attached()) {
    myServo.attach(servoPin, 500, 2400);
  }

  if (manualIsOpen) {
    // Close: return to MIN
    myServo.write(SERVO_MIN);
    manualIsOpen = false;
  } else {
    // Open: go to MAX
    myServo.write(SERVO_MAX);
    manualIsOpen = true;
  }

  // Wait 1 second then detach
  delay(500);
  myServo.detach();

  // Update button display
  drawManualButton();
}

void startTimer() {
  if (!timerActive) {
    timerActive = true;
    timerStartTime = millis();
    drawTimerButton(); // Visual update
  }
}

void checkTriggerButton(int x, int y) {
  if (x > trigX && x < trigX + trigW && y > trigY && y < trigY + trigH) {
    activateServo();
  }
}

void checkManualButton(int x, int y) {
  if (x > manualX && x < manualX + manualW && y > manualY && y < manualY + manualH) {
    toggleManual();
  }
}

void checkTimerButton(int x, int y) {
  if (x > timerX && x < timerX + timerW && y > timerY && y < timerY + timerH) {
    startTimer();
  }
}

void checkDurationButtons(int x, int y) {
  int col = 0;
  int row = 0;

  for (int i = 0; i < durationsCount; i++) {
    // Calculate for 2 rows of 4 buttons
    col = i % 4;
    row = i / 4;

    int bx = gridX + col * (btnWidth + gap);
    int by = gridY + row * (btnHeight + gap);

    if (x > bx && x < bx + btnWidth && y > by && y < by + btnHeight) {
      if (selectedIndex != i) {
        selectedIndex = i;
        drawInterface(); 
      }
    }
  }
}

// --- GRAPHICS FUNCTIONS ---

void drawInterface() {
  M5.Display.fillScreen(BLACK);

  drawHeader();
  drawBatteryIndicator();

  int col = 0;
  int row = 0;

  M5.Display.setTextSize(2);

  for (int i = 0; i < durationsCount; i++) {
    col = i % 4;
    row = i / 4;

    int bx = gridX + col * (btnWidth + gap);
    int by = gridY + row * (btnHeight + gap);

    uint16_t color = (i == selectedIndex) ? GREEN : DARKGREY;
    uint16_t textColor = (i == selectedIndex) ? BLACK : WHITE;

    M5.Display.fillRect(bx, by, btnWidth, btnHeight, color);
    M5.Display.drawRect(bx, by, btnWidth, btnHeight, WHITE);

    M5.Display.setTextColor(textColor);

    String label;
    if (durations[i] == (int)durations[i]) {
       label = String((int)durations[i]) + "s";
    } else {
       label = String(durations[i], 1) + "s";
    }

    M5.Display.setCursor(bx + 10, by + 15);
    M5.Display.print(label);
  }

  drawTriggerButton();
  drawManualButton();
  drawTimerButton();
}

void drawHeader() {
  // Header bar
  M5.Display.fillRect(0, 0, 320, 25, NAVY);

  // Title
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(1);
  M5.Display.setCursor(5, 8);
  M5.Display.print("CAMERA SHUTTER");
}

void drawBatteryIndicator() {
  // Get battery info
  int batteryLevel = M5.Power.getBatteryLevel();
  bool isCharging = M5.Power.isCharging();

  // Battery icon position (top right)
  int batX = 260;
  int batY = 5;
  int batW = 50;
  int batH = 15;

  // Clear previous battery area
  M5.Display.fillRect(batX, batY, batW + 5, batH, NAVY);

  // Draw battery outline
  M5.Display.drawRect(batX, batY, batW, batH, WHITE);
  M5.Display.fillRect(batX + batW, batY + 4, 3, 7, WHITE); // Battery tip

  // Fill battery based on level
  uint16_t fillColor;
  if (batteryLevel > 60) {
    fillColor = GREEN;
  } else if (batteryLevel > 20) {
    fillColor = YELLOW;
  } else {
    fillColor = RED;
  }

  int fillWidth = (batW - 4) * batteryLevel / 100;
  if (fillWidth > 0) {
    M5.Display.fillRect(batX + 2, batY + 2, fillWidth, batH - 4, fillColor);
  }

  // Charging indicator
  if (isCharging) {
    M5.Display.setTextColor(YELLOW);
    M5.Display.setTextSize(1);
    M5.Display.setCursor(batX + batW + 8, batY + 3);
    M5.Display.print("+");
  }
}

void drawTriggerButton() {
  M5.Display.fillRect(trigX, trigY, trigW, trigH, RED);
  M5.Display.drawRect(trigX, trigY, trigW, trigH, WHITE);

  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(2);
  M5.Display.drawCenterString("TRIGGER", trigX + trigW/2, trigY + 15);

  M5.Display.setTextSize(1);
  String subText = "(Duration: " + String(durations[selectedIndex]) + "s)";
  M5.Display.drawCenterString(subText, trigX + trigW/2, trigY + 45);
}

void drawManualButton() {
  uint16_t color = manualIsOpen ? BLUE : DARKGREY;
  M5.Display.fillRect(manualX, manualY, manualW, manualH, color);
  M5.Display.drawRect(manualX, manualY, manualW, manualH, WHITE);

  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(2);
  String label = manualIsOpen ? "OPEN" : "MANUAL";
  M5.Display.drawCenterString(label, manualX + manualW/2, manualY + 8);
}

void drawTimerButton() {
  uint16_t color = timerActive ? ORANGE : DARKGREEN;
  M5.Display.fillRect(timerX, timerY, timerW, timerH, color);
  M5.Display.drawRect(timerX, timerY, timerW, timerH, WHITE);

  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(2);

  if (timerActive) {
    // Display countdown in seconds
    unsigned long elapsed = millis() - timerStartTime;
    unsigned long remaining = 10 - (elapsed / 1000);
    String label = String(remaining) + "s";
    M5.Display.drawCenterString(label, timerX + timerW/2, timerY + 8);
  } else {
    M5.Display.drawCenterString("TIMER 10s", timerX + timerW/2, timerY + 8);
  }
}