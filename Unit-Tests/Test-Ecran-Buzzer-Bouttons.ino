#include <TFT_eSPI.h> 
#include <SPI.h>

// --- Pins ---
#define BTN_UP     32
#define BTN_DOWN   33
#define BTN_LEFT   25
#define BTN_RIGHT  26

#define BUZZER_PIN 15
#define BLK_PIN    21 

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);

  // Configuration Buzzer
  pinMode(BUZZER_PIN, OUTPUT); 

  // Boutons (On reste en INPUT simple si tes boutons sont au 3.3V)
  pinMode(BTN_UP, INPUT);
  pinMode(BTN_DOWN, INPUT);
  pinMode(BTN_LEFT, INPUT);
  pinMode(BTN_RIGHT, INPUT);

  // Rétroéclairage
  pinMode(BLK_PIN, OUTPUT);
  digitalWrite(BLK_PIN, HIGH);

  // Écran
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Systeme OK !");
}

void loop() {
  // On vérifie chaque bouton
  if (digitalRead(BTN_UP) == HIGH) {
    handleAction("HAUT", 1000);
  }
  if (digitalRead(BTN_DOWN) == HIGH) {
    handleAction("BAS", 600);
  }
  if (digitalRead(BTN_LEFT) == HIGH) {
    handleAction("GAUCHE", 800);
  }
  if (digitalRead(BTN_RIGHT) == HIGH) {
    handleAction("DROITE", 1200);
  }
}

// Fonction pour éviter de répéter le code de l'écran partout
void handleAction(String nom, int frequence) {
  Serial.println(nom);
  
  // Affichage écran
  tft.setCursor(10, 50);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.print("Action : ");
  tft.print(nom + "    "); 

  // Son (Ta méthode qui marche !)
  tone(BUZZER_PIN, frequence, 120);
  
  delay(250); // Anti-rebond
}