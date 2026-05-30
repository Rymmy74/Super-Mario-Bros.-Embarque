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

// --- Fonctions d'affichage ---
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

void dessinerOptions() {
    tft.fillScreen(couleurFond);
    tft.setTextSize(2);
    tft.drawString("OPTIONS", 100, 40);
}

void dessinerCredits() {
    tft.fillScreen(couleurFond);
    tft.setTextSize(2);
    tft.drawString("CREDITS", 100, 40);
}

void dessinerGame() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("GAME START", 80, 100);
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
    dessinerMenu();
}

// --- Loop ---
void loop() {
    switch (currentScene) {
        case MENU:
            if (digitalRead(BTN_UP) == HIGH) {
                indexSelection--;
                if (indexSelection < 0) indexSelection = totalOptions - 1;
                tone(BUZZER_PIN, 800, 50);
                dessinerMenu();
                delay(200);
            }
            if (digitalRead(BTN_DOWN) == HIGH) {
                indexSelection++;
                if (indexSelection >= totalOptions) indexSelection = 0;
                tone(BUZZER_PIN, 800, 50);
                dessinerMenu();
                delay(200);
            }
            if (digitalRead(BTN_SELECT) == HIGH) {
                if (indexSelection == 0) currentScene = GAME;
                if (indexSelection == 1) currentScene = OPTIONS;
                if (indexSelection == 2) currentScene = CREDITS;
                delay(200);
            }
            break;

        case GAME:
            dessinerGame();
            break;

        case OPTIONS:
            dessinerOptions();
            break;

        case CREDITS:
            dessinerCredits();
            break;
    }
}
