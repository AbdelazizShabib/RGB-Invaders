#pragma once
#include <stdint.h>

// ---------------------------------------------------------------------------
// GameState enum — shared between main.cpp and native unit tests.
// main.cpp includes this header; the enum is no longer redefined there.
// ---------------------------------------------------------------------------
enum GameState {
    STATE_MENU, STATE_INTRO, STATE_PLAYING, STATE_BOSS_PLAYING,
    STATE_LEVEL_COMPLETED, STATE_GAME_FINISHED, STATE_BASE_DESTROYED,
    STATE_GAMEOVER, STATE_BONUS_INTRO, STATE_BONUS_PLAYING, STATE_BONUS_SIMON
};

// ---------------------------------------------------------------------------
// ScoreResult — returned by computeLevelScore()
// ---------------------------------------------------------------------------
struct ScoreResult {
    int achieved;
    int maxPossible;
};

// ---------------------------------------------------------------------------
// Pure logic function declarations
// ---------------------------------------------------------------------------

// Returns the required input-sequence length for Simon Says stage [0-8].
// Returns 17 for any out-of-range stage.
int getSimonStageLength(int stage);

// Maps a button combination to a color code 0-7.
// 0=none 1=blue 2=red 3=green 4=yellow 5=magenta 6=cyan 7=white
int getComboColor(bool blue, bool red, bool green);

// Maps a score to a rank string: "R","C","B","A","S","S+"
const char* getScoreRankFor(int score);

// Fills r,g,b with the canonical RGB values for color codes 1-7.
// Sets all to 0 for code 0 or any invalid code.
void getColorComponents(int colorCode, uint8_t &r, uint8_t &g, uint8_t &b);

// Classifies an RGB triplet back into a color code 0-7.
// Inverse of getColorComponents for the canonical palette values.
uint8_t getLedColorId(uint8_t r, uint8_t g, uint8_t b);

// Returns player accuracy as a percentage clamped to [0, 100].
// Returns 0 when maxPossible <= 0.
int computeAccuracy(int achieved, int maxPossible);

// Pure level-score calculation extracted from calculateLevelScore().
// level      : current level number (1+)
// durationMs : elapsed time in milliseconds
// entityCount: enemy count or total boss HP units
// bossType   : 0=normal wave, 1=Boss1, 2=Boss2, 3=Boss3
// numLeds    : CONFIG_NUM_LEDS value used for target-time calculation
ScoreResult computeLevelScore(int level, unsigned long durationMs,
                               int entityCount, int bossType, int numLeds);

// Returns true when game state + vector-empty flags warrant a level win.
// stateVal          : current GameState cast to int
// enemiesEmpty      : enemies vector is empty
// bossSegmentsEmpty : bossSegments vector is empty
bool shouldTriggerWin(int stateVal, bool enemiesEmpty, bool bossSegmentsEmpty);
