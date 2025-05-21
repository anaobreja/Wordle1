 #include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// --- TFT Display Pins ---
#define PIN_CLK 13
#define PIN_MOSI 11
#define PIN_RES 8
#define PIN_DC   9
#define PIN_CS1 10

// --- PS/2 Keyboard Pins ---
#define PS2_DATA 4
#define PS2_CLK  3

Adafruit_ILI9341 display = Adafruit_ILI9341(PIN_CS1, PIN_DC, PIN_RES);

// --- PS/2 handling ---
volatile byte ps2_data = 0;
volatile bool ps2_data_ready = false;
volatile byte bitCount = 0;

void ps2_clk_isr() {
  static byte data = 0;
  static byte bitsRead = 0;
  bool bit = digitalRead(PS2_DATA);

  if (bitCount == 0) {
    data = 0;
    bitsRead = 0;
  } else if (bitCount >= 1 && bitCount <= 8) {
    data |= (bit << (bitCount - 1));
  }

  bitCount++;
  if (bitCount == 11) {
    ps2_data = data;
    ps2_data_ready = true;
    bitCount = 0;
  }
}

bool ps2KeyAvailable() {
  return ps2_data_ready;
}

char decodeScancode(byte code) {
  switch (code) {
    case 0x1C: return 'A'; case 0x32: return 'B'; case 0x21: return 'C';
    case 0x23: return 'D'; case 0x24: return 'E'; case 0x2B: return 'F';
    case 0x34: return 'G'; case 0x33: return 'H'; case 0x43: return 'I';
    case 0x3B: return 'J'; case 0x42: return 'K'; case 0x4B: return 'L';
    case 0x3A: return 'M'; case 0x31: return 'N'; case 0x44: return 'O';
    case 0x4D: return 'P'; case 0x15: return 'Q'; case 0x2D: return 'R';
    case 0x1B: return 'S'; case 0x2C: return 'T'; case 0x3C: return 'U';
    case 0x2A: return 'V'; case 0x1D: return 'W'; case 0x22: return 'X';
    case 0x35: return 'Y'; case 0x1A: return 'Z';
    case 0x5A: return '\n'; // Enter
    case 0x66: return '\b'; // Backspace
    case 0x75: return '^';  // Up
    case 0x72: return 'v';  // Down
    case 0x76: return 27;   // ESC
    default: return 0;
  }
}

char ps2ReadKey() {
  ps2_data_ready = false;
  return decodeScancode(ps2_data);
}

// --- Game logic ---

const int screenWidth = 240;
const int screenHeight = 320;
const int boxWidth = 30;
const int boxHeight = 35;
const int boxSpacing = 5;
const int gridWidth = 5 * boxWidth + 4 * boxSpacing;
const int gridHeight = 6 * boxHeight + 5 * boxSpacing;
const int gridStartX = (screenWidth - gridWidth) / 2;
const int gridStartY = (screenHeight - gridHeight) / 2;

const char* wordList[100] = {
  "APPLE", "BREAD", "CRANE", "DREAM", "EAGLE", "FAITH", "GRAIN", "HOUSE", "IRISH", "JUMPY",
  "KHAKI", "LEMON", "MANGO", "NOBLE", "OCEAN", "PAINT", "QUERY", "RIVER", "SUGAR", "THUMB",
  "UNION", "VIVID", "WATER", "XENON", "YEARN", "ZEBRA", "ACTOR", "BANJO", "CHAIR", "DODGE",
  "EARTH", "FABLE", "GHOST", "HONEY", "INPUT", "JAZZY", "KNACK", "LASER", "MAGIC", "NEEDY",
  "OPERA", "PRISM", "QUICK", "ROVER", "STEAM", "TOUCH", "UPPER", "VOICE", "WHISK", "XYLEM",
  "YOUNG", "ZONED", "ANGEL", "BRAVE", "CLOVE", "DOUBT", "EXACT", "FLEET", "GRACE", "HORSE",
  "INDEX", "JAUNT", "KAYAK", "LIGHT", "MIRTH", "NIGHT", "OPINE", "PLUMB", "QUEST", "ROUTE",
  "STORY", "TRUTH", "USAGE", "VAULT", "WORTH", "XENIA", "YIELD", "ZAPPY", "AGILE", "BLAST",
  "CHILL", "DRIFT", "ELATE", "FROZE", "GIANT", "HUMAN", "IMAGE", "JOKER", "KNEEL", "LODGE"
};

const int wordCount = 100;
const int maxGuesses = 6;
char guess[6] = {0};
int charIndex = 0;
int currentGuess = 0;
bool gameOver = false;
const char* targetWord;

const char* menuOptions[] = {"Reguli", "Joaca"};
const int menuOptionCount = sizeof(menuOptions) / sizeof(menuOptions[0]);
int currentOption = 0;
const int menuX = 10;
const int menuY = 100;
const int menuSpacing = 20;
const int boxWidthMenu = 35;
const int boxHeightMenu = 35;
const int titleSpacing = 50;

const char* rulesText = "Wordle Rules:\n\n1. Guess the hidden word.\n2. You have 6 attempts.\n3. Each guess will be displayed.\n4. Green box: correct letter and pos.\n5. Yellow box: correct letter, wrong pos.\n6. Grey box: incorrect letter.\n\n Press BACKSPACE to return to main menu.";

void resetGame() {
  targetWord = wordList[random(0, wordCount)];
  currentGuess = 0;
  charIndex = 0;
  memset(guess, 0, sizeof(guess));
  gameOver = false;

  // Clear the display and redraw the grid
  display.fillScreen(ILI9341_BLACK);
  display.setTextColor(ILI9341_WHITE);
  display.setTextSize(2);

  for (int i = 0; i < maxGuesses; i++) {
    for (int j = 0; j < 5; j++) {
      display.drawRect(gridStartX + j * (boxWidth + boxSpacing), gridStartY + i * (boxHeight + boxSpacing), boxWidth, boxHeight, ILI9341_WHITE);
    }
  }
}

void drawMenu() {
  display.fillScreen(ILI9341_BLACK);
  display.setTextSize(4);
  display.setTextColor(ILI9341_WHITE);
  for (int i = 0; i < 6; i++) {
    display.fillRect(menuX + i * (boxWidth + 4), menuY - titleSpacing, boxWidthMenu, boxHeightMenu, ILI9341_GREEN);
    display.setCursor(menuX + i * (boxWidth + 4) + 4, menuY - titleSpacing + 8);
    display.print("WORDLE"[i]);
  }
  display.setTextSize(2);
  display.setCursor(menuX, menuY + titleSpacing);
  for (int i = 0; i < menuOptionCount; i++) {
    display.setTextColor(i == currentOption ? ILI9341_GREEN : ILI9341_WHITE);
    display.println(menuOptions[i]);
    display.setCursor(menuX, menuY + titleSpacing + (i + 1) * menuSpacing);
  }
}

void showPopup(const char* message, const char* subMessage = nullptr) {
  int popupWidth = 180, popupHeight = 60;
  int popupX = (screenWidth - popupWidth) / 2;
  int popupY = (screenHeight - popupHeight) / 2;
  display.fillRect(popupX, popupY, popupWidth, popupHeight, ILI9341_WHITE);
  display.drawRect(popupX, popupY, popupWidth, popupHeight, ILI9341_BLACK);
  display.setCursor(popupX + 10, popupY + 10);
  display.setTextColor(ILI9341_BLACK);
  display.setTextSize(2);
  display.print(message);
  display.setCursor(popupX + 10, popupY + 30);
  display.setTextSize(1);
  if (subMessage && subMessage[0]) display.print(subMessage);
  display.setCursor(popupX + 10, popupY + 45);
  display.print("Press ESC to start again");
}

void showRulesPopup() {
  display.fillScreen(ILI9341_BLACK);
  display.setCursor(10, 10);
  display.setTextColor(ILI9341_WHITE);
  display.setTextSize(2);
  display.print(rulesText);
  while (true) {
    if (ps2KeyAvailable()) {
      if (ps2ReadKey() == '\b') {
        drawMenu();
        break;
      }
    }
  }
}

void startWordleGame() {
  resetGame();
  while (!gameOver) {
    if (ps2KeyAvailable()) {
      char c = ps2ReadKey();
      if (c >= 'a' && c <= 'z') c -= 32;
      if (charIndex < 5 && c >= 'A' && c <= 'Z') {
        guess[charIndex] = c;
        display.setCursor(gridStartX + charIndex * (boxWidth + boxSpacing) + 8,
                          gridStartY + currentGuess * (boxHeight + boxSpacing) + 8);
        display.print(c);
        charIndex++;
      }
      if (c == '\n' && charIndex == 5) {
        guess[5] = '\0';
        for (int i = 0; i < 5; i++) {
          uint16_t color = ILI9341_DARKGREY;
          if (guess[i] == targetWord[i]) color = ILI9341_GREEN;
          else if (strchr(targetWord, guess[i])) color = ILI9341_YELLOW;
          display.fillRect(gridStartX + i * (boxWidth + boxSpacing), gridStartY + currentGuess * (boxHeight + boxSpacing),
                           boxWidth, boxHeight, color);
          display.setCursor(gridStartX + i * (boxWidth + boxSpacing) + 8,
                            gridStartY + currentGuess * (boxHeight + boxSpacing) + 8);
          display.setTextColor(ILI9341_WHITE);
          display.print(guess[i]);
        }
        if (strcmp(guess, targetWord) == 0) {
          showPopup("You Win!", "");
          gameOver = true;
          return;
        }
        currentGuess++;
        charIndex = 0;
        memset(guess, 0, sizeof(guess));
        if (currentGuess == maxGuesses) {
          char buffer[30];
          sprintf(buffer, "The word is: %s", targetWord);
          showPopup("You lose!", buffer);
          gameOver = true;
          return;
        }
      }
      if (c == '\b' && charIndex > 0) {
        charIndex--;
        guess[charIndex] = '\0';
        display.fillRect(gridStartX + charIndex * (boxWidth + boxSpacing),
                         gridStartY + currentGuess * (boxHeight + boxSpacing), boxWidth, boxHeight, ILI9341_BLACK);
        display.drawRect(gridStartX + charIndex * (boxWidth + boxSpacing),
                         gridStartY + currentGuess * (boxHeight + boxSpacing), boxWidth, boxHeight, ILI9341_WHITE);
      }
    }
  }
}

void setup() {
  pinMode(PS2_CLK, INPUT_PULLUP);
  pinMode(PS2_DATA, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PS2_CLK), ps2_clk_isr, FALLING);
  Serial.begin(9600);
  display.begin();
  display.setRotation(0);
  drawMenu();
}

void loop() {
  static char lastKey = 0;

  if (ps2KeyAvailable()) {
    char c = ps2ReadKey();

    // Debounce simplu: ignoră dacă e repetat
    if (c == lastKey) return;
    lastKey = c;

    if (!gameOver) {
      if (currentOption == 1 && c == '\n') { // PS2_ENTER
        startWordleGame();
        lastKey = 0;
        return;
      }
    }

    if (gameOver && c == 27) { // PS2_ESC
      resetGame();
      drawMenu();
      lastKey = 0;
      return;
    }

    if (c == '^') { // PS2_UPARROW
      currentOption--;
      if (currentOption < 0) {
        currentOption = menuOptionCount - 1;
      }
      drawMenu();
    } else if (c == 'v') { // PS2_DOWNARROW
      currentOption++;
      if (currentOption >= menuOptionCount) {
        currentOption = 0;
      }
      drawMenu();
    } else if (c == '\n') { // PS2_ENTER
      if (currentOption == 0) {
        showRulesPopup();
        lastKey = 0;
      }
    }
  } else {
    lastKey = 0; // Reset debounce dacă nu e apăsat nimic
  }
}
