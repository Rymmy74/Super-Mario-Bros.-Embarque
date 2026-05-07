#include <TFT_eSPI.h>
#include <SPI.h>

// --- Pins ---
#define BTN_UP     32
#define BTN_DOWN   33
#define BTN_SELECT 26 // On utilise le bouton DROITE pour valider
#define BUZZER_PIN 15
#define BLK_PIN    21

TFT_eSPI tft = TFT_eSPI();

int indexSelection = 0; // 0: Jouer, 1: Options, 2: Quitter
const int totalOptions = 3;

// Couleurs (Pastels comme sur ton image)
uint16_t couleurFond    = tft.color565(200, 255, 240); // Cyan très clair
uint16_t couleurBandeau = tft.color565(210, 100, 100); // Rouge pastel
uint16_t couleurBouton  = tft.color565(130, 190, 205); // Bleu-gris
uint16_t couleurHover   = tft.color565(255, 255, 255); // Blanc pour le "hover"

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

void loop() {
  // Navigation vers le haut
  if (digitalRead(BTN_UP) == HIGH) {
    indexSelection--;
    if (indexSelection < 0) indexSelection = totalOptions - 1;
    tone(BUZZER_PIN, 800, 50); // Petit clic sonore de navigation
    dessinerMenu();
    delay(200);
  }

  // Navigation vers le bas
  if (digitalRead(BTN_DOWN) == HIGH) {
    indexSelection++;
    if (indexSelection >= totalOptions) indexSelection = 0;
    tone(BUZZER_PIN, 800, 50);
    dessinerMenu();
    delay(200);
  }

  // Validation (Bouton Select)
  if (digitalRead(BTN_SELECT) == HIGH) {
    tone(BUZZER_PIN, 1500, 200); // Son de validation plus long/aigu
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(50, 100);
    tft.print("Lancement : ");
    if(indexSelection == 0) tft.print("JEU");
    if(indexSelection == 1) tft.print("OPTIONS");
    if(indexSelection == 2) tft.print("QUITTER");
    delay(2000);
    dessinerMenu(); // Retour au menu
  }
}

// --- Fonction pour dessiner l'interface graphique ---
void dessinerMenu() {
  // Fond principal
  tft.fillScreen(couleurFond);

  // Bandeau Titre
  tft.fillRect(0, 0, 320, 60, couleurBandeau);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.setCursor(45, 20);
  tft.print("TITRE DU JEU");

  // Dessin des 3 boutons
  drawButton(80, "JOUER", indexSelection == 0);
  drawButton(130, "OPTIONS", indexSelection == 1);
  drawButton(180, "QUITTER", indexSelection == 2);
}

// Fonction utilitaire pour dessiner un bouton arrondi
void drawButton(int y, String label, bool isSelected) {
  uint16_t col = isSelected ? couleurHover : couleurBouton;
  uint16_t textCol = isSelected ? TFT_BLACK : TFT_WHITE;

  // Corps du bouton (Rectangle arrondi)
  tft.fillRoundRect(60, y, 200, 40, 15, col);
  tft.drawRoundRect(60, y, 200, 40, 15, TFT_SKYBLUE); // Bordure fine

  // Texte du bouton
  tft.setTextColor(textCol);
  tft.setTextSize(2);
  // Centrage manuel approximatif du texte
  int padding = (200 - (label.length() * 12)) / 2; 
  tft.setCursor(60 + padding, y + 12);
  tft.print(label);
}