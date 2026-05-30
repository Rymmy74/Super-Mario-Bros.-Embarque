#include <TFT_eSPI.h>
#include <SPI.h>

// --- Pins ---
#define BTN_UP    32
#define BTN_DOWN  33
#define BTN_LEFT  25
#define BTN_RIGHT 26
#define BTN_SELECT 26
#define BUZZER_PIN 15
#define BLK_PIN 21

TFT_eSPI tft = TFT_eSPI();

// --- Couleurs ---
uint16_t couleurFond    = tft.color565(200, 255, 240);
uint16_t couleurBouton  = tft.color565(130, 190, 205);
uint16_t couleurHover   = tft.color565(255, 255, 255);

// --- Menu ---
int indexSelection = 0;
const int totalOptions = 3;

// --- Scenes ---
enum Scene { MENU, GAME, OPTIONS, CREDITS };
Scene currentScene = MENU;

// --- Maze ---
const int mazeWidth = 160;
const int mazeHeight = 120;

int maze[mazeHeight][mazeWidth];

int playerX = 10;
int playerY = 10;

// --- Fonctions ---
void generateMaze() {
    for (int y = 0; y < mazeHeight; y++) {
        for (int x = 0; x < mazeWidth; x++) {
            maze[y][x] = (x % 20 == 0) ? 1 : 0;
        }
    }
}

bool isWall(int x, int y) {
    if (x < 0 || y < 0 || x >= mazeWidth || y >= mazeHeight) return true;
    return maze[y][x] == 1;
}

void drawMaze() {
    tft.fillScreen(TFT_BLACK);
    for (int y = 0; y < mazeHeight; y++) {
        for (int x = 0; x < mazeWidth; x++) {
            if (maze[y][x] == 1) {
                tft.drawPixel(x, y, TFT_WHITE);
            }
        }
    }
    tft.fillCircle(playerX, playerY, 3, TFT_RED);
}

void dessinerMenu() {
    tft.fillScreen(couleurFond);
    tft.setTextSize(2);

    const char* options[] = {"Jouer", "Options", "Quitter"};

    for (int i = 0; i < totalOptions; i++) {
        uint16_t col = (i == indexSelection) ? couleurHover : couleurBouton;
        tft.fillRoundRect(40, 60 + i*50, 240, 40, 8, col);
        tft.setCursor(60, 70 + i*50);
        tft.print(options[i]);
    }
}

// --- Setup ---
void setup() {
    pinMode(BTN_UP, INPUT);
    pinMode(BTN_DOWN, INPUT);
    pinMode(BTN_LEFT, INPUT);
    pinMode(BTN_RIGHT, INPUT);
    pinMode(BTN_SELECT, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BLK_PIN, OUTPUT);

    digitalWrite(BLK_PIN, HIGH);

    tft.init();
    tft.setRotation(1);

    generateMaze();
    dessinerMenu();
}

// --- Loop ---
void loop() {
    switch (currentScene) {
        case MENU:
            if (digitalRead(BTN_UP) == HIGH) {
                indexSelection--;
                if (indexSelection < 0) indexSelection = totalOptions - 1;
                dessinerMenu();
                delay(200);
            }
            if (digitalRead(BTN_DOWN) == HIGH) {
                indexSelection++;
                if (indexSelection >= totalOptions) indexSelection = 0;
                dessinerMenu();
                delay(200);
            }
            if (digitalRead(BTN_SELECT) == HIGH) {
                if (indexSelection == 0) currentScene = GAME;
                delay(200);
            }
            break;

        case GAME:
            int nextX = playerX;
            int nextY = playerY;

            if (digitalRead(BTN_UP) == HIGH) nextY--;
            if (digitalRead(BTN_DOWN) == HIGH) nextY++;
            if (digitalRead(BTN_LEFT) == HIGH) nextX--;
            if (digitalRead(BTN_RIGHT) == HIGH) nextX++;

            if (!isWall(nextX, nextY)) {
                playerX = nextX;
                playerY = nextY;
            }

            drawMaze();
            delay(20);
            break;
    }
}
