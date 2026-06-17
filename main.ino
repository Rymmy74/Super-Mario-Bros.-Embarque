#include <TFT_eSPI.h> // Include the TFT display library for ESP32

// Pin definitions for the game buttons
#define BTN_UP 32     // GPIO pin for the Up button
#define BTN_DOWN 33   // GPIO pin for the Down button
#define BTN_LEFT 25   // GPIO pin for the Left button
#define BTN_RIGHT 26  // GPIO pin for the Right button
#define BUZZER_PIN 15 // GPIO pin for the audio buzzer

// Initialize the display controller instance
TFT_eSPI tft = TFT_eSPI();

// 16-bit RGB565 Hex color definitions for the user interface
#define COLOR_BG_TOP 0x190A      // Top gradient color for menus (Dark Indigo)
#define COLOR_BG_BOTTOM 0x9150   // Bottom gradient color for menus (Muted Purple)
#define COLOR_PINK 0xE12B        // Accent pink for titles and special UI frames
#define COLOR_DARK_PURP 0x30E6   // Solid deep purple for UI base lines
#define COLOR_WHITE 0xFFFF       // Pure white for text and primary walls
#define COLOR_BLUE_TEXT 0x229F   // Deep blue text for specific contrast screens
#define COLOR_CHAR_BG 0x112B     // Dark background for character portraits
#define COLOR_MAZE_BROWN 0x38A1  // Level 1 background color (Pac-Man style)
#define COLOR_MAZE_BLUE 0x01A6   // Levels 2 & 3 background color / Selection highlight
#define COLOR_START 0x07E0       // Bright green color for the "S" (Start) zone
#define COLOR_ENEMY_GREEN 0x03E0 // Dark green color representing the enemy cube

// Hex color codes for the selectable player skins
#define COLOR_CUBE_RED 0xF800    // Primary skin: Bright Red
#define COLOR_CUBE_ORANGE 0xFDA0 // Alternative skin: Vibrant Orange

// Background color used for non-selected pill-shaped secondary buttons
#define COLOR_BT_BG 0xDEFB

// State machine enumeration representing all screens/scenes in the game loop
enum Scene
{
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

// Global structural tracking variables
Scene currentScene = MAIN_MENU; // Tracks active state machine stage
int selection = 0;              // Highlights index within active settings menu
int gameOverSelection = 0;      // Highlight pointer for Game Over choices (0=Restart, 1=Menu)
int victorySelection = 0;       // Highlight pointer for Victory choices (0=Continue, 1=Menu)
int selectedLevel = 1;          // Tracks target map layout (1 to 3)
int selectedCharacter = 0;      // Tracks player avatar selection (0=Red, 1=Orange)

// ==========================================
// PLAYER & ENEMY MANIFESTS
// ==========================================
int playerX = 16;          // Initial dynamic horizontal player coordinate (centered in Start box)
int playerY = 18;          // Initial dynamic vertical player coordinate (centered in Start box)
const int playerSize = 12; // Box width/height dimensions of player character
int playerLives = 3;       // Starting structural hit-points asset

// Enemy pathing system profiles (Only used on Level 3)
int enemyX = 145;                // Dynamic horizontal coordinate tracking the enemy position
int enemyY = 105;                // Dynamic vertical coordinate tracking the enemy position
int enemyDirX = 0;               // Normalized horizontal velocity component (-1, 0, or 1)
int enemyDirY = 0;               // Normalized vertical velocity component (-1, 0, or 1)
const int enemySize = 12;        // Box width/height dimensions of enemy character
unsigned long lastEnemyMove = 0; // Millisecond timestamp tracking the enemy's speed tick

// Predetermined map coordinates that the enemy cycles through continuously
int enemyWaypointsX[] = {145, 145, 292, 112, 112, 145};
int enemyWaypointsY[] = {105, 168, 168, 168, 48, 48};
// Dynamically compute size index mapping constraints using array memory lengths
const int NUM_WAYPOINTS = sizeof(enemyWaypointsX) / sizeof(enemyWaypointsX[0]);
int currentWaypoint = 0; // Current destination waypoint tracking index

bool soundOn = true; // Global state toggle enabling or disabling audio alerts

// Blueprint structure defining basic geometric rectangles for obstacle blocks
struct Wall
{
  int x, y, w, h; // X-axis start, Y-axis start, width, and height bounds
};

// ==========================================
// MAZE CONFIGURATION LEVEL 1 (Pac-Man Style Layout)
// ==========================================
Wall wallsLevel1[] = {
    {45, 5, 6, 50}, {45, 55, 50, 6}, {135, 5, 6, 55}, {135, 55, 55, 6}, {230, 5, 6, 55}, {230, 55, 45, 6}, {45, 95, 96, 6}, {90, 95, 6, 50}, {180, 95, 6, 50}, {180, 95, 95, 6}, {45, 145, 6, 50}, {45, 145, 45, 6}, {135, 110, 6, 50}, {135, 145, 45, 6}, {90, 190, 96, 6}, {230, 145, 6, 55}, {230, 145, 45, 6}, {275, 95, 6, 55}, {5, 190, 45, 6}};
const int NUM_WALLS_L1 = sizeof(wallsLevel1) / sizeof(wallsLevel1[0]);

// ==========================================
// MAZE CONFIGURATION LEVELS 2 & 3 (Technical Layout)
// ==========================================
Wall wallsLevel2And3[] = {
    {32, 35, 3, 25}, {32, 35, 32, 3}, {64, 35, 3, 85}, {5, 85, 59, 3}, {36, 120, 3, 85}, {36, 165, 59, 3}, {68, 200, 3, 35}, {95, 35, 3, 140}, {135, 5, 3, 35}, {135, 65, 3, 115}, {135, 35, 45, 3}, {180, 35, 3, 30}, {95, 65, 40, 3}, {135, 90, 85, 3}, {220, 38, 3, 52}, {220, 38, 72, 3}, {185, 120, 80, 3}, {185, 120, 3, 40}, {233, 120, 3, 45}, {233, 162, 62, 3}, {295, 110, 3, 55}, {265, 78, 50, 3}, {68, 190, 222, 3}};
const int NUM_WALLS_L23 = sizeof(wallsLevel2And3) / sizeof(wallsLevel2And3[0]);

// ==========================================
// FUNCTION PROTOTYPES
// ==========================================
// Declaring function blueprints ahead of time so the compiler knows they exist
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
// BEHAVIORAL UTILITY FUNCTIONS
// ==========================================

// Blocks execution loop until the user lets go of a physical button pin
void waitForRelease(int pin)
{
  delay(50); // Small debounce window settling physical metal contacts
  while (digitalRead(pin) == HIGH)
  {
    delay(5);
  } // Spin indefinitely until input goes LOW
}

// Determines if a button is held down for more than 400ms to confirm a choice
bool checkLongPress(int pin)
{
  if (digitalRead(pin) == HIGH)
  {
    unsigned long start = millis(); // Benchmark beginning anchor timestamp
    while (digitalRead(pin) == HIGH)
    {
      if (millis() - start > 400)
      {                      // If elapsed duration exceeds 400 milliseconds
        waitForRelease(pin); // Clear the state by waiting for release
        return true;         // Confirm long-press action occurred
      }
      delay(10);
    }
  }
  return false; // Register as a standard short-press instead
}

// Generates descending dramatic pitch frequencies when a player takes damage
void playDyingSound()
{
  if (!soundOn)
    return; // Exit immediately if sound effects are toggled off

  pinMode(BUZZER_PIN, OUTPUT); // Configure buzzer pin to push electrical current
  tone(BUZZER_PIN, 280, 120);  // Frequency 1: Mid-low warning chirp
  delay(130);
  tone(BUZZER_PIN, 180, 120); // Frequency 2: Lower descending slide pitch
  delay(130);
  tone(BUZZER_PIN, 100, 250); // Frequency 3: Sustained low drone base out
  delay(250);
  noTone(BUZZER_PIN);         // Cut audio transmission lines
  pinMode(BUZZER_PIN, INPUT); // Revert pin mode to prevent floating background noise
}

// Resets coordinates back to the starting area defaults
void resetPlayerPosition()
{
  playerX = 16; // Return home to left spawn container box x
  playerY = 18; // Return home to left spawn container box y
  if (selectedLevel == 3)
  {
    enemyX = 145;        // Reset map index point X for enemy
    enemyY = 105;        // Reset map index point Y for enemy
    currentWaypoint = 0; // Direct logic path back to destination node index zero
    enemyDirX = 0;       // Halt momentum components
    enemyDirY = 0;
  }
}

// ==========================================
// RENDER UI & INTERFACES
// ==========================================

// Renders a smooth background color gradient from top to bottom
void drawBackground()
{
  for (int y = 0; y < 225; y++)
  {
    // Interpolate channels independently across lines to calculate an RGB565 color mix
    uint8_t r = map(y, 0, 225, (COLOR_BG_TOP >> 11) & 0x1F, (COLOR_BG_BOTTOM >> 11) & 0x1F);
    uint8_t g = map(y, 0, 225, (COLOR_BG_TOP >> 5) & 0x3F, (COLOR_BG_BOTTOM >> 5) & 0x3F);
    uint8_t b = map(y, 0, 225, COLOR_BG_TOP & 0x1F, COLOR_BG_BOTTOM & 0x1F);
    uint16_t color = (r << 11) | (g << 5) | b; // Pack colors back into a single 16-bit integer

    tft.drawFastHLine(0, y, 320, color); // Draw horizontal pixel scanlines across the display width (320px)
  }
  tft.fillRect(0, 225, 320, 15, COLOR_DARK_PURP); // Bottom informational bar base border
}

// Draws angular block-style choice buttons with dynamic text coloring
void drawOmniButton(String text, int x, int y, int w, int h, bool isSelected, int textOffsetXX = 12, int textOffsetYY = 5)
{
  if (isSelected)
  {
    tft.fillRoundRect(x, y, w, h, 8, COLOR_WHITE); // Selected button gets a white background
    tft.setTextColor(COLOR_PINK);                  // And pink text
  }
  else
  {
    tft.fillRoundRect(x, y, w, h, 8, COLOR_PINK); // Unselected button gets a pink background
    tft.setTextColor(COLOR_WHITE);                // And white text
  }
  tft.drawString(text, x + textOffsetXX, y + textOffsetYY); // Print label within coordinates
}

// Draws highly rounded pill-style option buttons for game-over screens
void drawPillButton(String text, int x, int y, int w, int h, bool isSelected)
{
  int r = h / 2; // Radius matches half of total height to ensure perfect semi-circle cap rounding
  int textWidth = tft.textWidth(text);
  int textOffsetXX = (w - textWidth) / 2;            // Automatically centers text horizontally
  int textOffsetYY = (h - tft.fontHeight()) / 2 + 1; // Automatically centers text vertically
//w is for width , to center it we calculate the left over blank space

  if (isSelected)
  {
    tft.fillRoundRect(x, y, w, h, r, COLOR_MAZE_BLUE); // Blue background if highlighted
    tft.setTextColor(COLOR_WHITE);
  }
  else
  {
    tft.fillRoundRect(x, y, w, h, r, COLOR_BT_BG); // Pastel default background if not highlighted
    tft.setTextColor(COLOR_BLUE_TEXT);
  }
  tft.drawString(text, x + textOffsetXX, y + textOffsetYY); // Draw text inside button center
}

// Renders the character preview selector interface boxes
void drawCubeCharacter(int x, int y, bool isSelected, uint16_t cubeColor)
{
  tft.fillRect(x - 8, y - 8, 66, 66, COLOR_CHAR_BG); // Render dark box frame background
  if (isSelected)
  {
    tft.drawRect(x - 6, y - 6, 62, 62, COLOR_WHITE); // Highlight inner white border
    tft.drawRect(x - 5, y - 5, 60, 60, COLOR_WHITE); // Double thickness accent line
  }
  tft.fillRect(x, y, 50, 50, cubeColor); // Draw the actual playable player color avatar square
}

// ==========================================
// MAZE RENDER & LAYOUT GENERATOR
// ==========================================
void drawLevelMaze()
{
  // Set the maze floor color based on the selected level
  uint16_t currentBg = (selectedLevel == 1) ? COLOR_MAZE_BROWN : COLOR_MAZE_BLUE;
  tft.fillScreen(currentBg); // Clear display screen layout

  // Draw bright pink protective boundary frame lines around display borders
  tft.fillRect(0, 0, 320, 5, COLOR_PINK);   // Outer Top
  tft.fillRect(0, 235, 320, 5, COLOR_PINK); // Outer Bottom
  tft.fillRect(0, 0, 5, 240, COLOR_PINK);   // Outer Left
  tft.fillRect(315, 0, 5, 240, COLOR_PINK); // Outer Right

  // Iterate over configuration sets and render solid obstacle structures on the screen
  if (selectedLevel == 1)
  {
    for (int i = 0; i < NUM_WALLS_L1; i++)
    {
      tft.fillRect(wallsLevel1[i].x, wallsLevel1[i].y, wallsLevel1[i].w, wallsLevel1[i].h, COLOR_WHITE);
    }
  }
  else
  {
    for (int i = 0; i < NUM_WALLS_L23; i++)
    {
      tft.fillRect(wallsLevel2And3[i].x, wallsLevel2And3[i].y, wallsLevel2And3[i].w, wallsLevel2And3[i].h, COLOR_WHITE);
    }
  }

  // Draw the green starting safety zone container ("S")
  tft.fillRect(5, 5, 35, 38, COLOR_START);
  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(1);
  tft.drawString("S", 19, 20);

  // Draw the pink exit/endpoint zone container ("E")
  tft.fillRect(273, 195, 42, 40, COLOR_PINK);
  tft.drawString("E", 290, 210);

  // Render player hit-points counter tracking text
  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(1);
  tft.drawString("LIVES: " + String(playerLives), 145, 11);

  // Render the player avatar cube on top of the map grid positions
  uint16_t pColor = (selectedCharacter == 0) ? COLOR_CUBE_RED : COLOR_CUBE_ORANGE;
  tft.fillRect(playerX, playerY, playerSize, playerSize, pColor);

  // If playing Level 3, render the green enemy box onto its starting position
  if (selectedLevel == 3)
  {
    tft.fillRect(enemyX, enemyY, enemySize, enemySize, COLOR_ENEMY_GREEN);
  }
}

// Axis-Aligned Bounding Box (AABB) collision tracker checking framework blocks
bool checkWallCollision(int nx, int ny, int size)
{
  // Check if player position is outside the outer map boundaries
  if (nx < 5 || (nx + size) > 315 || ny < 5 || (ny + size) > 235) //new x possition - new y position
    return true;

  // Check overlap collision states against active Level 1 wall obstacle structural indices
  if (selectedLevel == 1)
  {
    for (int i = 0; i < NUM_WALLS_L1; i++)
    {
      if (nx < wallsLevel1[i].x + wallsLevel1[i].w //The left edge of the player is to the left of the wall’s right edge.
         && nx + size > wallsLevel1[i].x && //The left edge of the player is to the left of the wall’s right edge.
          ny < wallsLevel1[i].y + wallsLevel1[i].h && //The top edge of the player is higher than the wall’s bottom edge.
           ny + size > wallsLevel1[i].y) //The bottom edge of the player is lower than the wall’s top edge.
      {
        return true; // Overlap detected
      }
    }
  }
  else
  { // Check overlap collision states against active Level 2 & 3 obstacle matrices
    for (int i = 0; i < NUM_WALLS_L23; i++)
    {
      if (nx < wallsLevel2And3[i].x + wallsLevel2And3[i].w && nx + size > wallsLevel2And3[i].x &&
          ny < wallsLevel2And3[i].y + wallsLevel2And3[i].h && ny + size > wallsLevel2And3[i].y)
      {
        return true; // Overlap detected
      }
    }
  }
  return false; // Path is clear
}

// ==========================================
// LEVEL 3 ENEMY INTEL SYSTEM (WAYPOINT AI)
// ==========================================
void updateEnemyAI()
{
  const int speed = 1; // Movement step distance per speed update tick

  // Track the coordinates of our current target waypoint
  int targetX = enemyWaypointsX[currentWaypoint];
  int targetY = enemyWaypointsY[currentWaypoint];

  // Set direction vector variables to step towards the targeted waypoint coordinates
  if (enemyX < targetX)
  {
    enemyDirX = 1; //move right
    enemyDirY = 0; //don't move
  }
  else if (enemyX > targetX)
  {
    enemyDirX = -1; //move left
    enemyDirY = 0;
  }
  else if (enemyY < targetY)
  {
    enemyDirX = 0;
    enemyDirY = 1; //move down
  }
  else if (enemyY > targetY)
  {
    enemyDirX = 0;
    enemyDirY = -1; //move up
  }
  else
  {
    enemyDirX = 0; //If it is standing perfectly on top of the target, set both to 0 (stop moving).
    enemyDirY = 0;
  }

  // Calculate the next step coordinates
  int stepX = enemyX + (enemyDirX * speed);
  int stepY = enemyY + (enemyDirY * speed);

  // Move the enemy if the calculated path does not crash into a wall layout structure
  if (!checkWallCollision(stepX, stepY, enemySize))
  {
    enemyX = stepX;
    enemyY = stepY;
  }

  // Advance destination targets index looping if target reached, or path blocked by obstacles
  if ((enemyX == targetX && enemyY == targetY) || checkWallCollision(stepX, stepY, enemySize))
  {
    currentWaypoint = (currentWaypoint + 1) % NUM_WAYPOINTS; // Loop back to zero when reaching the last waypoint
  }
}

// Comprehensive central dispatcher system updating layout text elements
void drawCurrentScene()
{
  // Skip clear calls on solid color screen modules to optimize processing loops
  if (currentScene != STARTING_SCREEN && currentScene != LOADING_SCREEN &&
      currentScene != CHARACTER_SELECTION && currentScene != THE_GAME &&
      currentScene != GAME_OVER && currentScene != VICTORY_SCREEN)
  {
    drawBackground();
  }

  tft.setTextSize(2); // Set default global typography baseline scale factors
  switch (currentScene)
  {

  case MAIN_MENU:
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(3);
    tft.drawString("Omni", 125, 30); // Draw game title
    tft.setTextSize(2);
    // Main menu button selections
    drawOmniButton("PLAY", 100, 95, 120, 28, (selection == 0), 35);
    drawOmniButton("OPTIONS", 100, 135, 120, 28, (selection == 1), 18);
    drawOmniButton("QUIT", 100, 175, 120, 28, (selection == 2), 35);
    break;

  case LEVEL_SELECTION:
    tft.setTextColor(COLOR_PINK);
    tft.drawString("SELECT THE LEVEL", 25, 22);
    for (int i = 0; i < 3; i++)
    {
      drawOmniButton("LEVEL " + String(i + 1), 100, 75 + (i * 35), 120, 26, (selection == i), 22);
    }
    drawOmniButton("RETURN", 210, 190, 100, 26, (selection == 3), 15);
    break;

  case CONFIRMATION:
    tft.fillRoundRect(40, 65, 240, 55, 12, COLOR_PINK); // Confirmation prompt box frame
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(1);
    tft.drawString("YOU CHOSE LEVEL " + String(selectedLevel), 95, 75);
    tft.drawString("DO YOU WANT TO PROCEED?", 78, 95);
    tft.setTextSize(2);
    drawOmniButton("YES", 60, 145, 70, 26, (selection == 0), 16);
    drawOmniButton("NO", 180, 145, 60, 26, (selection == 1), 18);
    break;

  case OPTIONS:
    tft.setTextColor(COLOR_PINK);
    tft.drawString("OPTIONS", 25, 18);
    drawOmniButton("SOUND", 25, 60, 100, 26, (selection == 0), 20);
    // Sound toggle status switches
    drawOmniButton("ON", 160, 60, 45, 24, soundOn, 15, 4);
    drawOmniButton("OFF", 215, 60, 45, 24, !soundOn, 6, 4);
    drawOmniButton("CREDITS", 25, 100, 110, 26, (selection == 1), 15);
    drawOmniButton("RETURN", 185, 190, 100, 26, (selection == 2), 15);
    break;

  case CREDITS:
    tft.setTextColor(COLOR_PINK);
    tft.drawString("Omni - CREDITS", 25, 18);
    tft.fillRoundRect(25, 50, 215, 165, 12, COLOR_PINK); // Display structural background placard
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(1);
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
    tft.fillScreen(0x0842); // Clean structural backdrop splash paint
    tft.setTextColor(COLOR_BLUE_TEXT);
    tft.setTextSize(3);
    tft.drawString("STARTING", 85, 105);
    break;

  case CHARACTER_SELECTION:
    tft.fillScreen(COLOR_CHAR_BG);
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(2);
    tft.drawString("SELECT YOUR CHARACTER", 35, 30);
    // Render preview boxes for selectable cube styles
    drawCubeCharacter(75, 95, (selectedCharacter == 0), COLOR_CUBE_RED);
    drawCubeCharacter(195, 95, (selectedCharacter == 1), COLOR_CUBE_ORANGE);
    tft.setTextSize(1);
    tft.drawString("Hold any button to Confirm", 85, 190);
    break;

  case LOADING_SCREEN:
    tft.fillScreen(0x0842);
    tft.setTextColor(COLOR_BLUE_TEXT);
    tft.setTextSize(3);
    tft.drawString("LOADING", 95, 105);
    break;

  case THE_GAME:
    drawLevelMaze(); // Jump directory branch execution line to draw active map elements
    break;

  case GAME_OVER:
    tft.fillScreen(0x0002); // Dark slate backdrop paint color
    tft.setTextColor(COLOR_BLUE_TEXT);
    tft.setTextSize(3);
    tft.drawString("GAME OVER", 85, 90);
    tft.setTextSize(2);
    drawPillButton("RESTART", 50, 145, 95, 28, (gameOverSelection == 0));
    drawPillButton("RETURN", 175, 145, 95, 28, (gameOverSelection == 1));
    break;

  case VICTORY_SCREEN:
    tft.fillScreen(0x0002);
    tft.setTextColor(COLOR_BLUE_TEXT);
    tft.setTextSize(3);
    tft.drawString("YOU WON !", 95, 90);
    tft.setTextSize(2);
    drawPillButton("CONTINUE", 45, 145, 105, 28, (victorySelection == 0));
    drawPillButton("RETURN", 175, 145, 95, 28, (victorySelection == 1));
    break;
  }
}

// Action executor called during menu navigation confirmation triggers
void executeSelection()
{
  switch (currentScene)
  {
  case MAIN_MENU:
    switch (selection)
    {
    case 0:
      currentScene = LEVEL_SELECTION;
      selection = 0;
      break;
    case 1:
      currentScene = OPTIONS;
      selection = 0;
      break;
    case 2:
      tft.fillScreen(TFT_BLACK);
      while (1)
        ;
      break; // Halt operations, creating clean screen state
    }
    break;

  case LEVEL_SELECTION:
    if (selection == 3)
    {
      currentScene = MAIN_MENU;
      selection = 0; // Return back to primary menu
    }
    else
    {
      selectedLevel = selection + 1; // Translate loop layout references mapping matrix bounds
      currentScene = CONFIRMATION;
      selection = 0;
    }
    break;

  case CONFIRMATION:
    if (selection == 0)
    {
      currentScene = STARTING_SCREEN;
      drawCurrentScene();
      delay(1200); // Cinematic pause window
      currentScene = CHARACTER_SELECTION;
      drawCurrentScene();
    }
    else
    {
      currentScene = LEVEL_SELECTION;
      selection = 0;
      drawCurrentScene();
    }
    return; // Return immediately to bypass the automatic draw call at the bottom of the function

  case CHARACTER_SELECTION:
    currentScene = LOADING_SCREEN;
    drawCurrentScene();
    delay(1200); // Simulate processing loading sequences
    playerLives = 3;
    resetPlayerPosition();
    currentScene = THE_GAME;
    break;

  case GAME_OVER:
    if (gameOverSelection == 0)
    {
      playerLives = 3;
      resetPlayerPosition();
      currentScene = THE_GAME; // Reset dynamic game parameters
    }
    else
    {
      currentScene = MAIN_MENU;
      selection = 0;
    }
    break;

  case VICTORY_SCREEN:
    if (victorySelection == 0)
    {
      if (selectedLevel < 3)
        selectedLevel++; // Auto-advance map complexity layout numbers
      playerLives = 3;
      resetPlayerPosition();
      currentScene = THE_GAME;
    }
    else
    {
      currentScene = MAIN_MENU;
      selection = 0;
    }
    break;

  case OPTIONS:
    switch (selection)
    {
    case 0:
      soundOn = !soundOn;
      break; // Invert global flag parameter status states
    case 1:
      currentScene = CREDITS;
      selection = 0;
      break;
    case 2:
      currentScene = MAIN_MENU;
      selection = 1;
      break;
    }
    break;

  case CREDITS:
    if (selection == 0)
    {
      currentScene = OPTIONS;
      selection = 1;
    }
    break;

  default:
    break;
  }

  drawCurrentScene(); // Re-render updated interfaces immediately
}

// Microcontroller initialization block running once at start-up
void setup()
{
  tft.init();                // Send setup commands to the TFT hardware panel controller
  tft.setRotation(1);        // Set display orientation to Landscape (320x240)
  randomSeed(analogRead(0)); // Seed random generation algorithm using noise from an empty analog channel pin

  // Configure input pins with internal pull-down resistors (Default state is LOW, clicks pull state HIGH)
  pinMode(BTN_UP, INPUT_PULLDOWN);
  pinMode(BTN_DOWN, INPUT_PULLDOWN);
  pinMode(BTN_LEFT, INPUT_PULLDOWN);
  pinMode(BTN_RIGHT, INPUT_PULLDOWN);

  drawCurrentScene(); // Run the initial interface render pass
}

// Continuous core processing runtime execution pipeline loop
void loop()
{
  // ------------------------------------------
  // IN-GAME REAL-TIME LOGIC LOOP
  // ------------------------------------------
  if (currentScene == THE_GAME)
  {
    int nextX = playerX; // Temporary layout copy parameters tracking tentative step target offsets
    int nextY = playerY;
    bool moved = false; // Flag tracking movement input state updates

    // Read input registers and modify target movement coordinates accordingly
    if (digitalRead(BTN_UP) == HIGH)
    {
      nextY--;
      moved = true;
    }
    if (digitalRead(BTN_DOWN) == HIGH)
    {
      nextY++;
      moved = true;
    }
    if (digitalRead(BTN_LEFT) == HIGH)
    {
      nextX--;
      moved = true;
    }
    if (digitalRead(BTN_RIGHT) == HIGH)
    {
      nextX++;
      moved = true;
    }

    uint16_t currentBg = (selectedLevel == 1) ? COLOR_MAZE_BROWN : COLOR_MAZE_BLUE;

    if (moved)
    {
      // If the player is inside the green spawn zone dimensions (x: 5-40, y: 5-43)
      if (playerX >= 5 && playerX <= 40 && playerY >= 5 && playerY <= 43)
      {
        tft.fillRect(playerX, playerY, playerSize, playerSize, COLOR_START); // Overwrite old player spot with green
      }
      else
      {
        tft.fillRect(playerX, playerY, playerSize, playerSize, currentBg); // Overwrite old player spot with standard floor background
      }

      // Check collision state profiles against next targeted step values
      if (checkWallCollision(nextX, nextY, playerSize))
      {
        playerLives--;    // Deduct point asset score allocations
        playDyingSound(); // Trigger descending alert frequency audio tones
        if (playerLives <= 0)
        {
          currentScene = GAME_OVER;
          gameOverSelection = 0;
          drawCurrentScene();
        }
        else
        {
          resetPlayerPosition(); // Return entity back to base
          drawLevelMaze();       // Redraw the entire maze structure
        }
      }
      else
      {
        // If there is no collision, update player positional coordinates
        playerX = nextX;
        playerY = nextY;
      }

      // Render player token box sprite onto display panel
      uint16_t pColor = (selectedCharacter == 0) ? COLOR_CUBE_RED : COLOR_CUBE_ORANGE;
      tft.fillRect(playerX, playerY, playerSize, playerSize, pColor);

      // Check for win condition (Entering the pink endpoint box x >= 273, y >= 195)
      if (playerX >= 273 && playerY >= 195)
      {
        currentScene = VICTORY_SCREEN;
        victorySelection = 0;
        drawCurrentScene();
      }
      delay(12); // Pacing delay controlling movement velocity loops
    }

    // Level 3 Enemy updates triggered sequentially across 10 millisecond intervals
    if (selectedLevel == 3 && millis() - lastEnemyMove > 10)
    {
      lastEnemyMove = millis(); // Reset loop interval validation clock anchor

      tft.fillRect(enemyX, enemyY, enemySize, enemySize, currentBg);         // Erase old enemy position trailing pixels
      updateEnemyAI();                                                       // Run calculation routine update tracks
      tft.fillRect(enemyX, enemyY, enemySize, enemySize, COLOR_ENEMY_GREEN); // Render enemy cube at its updated position

      // Simple intersection calculation checking AABB box overlap parameters between player and enemy
      if (playerX < enemyX + enemySize && playerX + playerSize > enemyX &&
          playerY < enemyY + enemySize && playerY + playerSize > enemyY)
      {
        playerLives--;
        playDyingSound();
        if (playerLives <= 0)
        {
          currentScene = GAME_OVER;
          gameOverSelection = 0;
          drawCurrentScene();
        }
        else
        {
          resetPlayerPosition();
          drawLevelMaze();
        }
      }
    }
  }

  // ------------------------------------------
  // STATIC NON-GAME UI NAVIGATION CONTROL LOGIC
  // ------------------------------------------
  if (currentScene != THE_GAME)
  {

    // Process input handlers targeting Up Arrow controls
    if (digitalRead(BTN_UP) == HIGH)
    {
      if (checkLongPress(BTN_UP))
      {
        executeSelection(); // Trigger confirming activation state profiles
      }
      else
      {
        // Simple index shifting tracking constraints wrapping limits backwards
        if (currentScene == MAIN_MENU)
          selection = (selection - 1 + 3) % 3;
        else if (currentScene == LEVEL_SELECTION)
          selection = (selection - 1 + 4) % 4;
        else if (currentScene == CONFIRMATION)
          selection = (selection - 1 + 2) % 2;
        else if (currentScene == OPTIONS)
          selection = (selection - 1 + 3) % 3;
        drawCurrentScene();
        waitForRelease(BTN_UP); // Stabilize debounce runtime constraints
      }
    }

    // Process input handlers targeting Down Arrow controls
    if (digitalRead(BTN_DOWN) == HIGH)
    {
      if (checkLongPress(BTN_DOWN))
      {
        executeSelection();
      }
      else
      {
        // Forward loop option iteration parameters tracking constraints indexes
        if (currentScene == MAIN_MENU)
          selection = (selection + 1) % 3;
        else if (currentScene == LEVEL_SELECTION)
          selection = (selection + 1) % 4;
        else if (currentScene == CONFIRMATION)
          selection = (selection + 1) % 2;
        else if (currentScene == OPTIONS)
          selection = (selection + 1) % 3;
        drawCurrentScene();
        waitForRelease(BTN_DOWN);
      }
    }

    // Process input handlers targeting Left Arrow controls
    if (digitalRead(BTN_LEFT) == HIGH)
    {
      if (checkLongPress(BTN_LEFT))
      {
        executeSelection();
      }
      else
      {
        // Map horizontal option choice allocations
        if (currentScene == CONFIRMATION)
          selection = 0; // Choose "YES" option indices index zero
        else if (currentScene == CHARACTER_SELECTION)
        {
          selectedCharacter = 0;
        } // Highlight Red Avatar
        else if (currentScene == GAME_OVER)
        {
          gameOverSelection = 0;
        } // Highlight Restart Button
        else if (currentScene == VICTORY_SCREEN)
        {
          victorySelection = 0;
        } // Highlight Continue Button
        drawCurrentScene();
        waitForRelease(BTN_LEFT);
      }
    }

    // Process input handlers targeting Right Arrow controls
    if (digitalRead(BTN_RIGHT) == HIGH)
    {
      if (checkLongPress(BTN_RIGHT))
      {
        executeSelection();
      }
      else
      {
        // Map alternative choice selection points
        if (currentScene == CONFIRMATION)
          selection = 1; // Choose "NO" option indices index one
        else if (currentScene == CHARACTER_SELECTION)
        {
          selectedCharacter = 1;
        } // Highlight Orange Avatar
        else if (currentScene == GAME_OVER)
        {
          gameOverSelection = 1;
        } // Highlight Return Menu Button
        else if (currentScene == VICTORY_SCREEN)
        {
          victorySelection = 1;
        } // Highlight Return Menu Button
        drawCurrentScene();
        waitForRelease(BTN_RIGHT);
      }
    }
  }
}