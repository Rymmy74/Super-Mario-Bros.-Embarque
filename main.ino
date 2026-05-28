#include <TFT_eSPI.h>
#include <SPI.h>

#define BTN_UP    32
#define BTN_DOWN  33
#define BTN_LEFT  25
#define BTN_RIGHT 26
#define BUZZER_PIN 15
#define BLK_PIN 21

TFT_eSPI tft = TFT_eSPI();

void setup() {
    pinMode(BTN_UP, INPUT);
    pinMode(BTN_DOWN, INPUT);
    pinMode(BTN_LEFT, INPUT);
    pinMode(BTN_RIGHT, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BLK_PIN, OUTPUT);

    digitalWrite(BLK_PIN, HIGH);

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
}

void loop() {}
