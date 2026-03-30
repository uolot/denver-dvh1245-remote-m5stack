// =============================================================================
// Denver DVH-1245 IR Remote (GPIO 21)
// =============================================================================
// NEC ALT encoding, address 0x00FF
// Send ONCE per button press (no repeats — DVD repeats internally)
//
// Controls:
//   A / B / C          = primary commands
//   Hold C + A / B     = secondary commands
//   Hold A + B / C     = prev / next page
// =============================================================================

#include <M5Stack.h>
#include <IRsend.h>

const uint16_t IR_TX_PIN = 21;
IRsend irsend(IR_TX_PIN, false, true);

const unsigned long HOLD_MS = 400;

uint32_t buildAlt(uint16_t addr, uint8_t cmd) {
  return ((uint32_t)addr << 16) | ((uint32_t)cmd << 8) | (uint8_t)(~cmd);
}

void sendCmd(uint8_t cmd) {
  uint32_t code = buildAlt(0x00FF, cmd);
  irsend.sendNEC(code, 32);
  Serial.printf("TX: 0x%02X -> 0x%08lX\n", cmd, (unsigned long)code);
}

// 0xFF = no command assigned
const uint8_t NONE = 0xFF;

struct Page {
  const char* name;
  const char* labelA;  uint8_t codeA;   // press A
  const char* labelB;  uint8_t codeB;   // press B
  const char* labelC;  uint8_t codeC;   // press C
  const char* labelCA; uint8_t codeCA;  // hold C + press A
  const char* labelCB; uint8_t codeCB;  // hold C + press B
};

Page pages[] = {
  { "Navigate",
    "LEFT",  0x30,
    "RIGHT", 0xA0,
    "OK",    0x29,
    "UP",    0x40,
    "DOWN",  0x80 },

  { "Playback",
    "PLAY/PAUSE", 0xC0,
    "VOL+",       0xEA,
    "VOL-",       0xB2,
    "STOP",       0xC8,
    "MUTE",       0x00 },

  { "Extras",
    "POWER",    0x20,
    "EJECT",    0x0E,
    "SETTINGS", 0x05,
    NULL, NONE,
    NULL, NONE },
};

const int NUM_PAGES = sizeof(pages) / sizeof(pages[0]);
int currentPage = 0;

// Button state tracking
bool aHeld = false;   // A is being held as modifier
bool cHeld = false;   // C is being held as modifier
bool aUsedAsModifier = false;  // A hold was consumed by a combo
bool cUsedAsModifier = false;  // C hold was consumed by a combo
unsigned long aPressTime = 0;
unsigned long cPressTime = 0;

void drawPage() {
  M5.Lcd.fillScreen(BLACK);
  Page& p = pages[currentPage];

  // Header
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 6);
  M5.Lcd.printf("[%d/%d] %s", currentPage + 1, NUM_PAGES, p.name);
  M5.Lcd.drawLine(0, 28, 320, 28, DARKGREY);

  // Primary commands — large
  M5.Lcd.setTextColor(YELLOW, BLACK);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(10, 38);
  M5.Lcd.printf("A:%s", p.labelA);
  M5.Lcd.setCursor(10, 70);
  M5.Lcd.printf("B:%s", p.labelB);
  M5.Lcd.setCursor(10, 102);
  M5.Lcd.printf("C:%s", p.labelC);

  // Secondary commands (hold C + A/B)
  if (p.labelCA || p.labelCB) {
    M5.Lcd.drawLine(0, 134, 320, 134, DARKGREY);
    M5.Lcd.setTextColor(CYAN, BLACK);
    M5.Lcd.setTextSize(2);
    int y = 142;
    if (p.labelCA) {
      M5.Lcd.setCursor(10, y);
      M5.Lcd.printf("C+A: %s", p.labelCA);
    }
    if (p.labelCB) {
      M5.Lcd.setCursor(170, y);
      M5.Lcd.printf("C+B: %s", p.labelCB);
    }
  }

  // Page nav hint
  M5.Lcd.setTextColor(DARKGREY, BLACK);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(65, 200);
  M5.Lcd.print("A+B: prev page   A+C: next page");

  // Bottom bar
  M5.Lcd.fillRect(0, 220, 320, 20, DARKGREY);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE, DARKGREY);
  int w = 320 / 3;
  const char* labels[] = { p.labelA, p.labelB, p.labelC };
  for (int i = 0; i < 3; i++) {
    int x = i * w + (w - strlen(labels[i]) * 6) / 2;
    M5.Lcd.setCursor(x, 225);
    M5.Lcd.print(labels[i]);
  }
}

void flashSent(const char* label) {
  M5.Lcd.fillRect(0, 168, 320, 28, BLACK);
  M5.Lcd.setTextColor(GREEN, BLACK);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(10, 168);
  M5.Lcd.printf("> %s", label);
}

void changePage(int delta) {
  currentPage = (currentPage + delta + NUM_PAGES) % NUM_PAGES;
  drawPage();
}

void setup() {
  M5.begin();
  M5.Power.begin();
  Serial.begin(115200);
  irsend.begin();
  Serial.println("DVH-1245 Remote");
  drawPage();
}

void loop() {
  M5.update();
  Page& p = pages[currentPage];

  // --- Track A hold state ---
  if (M5.BtnA.wasPressed()) {
    aPressTime = millis();
    aHeld = false;
    aUsedAsModifier = false;
  }
  if (M5.BtnA.isPressed() && !aHeld && millis() - aPressTime > HOLD_MS) {
    aHeld = true;
  }

  // --- Track C hold state ---
  if (M5.BtnC.wasPressed()) {
    cPressTime = millis();
    cHeld = false;
    cUsedAsModifier = false;
  }
  if (M5.BtnC.isPressed() && !cHeld && millis() - cPressTime > HOLD_MS) {
    cHeld = true;
  }

  // --- Hold A + B = prev page ---
  if (aHeld && M5.BtnB.wasPressed()) {
    aUsedAsModifier = true;
    changePage(-1);
  }

  // --- Hold A + C = next page ---
  if (aHeld && M5.BtnC.wasPressed()) {
    aUsedAsModifier = true;
    changePage(+1);
  }

  // --- Hold C + A = secondary command CA ---
  if (cHeld && M5.BtnA.wasPressed()) {
    cUsedAsModifier = true;
    if (p.codeCA != NONE) {
      sendCmd(p.codeCA);
      flashSent(p.labelCA);
    }
  }

  // --- Hold C + B = secondary command CB ---
  if (cHeld && M5.BtnB.wasPressed()) {
    cUsedAsModifier = true;
    if (p.codeCB != NONE) {
      sendCmd(p.codeCB);
      flashSent(p.labelCB);
    }
  }

  // --- A released: send cmdA if it wasn't used as modifier ---
  if (M5.BtnA.wasReleased()) {
    if (!aUsedAsModifier && !aHeld && !cHeld) {
      sendCmd(p.codeA);
      flashSent(p.labelA);
    }
    aHeld = false;
    aUsedAsModifier = false;
  }

  // --- B released: send cmdB if no modifier was active ---
  if (M5.BtnB.wasReleased()) {
    if (!aHeld && !cHeld) {
      sendCmd(p.codeB);
      flashSent(p.labelB);
    }
  }

  // --- C released: send cmdC if it wasn't used as modifier ---
  if (M5.BtnC.wasReleased()) {
    if (!cUsedAsModifier && !cHeld && !aHeld) {
      sendCmd(p.codeC);
      flashSent(p.labelC);
    }
    cHeld = false;
    cUsedAsModifier = false;
  }

  delay(5);
}
