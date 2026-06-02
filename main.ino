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
#define COLOR_MAZE_BROWN 0x38A1 // Fond du Niveau 1
#define COLOR_MAZE_BLUE 0x01A6  // Fond des Niveaux 2 & 3 / Bleu sélection
#define COLOR_START     0x07E0 
#define COLOR_ENEMY_GREEN 0x03E0 // Vert foncé pour l'ennemi

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
int gameOverSelection = 0; 
int victorySelection = 0;  
int selectedLevel = 1;   
int selectedCharacter = 0; 

// ==========================================
// CONFIGURATION DU JOUEUR & ENNEMI
// ==========================================
int playerX = 15;          
int playerY = 15;          
const int playerSize = 12; 
int playerLives = 3;       

// Variables de l'ennemi (Niveau 3)
int enemyX = 145; 
int enemyY = 105;
int enemyDirX = 0; 
int enemyDirY = 0; 
const int enemySize = 12;
unsigned long lastEnemyMove = 0;

// Extension maximale à droite (292) juste avant le mur vertical à 295
int enemyWaypointsX[] = {145, 145, 292, 112, 112, 145};
int enemyWaypointsY[] = {105, 168, 168, 168, 48,  48};
const int NUM_WAYPOINTS = sizeof(enemyWaypointsX) / sizeof(enemyWaypointsX[0]);
int currentWaypoint = 0;

bool soundOn = true;
int brightness = 50; 

struct Wall {
  int x, y, w, h;
};

// ==========================================
// LABYRINTHE NIVEAU 1 (Style Pac-Man)
// ==========================================
Wall wallsLevel1[] = {
  {45,  5,   6,   50},  
  {45,  55,  50,  6},   
  {135, 5,   6,   55},  
  {135, 55,  55,  6},  
  {230, 5,   6,   55},  
  {230, 55,  45,  6},  
  {45,  95,  96,  6},   
  {90,  95,  6,   50},  
  {180, 95,  6,   50},  
  {180, 95,  95,  6},  
  {45,  145, 6,   50},  
  {45,  145, 45,  6},   
  {135, 110, 6,   50},  
  {135, 145, 45,  6},  
  {90,  190, 96,  6},   
  {230, 145, 6,   55},  
  {230, 145, 45,  6},   
  {275, 95,  6,   55},  
  {5,   190, 45,  6}    
};
const int NUM_WALLS_L1 = sizeof(wallsLevel1) / sizeof(wallsLevel1[0]);

// ==========================================
// LABYRINTHE NIVEAUX 2 & 3 (Technique)
// ==========================================
Wall wallsLevel2And3[] = {
  {32,  35,  3,  25},  
  {32,  35,  32, 3},   
  {64,  35,  3,  85},  
  {5,   85,  59, 3},   
  {36,  120, 3,  85},  
  {36,  165, 59, 3},   
  {68,  200, 3,  35},  
  {95,  35,  3,  140}, 
  {135, 5,   3,  35},  
  {135, 65,  3,  115}, 
  {135, 35,  45, 3},   
  {180, 35,  3,  30},  
  {95,  65,  40, 3},   
  {135, 90,  85, 3},   
  {220, 38,  3,  52},  
  {220, 38,  72, 3},   
  {185, 120, 80, 3},   
  {185, 120, 3,  40},  
  {233, 120, 3,  45},  
  {233, 162, 62, 3},   
  {295, 110, 3,  55},  
  {265, 78,  50, 3},   
  {68,  190, 222, 3}   
};
const int NUM_WALLS_L23 = sizeof(wallsLevel2And3) / sizeof(wallsLevel2And3[0]);

// ==========================================
// PROTOTYPES DES FONCTIONS
// ==========================================
void drawCurrentScene();
void executeSelection();
void waitForRelease(int pin);
bool checkLongPress(int pin);
void drawLevelMaze();
bool checkWallCollision(int nx, int ny, int size);
void resetPlayerPosition();
void playDyingSound();
void updateEnemyAI();

// ==========================================
// FONCTIONS COMPORTEMENTALES
// ==========================================
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
  playerX = 15; 
  playerY = 15;
  if (selectedLevel == 3) {
    enemyX = 145;
    enemyY = 105;
    currentWaypoint = 0;
    enemyDirX = 0;
    enemyDirY = 0;
  }
}

// ==========================================
// LOGIQUE D'AFFICHAGE DES MENUS
// ==========================================
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
    // Changement ici : Fond bleu et texte blanc quand sélectionné
    tft.fillRoundRect(x, y, w, h, r, COLOR_MAZE_BLUE); 
    tft.setTextColor(COLOR_WHITE);
  } else {
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

// ==========================================
// DESSIN DU LABYRINTHE
// ==========================================
void drawLevelMaze() {
  uint16_t currentBg = (selectedLevel == 1) ? COLOR_MAZE_BROWN : COLOR_MAZE_BLUE;
  tft.fillScreen(currentBg);
  
  tft.fillRect(0, 0, 320, 5, COLOR_PINK);       
  tft.fillRect(0, 235, 320, 5, COLOR_PINK);     
  tft.fillRect(0, 0, 5, 240, COLOR_PINK);       
  tft.fillRect(315, 0, 5, 240, COLOR_PINK);     

  if (selectedLevel == 1) {
    for (int i = 0; i < NUM_WALLS_L1; i++) {
      tft.fillRect(wallsLevel1[i].x, wallsLevel1[i].y, wallsLevel1[i].w, wallsLevel1[i].h, COLOR_WHITE);
    }
  } else { 
    for (int i = 0; i < NUM_WALLS_L23; i++) {
      tft.fillRect(wallsLevel2And3[i].x, wallsLevel2And3[i].y, wallsLevel2And3[i].w, wallsLevel2And3[i].h, COLOR_WHITE);
    }
  }

  tft.fillRect(5, 5, 38, 35, COLOR_START);
  tft.setTextColor(COLOR_WHITE); tft.setTextSize(1);
  tft.drawString("S", 20, 18);

  tft.fillRect(273, 195, 42, 40, COLOR_PINK); 
  tft.drawString("E", 290, 210);

  tft.setTextColor(COLOR_WHITE); tft.setTextSize(1);
  tft.drawString("LIVES: " + String(playerLives), 145, 11);

  uint16_t pColor = (selectedCharacter == 0) ? COLOR_CUBE_RED : COLOR_CUBE_ORANGE; 
  tft.fillRect(playerX, playerY, playerSize, playerSize, pColor);

  if (selectedLevel == 3) {
    tft.fillRect(enemyX, enemyY, enemySize, enemySize, COLOR_ENEMY_GREEN);
  }
}

bool checkWallCollision(int nx, int ny, int size) {
  if (nx < 5 || (nx + size) > 315 || ny < 5 || (ny + size) > 235) return true;
  
  if (selectedLevel == 1) {
    for (int i = 0; i < NUM_WALLS_L1; i++) {
      if (nx < wallsLevel1[i].x + wallsLevel1[i].w && nx + size > wallsLevel1[i].x &&
          ny < wallsLevel1[i].y + wallsLevel1[i].h && ny + size > wallsLevel1[i].y) {
        return true; 
      }
    }
  } else {
    for (int i = 0; i < NUM_WALLS_L23; i++) {
      if (nx < wallsLevel2And3[i].x + wallsLevel2And3[i].w && nx + size > wallsLevel2And3[i].x &&
          ny < wallsLevel2And3[i].y + wallsLevel2And3[i].h && ny + size > wallsLevel2And3[i].y) {
        return true; 
      }
    }
  }
  return false;
}

// ==========================================
// IA DE L'ENNEMI : LOGIQUE COMPORTEMENTALE ET PROTECTION
// ==========================================
void updateEnemyAI() {
  const int speed = 1; 

  int targetX = enemyWaypointsX[currentWaypoint];
  int targetY = enemyWaypointsY[currentWaypoint];

  // Détermination de l'axe directionnel
  if (enemyX < targetX)       { enemyDirX = 1;  enemyDirY = 0; }
  else if (enemyX > targetX)  { enemyDirX = -1; enemyDirY = 0; }
  else if (enemyY < targetY)  { enemyDirX = 0;  enemyDirY = 1; }
  else if (enemyY > targetY)  { enemyDirX = 0;  enemyDirY = -1; }
  else {
    enemyDirX = 0;  enemyDirY = 0;
  }

  int stepX = enemyX + (enemyDirX * speed);
  int stepY = enemyY + (enemyDirY * speed);

  // Application du déplacement si aucun mur blanc n'est intercepté
  if (!checkWallCollision(stepX, stepY, enemySize)) {
    enemyX = stepX;
    enemyY = stepY;
  }

  // Changement de waypoint dès que la cible est atteinte ou qu'une collision bloque le passage
  if ((enemyX == targetX && enemyY == targetY) || checkWallCollision(stepX, stepY, enemySize)) {
    currentWaypoint = (currentWaypoint + 1) % NUM_WAYPOINTS;
  }
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
      drawLevelMaze();
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
      if (selectedLevel < 3) selectedLevel++;
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
  randomSeed(analogRead(0)); 
  
  pinMode(BTN_UP, INPUT_PULLDOWN);
  pinMode(BTN_DOWN, INPUT_PULLDOWN);
  pinMode(BTN_LEFT, INPUT_PULLDOWN);
  pinMode(BTN_RIGHT, INPUT_PULLDOWN);

  drawCurrentScene();
}

void loop() {
  // ------------------------------------------
  // LOGIQUE INTERNE DU JEU
  // ------------------------------------------
  if (currentScene == THE_GAME) {
    int nextX = playerX;
    int nextY = playerY;
    bool moved = false;

    if (digitalRead(BTN_UP) == HIGH)    { nextY--; moved = true; }
    if (digitalRead(BTN_DOWN) == HIGH)  { nextY++; moved = true; }
    if (digitalRead(BTN_LEFT) == HIGH)  { nextX--; moved = true; }  
    if (digitalRead(BTN_RIGHT) == HIGH) { nextX++; moved = true; }  

    uint16_t currentBg = (selectedLevel == 1) ? COLOR_MAZE_BROWN : COLOR_MAZE_BLUE;

    if (moved) {
      tft.fillRect(playerX, playerY, playerSize, playerSize, currentBg);
      if (checkWallCollision(nextX, nextY, playerSize)) {
        playerLives--;
        playDyingSound(); 
        if (playerLives <= 0) {
          currentScene = GAME_OVER; 
          gameOverSelection = 0; 
          drawCurrentScene();
        } else {
          resetPlayerPosition(); 
          drawLevelMaze();    
        }
      } else {
        playerX = nextX;
        playerY = nextY;
      }
      uint16_t pColor = (selectedCharacter == 0) ? COLOR_CUBE_RED : COLOR_CUBE_ORANGE; 
      tft.fillRect(playerX, playerY, playerSize, playerSize, pColor);
      
      if (playerX >= 273 && playerY >= 195) {
        currentScene = VICTORY_SCREEN; 
        victorySelection = 0;
        drawCurrentScene();
      }
      delay(12); 
    }

    // CALCUL ET AFFICHAGE ENNEMI
    if (selectedLevel == 3 && millis() - lastEnemyMove > 10) {
      lastEnemyMove = millis();
      
      tft.fillRect(enemyX, enemyY, enemySize, enemySize, currentBg);
      updateEnemyAI();
      tft.fillRect(enemyX, enemyY, enemySize, enemySize, COLOR_ENEMY_GREEN);
      
      // Hitbox Joueur / Ennemi
      if (playerX < enemyX + enemySize && playerX + playerSize > enemyX &&
          playerY < enemyY + enemySize && playerY + playerSize > enemyY) {
        playerLives--;
        playDyingSound();
        if (playerLives <= 0) {
          currentScene = GAME_OVER;
          gameOverSelection = 0;
          drawCurrentScene();
        } else {
          resetPlayerPosition();
          drawLevelMaze();
        }
      }
    }
  }

  // ------------------------------------------
  // LOGIQUE DES MENUS STATIQUES
  // ------------------------------------------
  if (currentScene != THE_GAME) {
    if (digitalRead(BTN_UP) == HIGH) {
      if (checkLongPress(BTN_UP)) { executeSelection(); } 
      else {
        if (currentScene == MAIN_MENU) selection = (selection - 1 + 3) % 3;
        else if (currentScene == LEVEL_SELECTION) selection = (selection - 1 + 4) % 4; 
        else if (currentScene == CONFIRMATION) selection = (selection - 1 + 2) % 2;
        else if (currentScene == OPTIONS) selection = (selection - 1 + 4) % 4; 
        drawCurrentScene();
        waitForRelease(BTN_UP);
      }
    }

    if (digitalRead(BTN_DOWN) == HIGH) {
      if (checkLongPress(BTN_DOWN)) { executeSelection(); } 
      else {
        if (currentScene == MAIN_MENU) selection = (selection + 1) % 3;
        else if (currentScene == LEVEL_SELECTION) selection = (selection + 1) % 4;
        else if (currentScene == CONFIRMATION) selection = (selection + 1) % 2;
        else if (currentScene == OPTIONS) selection = (selection + 1) % 4;
        drawCurrentScene();
        waitForRelease(BTN_DOWN);
      }
    }

    if (digitalRead(BTN_LEFT) == HIGH) {
      if (checkLongPress(BTN_LEFT)) { executeSelection(); } 
      else {
        if (currentScene == CONFIRMATION) selection = 0; 
        else if (currentScene == CHARACTER_SELECTION) { selectedCharacter = 0; } 
        else if (currentScene == GAME_OVER) { gameOverSelection = 0; }
        else if (currentScene == VICTORY_SCREEN) { victorySelection = 0; }
        drawCurrentScene();
        waitForRelease(BTN_LEFT);
      }
    }

    if (digitalRead(BTN_RIGHT) == HIGH) {
      if (checkLongPress(BTN_RIGHT)) { executeSelection(); } 
      else {
        if (currentScene == CONFIRMATION) selection = 1; 
        else if (currentScene == CHARACTER_SELECTION) { selectedCharacter = 1; } 
        else if (currentScene == GAME_OVER) { gameOverSelection = 1; }
        else if (currentScene == VICTORY_SCREEN) { victorySelection = 1; }
        drawCurrentScene();
        waitForRelease(BTN_RIGHT);
      }
    }
  }
}