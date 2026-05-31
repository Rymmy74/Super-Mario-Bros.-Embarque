#include <TFT_eSPI.h>

#define BTN_UP    32  
#define BTN_DOWN  33
#define BTN_LEFT  25  
#define BTN_RIGHT 26  
#define BUZZER_PIN 15 

TFT_eSPI tft = TFT_eSPI();

#define COLOR_BG_TOP    0x190A 
#define COLOR_BG_BOTTOM 0x9150 
#define COLOR_PINK      0xE12B 
#define COLOR_DARK_PURP 0x30E6 
#define COLOR_WHITE     0xFFFF 
#define COLOR_BLUE_TEXT 0x229F 
#define COLOR_CHAR_BG   0x112B 
#define COLOR_MAZE_BG   0x01A6 
#define COLOR_START     0x07E0 

// Couleurs des personnages
#define COLOR_CUBE_RED    0xF800 
#define COLOR_CUBE_ORANGE 0xFDA0 

// Couleur pour le fond des boutons style pilule (non sélectionnés)
#define COLOR_BT_BG     0xDEFB 

enum Scene {
  MAIN_MENU,
  LEVEL_SELECTION,
  CONFIRMATION,
  OPTIONS,
  CREDITS,
  STARTING_SCREEN,
  CHARACTER_SELECTION,
  LOADING_SCREEN,
  THE_GAME,
  GAME_OVER,
  VICTORY_SCREEN
};

Scene currentScene = MAIN_MENU;
int selection = 0;           
int gameOverSelection = 0; // 0 = RESTART, 1 = RETURN
int victorySelection = 0;  // 0 = CONTINUE, 1 = RETURN
int selectedLevel = 1;   
int selectedCharacter = 0; 

int playerX = 14;          
int playerY = 10;          
const int playerSize = 12; 
int playerLives = 3;       

bool soundOn = true;
int brightness = 50; 

struct Wall {
  int x, y, w, h;
};

Wall walls[] = {
  // --- BLOC EN HAUT À GAUCHE (START ZONE) ---
  {32,  35,  3,  25},  
  {32,  35,  32, 3},   
  {64,  35,  3,  85},  
  {5,   85,  59, 3},   

  // --- ZONE INFÉRIEURE GAUCHE ---
  {36,  120, 3,  85},  
  {36,  165, 59, 3},   
  {68,  200, 3,  35},  

  // --- CENTRE GAUCHE ---
  {95,  35,  3,  140}, 
  {135, 5,   3,  35},  
  {135, 65,  3,  115}, 

  // --- ZONE HAUTE CENTRE ET DROITE ---
  {135, 35,  45, 3},   
  {180, 35,  3,  30},  
  {95,  65,  40, 3},   
  {135, 90,  85, 3},   
  {220, 38,  3,  52},  
  {220, 38,  72, 3},   

  // --- LABYRINHE INFÉRIEUR & DROIT ---
  {185, 120, 80, 3},   
  {185, 120, 3,  40},  
  {233, 120, 3,  45},  
  {233, 162, 62, 3},   
  {295, 110, 3,  55},  
  {265, 78,  50, 3},   
  
  {68,  190, 222, 3}   
};

const int NUM_WALLS = sizeof(walls) / sizeof(walls[0]);

void drawCurrentScene();
void executeSelection();
void waitForRelease(int pin);
bool checkLongPress(int pin);


void waitForRelease(int pin) {
  delay(50); 
  while(digitalRead(pin) == HIGH) { delay(5); }
}

bool checkLongPress(int pin) {
  if (digitalRead(pin) == HIGH) {
    unsigned long start = millis();
    while (digitalRead(pin) == HIGH) {
      if (millis() - start > 400) { 
        waitForRelease(pin);
        return true; 
      }
      delay(10);
    }
  }
  return false;
}

void playDyingSound() {
  if (!soundOn) return;
  pinMode(BUZZER_PIN, OUTPUT);
  tone(BUZZER_PIN, 280, 120); delay(130);
  tone(BUZZER_PIN, 180, 120); delay(130);
  tone(BUZZER_PIN, 100, 250); delay(250);
  noTone(BUZZER_PIN);
  pinMode(BUZZER_PIN, INPUT);
}

void resetPlayerPosition() {
  playerX = 14; 
  playerY = 10;
}

void drawBackground() {
  for (int y = 0; y < 225; y++) {
    uint8_t r = map(y, 0, 225, (COLOR_BG_TOP >> 11) & 0x1F, (COLOR_BG_BOTTOM >> 11) & 0x1F);
    uint8_t g = map(y, 0, 225, (COLOR_BG_TOP >> 5) & 0x3F, (COLOR_BG_BOTTOM >> 5) & 0x3F);
    uint8_t b = map(y, 0, 225, COLOR_BG_TOP & 0x1F, COLOR_BG_BOTTOM & 0x1F);
    uint16_t color = (r << 11) | (g << 5) | b;
    tft.drawFastHLine(0, y, 320, color);
  }
  tft.fillRect(0, 225, 320, 15, COLOR_DARK_PURP);
}

void drawOmniButton(String text, int x, int y, int w, int h, bool isSelected, int textOffsetXX = 12, int textOffsetYY = 5) {
  if (isSelected) {
    tft.fillRoundRect(x, y, w, h, 8, COLOR_WHITE);
    tft.setTextColor(COLOR_PINK); 
  } else {
    tft.fillRoundRect(x, y, w, h, 8, COLOR_PINK); 
    tft.setTextColor(COLOR_WHITE);
  }
  tft.drawString(text, x + textOffsetXX, y + textOffsetYY);
}

void drawPillButton(String text, int x, int y, int w, int h, bool isSelected) {
  int r = h / 2;
  int textWidth = tft.textWidth(text);
  int textOffsetXX = (w - textWidth) / 2;
  int textOffsetYY = (h - tft.fontHeight()) / 2 + 1;

  if (isSelected) {
    // SÉLECTIONNÉ : Fond blanc et texte bleu
    tft.fillRoundRect(x, y, w, h, r, COLOR_WHITE); 
    tft.setTextColor(COLOR_BLUE_TEXT);
  } else {
    // PAR DÉFAUT : Fond clair d'origine et texte bleu
    tft.fillRoundRect(x, y, w, h, r, COLOR_BT_BG); 
    tft.setTextColor(COLOR_BLUE_TEXT);
  }
  tft.drawString(text, x + textOffsetXX, y + textOffsetYY);
}

void drawCubeCharacter(int x, int y, bool isSelected, uint16_t cubeColor) {
  tft.fillRect(x - 8, y - 8, 66, 66, COLOR_CHAR_BG); 
  if (isSelected) {
    tft.drawRect(x - 6, y - 6, 62, 62, COLOR_WHITE); 
    tft.drawRect(x - 5, y - 5, 60, 60, COLOR_WHITE);
  }
  tft.fillRect(x, y, 50, 50, cubeColor); 
}

void drawLevelOneMaze() {
  tft.fillScreen(COLOR_MAZE_BG);
  tft.fillRect(0, 0, 320, 5, COLOR_PINK);       
  tft.fillRect(0, 235, 320, 5, COLOR_PINK);     
  tft.fillRect(0, 0, 5, 240, COLOR_PINK);       
  tft.fillRect(315, 0, 5, 240, COLOR_PINK);     

  for (int i = 0; i < NUM_WALLS; i++) {
    tft.fillRect(walls[i].x, walls[i].y, walls[i].w, walls[i].h, COLOR_WHITE);
  }

  tft.fillRect(5, 5, 25, 25, COLOR_START);
  tft.setTextColor(COLOR_WHITE); tft.setTextSize(1);
  tft.drawString("S", 14, 14);

  tft.fillRect(285, 205, 25, 25, COLOR_PINK); 
  tft.drawString("E", 294, 214);

  tft.setTextColor(COLOR_WHITE); tft.setTextSize(1);
  tft.drawString("LIVES: " + String(playerLives), 145, 11);

  uint16_t pColor = (selectedCharacter == 0) ? COLOR_CUBE_RED : COLOR_CUBE_ORANGE; 
  tft.fillRect(playerX, playerY, playerSize, playerSize, pColor);
}

bool checkWallCollision(int nx, int ny) {
  if (nx < 5 || (nx + playerSize) > 315 || ny < 5 || (ny + playerSize) > 235) return true;
  for (int i = 0; i < NUM_WALLS; i++) {
    if (nx < walls[i].x + walls[i].w && nx + playerSize > walls[i].x &&
        ny < walls[i].y + walls[i].h && ny + playerSize > walls[i].y) {
      return true; 
    }
  }
  return false;
}

void drawCurrentScene() {
  if (currentScene != STARTING_SCREEN && currentScene != LOADING_SCREEN && currentScene != CHARACTER_SELECTION && currentScene != THE_GAME && currentScene != GAME_OVER && currentScene != VICTORY_SCREEN) {
    drawBackground();
  }
  
  tft.setTextSize(2);
  switch (currentScene) {
    case MAIN_MENU:
      tft.setTextColor(COLOR_WHITE); tft.setTextSize(3); tft.drawString("Omni", 125, 30); tft.setTextSize(2);
      drawOmniButton("PLAY", 100, 95, 120, 28, (selection == 0), 35);
      drawOmniButton("OPTIONS", 100, 135, 120, 28, (selection == 1), 18);
      drawOmniButton("QUIT", 100, 175, 120, 28, (selection == 2), 35);
      break;

    case LEVEL_SELECTION:
      tft.setTextColor(COLOR_PINK); tft.drawString("SELECT THE LEVEL", 25, 22); 
      for(int i = 0; i < 3; i++) drawOmniButton("LEVEL " + String(i + 1), 100, 75 + (i * 35), 120, 26, (selection == i), 22);
      drawOmniButton("RETURN", 210, 190, 100, 26, (selection == 3), 15);
      break;

    case CONFIRMATION:
      tft.fillRoundRect(40, 65, 240, 55, 12, COLOR_PINK); 
      tft.setTextColor(COLOR_WHITE); tft.setTextSize(1);
      tft.drawString("YOU CHOSE LEVEL " + String(selectedLevel), 95, 75);
      tft.drawString("DO YOU WANT TO PROCEED?", 78, 95); tft.setTextSize(2);
      drawOmniButton("YES", 60, 145, 70, 26, (selection == 0), 16);
      drawOmniButton("NO", 180, 145, 60, 26, (selection == 1), 18);
      break;

    case OPTIONS:
      tft.setTextColor(COLOR_PINK); tft.drawString("OPTIONS", 25, 18); 
      drawOmniButton("SOUND", 25, 60, 100, 26, (selection == 0), 20);
      drawOmniButton("ON", 160, 60, 45, 24, soundOn, 15, 4);
      drawOmniButton("OFF", 215, 60, 45, 24, !soundOn, 6, 4);
      drawOmniButton("BRIGHTNESS", 25, 100, 140, 26, (selection == 1), 10);
      drawOmniButton("CREDITS", 25, 140, 110, 26, (selection == 2), 15);
      drawOmniButton("RETURN", 185, 190, 100, 26, (selection == 3), 15);
      break;

    case CREDITS:
      tft.setTextColor(COLOR_PINK); tft.setTextSize(2); tft.drawString("Omni - CREDITS", 25, 18);
      tft.fillRoundRect(25, 50, 215, 165, 12, COLOR_PINK); 
      tft.setTextColor(COLOR_WHITE); tft.setTextSize(1);
      tft.drawString("Developed by Rim", 85, 60); 
      tft.drawString("Programming ............. Rim", 40, 78); 
      tft.drawString("UI/UX Design ............ Rim", 40, 93); 
      tft.drawString("Art Direction ........... Rim", 40, 108);
      tft.drawString("Sound Design ............ Rim", 40, 123);
      tft.drawString("Hardware Integration .... Rim", 40, 138);
      tft.drawString("Special Thanks", 95, 160);
      tft.drawString("Hexagone  |  ESP32 Community", 48, 178);
      tft.drawString("My Teachers", 100, 193);
      tft.setTextSize(2);
      drawOmniButton("RETURN", 242, 190, 75, 26, (selection == 0), 4);
      break;

    case STARTING_SCREEN:
      tft.fillScreen(0x0842); tft.setTextColor(COLOR_BLUE_TEXT); tft.setTextSize(3);
      tft.drawString("STARTING", 85, 105);
      break;

    case CHARACTER_SELECTION:
      tft.fillScreen(COLOR_CHAR_BG); 
      tft.setTextColor(COLOR_WHITE); tft.setTextSize(2);
      tft.drawString("SELECT YOUR CHARACTER", 35, 30);
      drawCubeCharacter(75, 95, (selectedCharacter == 0), COLOR_CUBE_RED);   
      drawCubeCharacter(195, 95, (selectedCharacter == 1), COLOR_CUBE_ORANGE); 
      tft.setTextSize(1); tft.drawString("Hold any button to Confirm", 85, 190);
      break;

    case LOADING_SCREEN:
      tft.fillScreen(0x0842); tft.setTextColor(COLOR_BLUE_TEXT); tft.setTextSize(3);
      tft.drawString("LOADING", 95, 105);
      break;

    case THE_GAME:
      drawLevelOneMaze();
      break;

    case GAME_OVER:
      tft.fillScreen(0x0002); 
      tft.setTextColor(COLOR_BLUE_TEXT); tft.setTextSize(3); 
      tft.drawString("GAME OVER", 85, 90); 
      tft.setTextSize(2);
      drawPillButton("RESTART", 50, 145, 95, 28, (gameOverSelection == 0));
      drawPillButton("RETURN", 175, 145, 95, 28, (gameOverSelection == 1));
      break;

    case VICTORY_SCREEN:
      tft.fillScreen(0x0002); 
      tft.setTextColor(COLOR_BLUE_TEXT); tft.setTextSize(3);
      tft.drawString("YOU WON !", 95, 90); 
      tft.setTextSize(2);
      drawPillButton("CONTINUE", 45, 145, 105, 28, (victorySelection == 0));
      drawPillButton("RETURN", 175, 145, 95, 28, (victorySelection == 1));
      break;
  }
}

void executeSelection() {
  if (currentScene == MAIN_MENU) {
    if (selection == 0) { currentScene = LEVEL_SELECTION; selection = 0; }
    else if (selection == 1) { currentScene = OPTIONS; selection = 0; }
    else if (selection == 2) { tft.fillScreen(TFT_BLACK); while(1); } 
  } 
  else if (currentScene == LEVEL_SELECTION) {
    if (selection == 3) { currentScene = MAIN_MENU; selection = 0; } 
    else { selectedLevel = selection + 1; currentScene = CONFIRMATION; selection = 0; }
  } 
  else if (currentScene == CONFIRMATION) {
    if (selection == 0) {
      currentScene = STARTING_SCREEN; drawCurrentScene(); delay(1200); 
      currentScene = CHARACTER_SELECTION;
      drawCurrentScene(); 
    } else { 
      currentScene = LEVEL_SELECTION; 
      selection = 0;
      drawCurrentScene();
    }
    return; 
  } 
  else if (currentScene == CHARACTER_SELECTION) {
    currentScene = LOADING_SCREEN; drawCurrentScene(); delay(1200); 
    playerLives = 3; 
    resetPlayerPosition();
    currentScene = THE_GAME; 
  }
  else if (currentScene == GAME_OVER) {
    if (gameOverSelection == 0) {
      playerLives = 3;
      resetPlayerPosition();
      currentScene = THE_GAME;
    } else {
      currentScene = MAIN_MENU;
      selection = 0;
    }
  }
  else if (currentScene == VICTORY_SCREEN) {
    if (victorySelection == 0) {
      playerLives = 3;
      resetPlayerPosition();
      currentScene = THE_GAME;
    } else {
      currentScene = MAIN_MENU;
      selection = 0;
    }
  }
  else if (currentScene == OPTIONS) {
    if (selection == 0) soundOn = !soundOn;
    else if (selection == 2) { currentScene = CREDITS; selection = 0; } 
    else if (selection == 3) { currentScene = MAIN_MENU; selection = 1; } 
  } 
  else if (currentScene == CREDITS) {
    if (selection == 0) { currentScene = OPTIONS; selection = 2; } 
  }
  drawCurrentScene();
}

void setup() {
  tft.init();
  tft.setRotation(1); 
  
  pinMode(BTN_UP, INPUT_PULLDOWN);
  pinMode(BTN_DOWN, INPUT_PULLDOWN);
  pinMode(BTN_LEFT, INPUT_PULLDOWN);
  pinMode(BTN_RIGHT, INPUT_PULLDOWN);

  drawCurrentScene();
}

void loop() {

  if (currentScene == THE_GAME) {
    int nextX = playerX;
    int nextY = playerY;
    bool moved = false;

    if (digitalRead(BTN_UP) == HIGH)    { nextY--; moved = true; }
    if (digitalRead(BTN_DOWN) == HIGH)  { nextY++; moved = true; }
    if (digitalRead(BTN_LEFT) == HIGH)  { nextX--; moved = true; }  
    if (digitalRead(BTN_RIGHT) == HIGH) { nextX++; moved = true; }  

    if (moved) {
      tft.fillRect(playerX, playerY, playerSize, playerSize, COLOR_MAZE_BG);
      if (checkWallCollision(nextX, nextY)) {
        playerLives--;
        playDyingSound(); 
        if (playerLives <= 0) {
          currentScene = GAME_OVER; 
          gameOverSelection = 0; 
          drawCurrentScene();
        } else {
          resetPlayerPosition(); 
          drawLevelOneMaze();    
        }
      } else {
        playerX = nextX;
        playerY = nextY;
      }
      uint16_t pColor = (selectedCharacter == 0) ? COLOR_CUBE_RED : COLOR_CUBE_ORANGE; 
      tft.fillRect(playerX, playerY, playerSize, playerSize, pColor);
      
      if (playerX >= 280 && playerY >= 200) {
        currentScene = VICTORY_SCREEN; 
        victorySelection = 0;
        drawCurrentScene();
      }
      delay(12); 
    }
  }

  if (currentScene != THE_GAME) {
    
    if (digitalRead(BTN_UP) == HIGH) {
      if (checkLongPress(BTN_UP)) {
        executeSelection();
      } else {
        if (currentScene == MAIN_MENU) selection = (selection - 1 + 3) % 3;
        else if (currentScene == LEVEL_SELECTION) selection = (selection - 1 + 4) % 4; 
        else if (currentScene == CONFIRMATION) selection = (selection - 1 + 2) % 2;
        else if (currentScene == OPTIONS) selection = (selection - 1 + 4) % 4; 
        drawCurrentScene();
        waitForRelease(BTN_UP);
      }
    }

    if (digitalRead(BTN_DOWN) == HIGH) {
      if (checkLongPress(BTN_DOWN)) {
        executeSelection();
      } else {
        if (currentScene == MAIN_MENU) selection = (selection + 1) % 3;
        else if (currentScene == LEVEL_SELECTION) selection = (selection + 1) % 4;
        else if (currentScene == CONFIRMATION) selection = (selection + 1) % 2;
        else if (currentScene == OPTIONS) selection = (selection + 1) % 4;
        drawCurrentScene();
        waitForRelease(BTN_DOWN);
      }
    }

    if (digitalRead(BTN_LEFT) == HIGH) {
      if (checkLongPress(BTN_LEFT)) {
        executeSelection();
      } else {
        if (currentScene == CONFIRMATION) selection = 0; 
        else if (currentScene == CHARACTER_SELECTION) { 
          selectedCharacter = 0; 
        } 
        else if (currentScene == GAME_OVER) {
          gameOverSelection = 0; 
        }
        else if (currentScene == VICTORY_SCREEN) {
          victorySelection = 0; 
        }
        drawCurrentScene();
        waitForRelease(BTN_LEFT);
      }
    }

    if (digitalRead(BTN_RIGHT) == HIGH) {
      if (checkLongPress(BTN_RIGHT)) {
        executeSelection();
      } else {
        if (currentScene == CONFIRMATION) selection = 1; 
        else if (currentScene == CHARACTER_SELECTION) { 
          selectedCharacter = 1; 
        } 
        else if (currentScene == GAME_OVER) {
          gameOverSelection = 1; 
        }
        else if (currentScene == VICTORY_SCREEN) {
          victorySelection = 1; 
        }
        drawCurrentScene();
        waitForRelease(BTN_RIGHT);
      }
    }
  }
}