#include <TFT_eSPI.h>
#include <SPI.h>

#define BTN_UP    32
#define BTN_DOWN  33
#define BTN_SELECT 26
#define BUZZER_PIN 15
#define BLK_PIN 21

TFT_eSPI tft = TFT_eSPI();

uint16_t couleurFond    = tft.color565(200, 255, 240);
uint16_t couleurBandeau = tft.color565(210, 100, 100);
uint16_t couleurBouton  = tft.color565(130, 190, 205);
uint16_t couleurHover   = tft.color565(255, 255, 255);

int indexSelection = 0;
const int totalOptions = 3;

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

void setup() {
    pinMode(BTN_UP, INPUT);
    pinMode(BTN_DOWN, INPUT);
    pinMode(BTN_SELECT, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BLK_PIN, OUTPUT);

    digitalWrite(BLK_PIN, HIGH);

    tft.init();
    tft.setRotation(1);
    dessinerMenu();
}

void loop() {}
