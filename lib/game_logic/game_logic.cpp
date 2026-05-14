#include "game_logic.h"

// ---------------------------------------------------------------------------
// getSimonStageLength
// ---------------------------------------------------------------------------
int getSimonStageLength(int stage) {
    const int lens[] = {4, 5, 6, 8, 9, 11, 13, 15, 17};
    if (stage >= 0 && stage <= 8) return lens[stage];
    return 17;
}

// ---------------------------------------------------------------------------
// getComboColor
// Priority: all-three > two-button combos > singles.
// Mirrors the inline combo logic used in the game task and Simon bonus.
// ---------------------------------------------------------------------------
int getComboColor(bool blue, bool red, bool green) {
    if (red && green && blue) return 7;
    if (red && green)         return 4;
    if (red && blue)          return 5;
    if (green && blue)        return 6;
    if (blue)                 return 1;
    if (red)                  return 2;
    if (green)                return 3;
    return 0;
}

// ---------------------------------------------------------------------------
// getScoreRankFor
// ---------------------------------------------------------------------------
const char* getScoreRankFor(int score) {
    if (score >= 100000) return "S+";
    if (score >= 70000)  return "S";
    if (score >= 40000)  return "A";
    if (score >= 20000)  return "B";
    if (score >= 8000)   return "C";
    return "R";
}

// ---------------------------------------------------------------------------
// getColorComponents
// Canonical palette values that match loadColors() in main.cpp.
// ---------------------------------------------------------------------------
void getColorComponents(int colorCode, uint8_t &r, uint8_t &g, uint8_t &b) {
    switch (colorCode) {
        case 1: r =   0; g =   0; b = 255; break; // blue
        case 2: r = 255; g =   0; b =   0; break; // red
        case 3: r =   0; g = 255; b =   0; break; // green
        case 4: r = 255; g = 255; b =   0; break; // yellow
        case 5: r = 255; g =   0; b = 255; break; // magenta
        case 6: r =   0; g = 255; b = 255; break; // cyan
        case 7: r = 255; g = 255; b = 255; break; // white
        default: r = 0; g = 0; b = 0; break;       // black / invalid
    }
}

// ---------------------------------------------------------------------------
// getLedColorId
// Mirrors getLedColorIdTelemetry() thresholds from main.cpp exactly.
// ---------------------------------------------------------------------------
uint8_t getLedColorId(uint8_t r, uint8_t g, uint8_t b) {
    if (r < 20  && g < 20  && b < 20)  return 0; // black
    if (r > 180 && g > 180 && b > 180) return 7; // white
    if (r > 120 && g > 120 && b < 100) return 4; // yellow
    if (r > 120 && b > 120 && g < 100) return 5; // magenta
    if (g > 120 && b > 120 && r < 100) return 6; // cyan
    if (b >= r  && b >= g)             return 1; // blue dominant
    if (r >= g  && r >= b)             return 2; // red dominant
    if (g >= r  && g >= b)             return 3; // green dominant
    return 0;
}

// ---------------------------------------------------------------------------
// computeAccuracy
// ---------------------------------------------------------------------------
int computeAccuracy(int achieved, int maxPossible) {
    if (maxPossible <= 0) return 0;
    long value = ((long)achieved * 100L) / (long)maxPossible;
    if (value < 0)   value = 0;
    if (value > 100) value = 100;
    return (int)value;
}

// ---------------------------------------------------------------------------
// computeLevelScore
// Pure extraction of the scoring math from calculateLevelScore() in main.cpp.
// The entity-count lookup (boss hp * segments) is the caller's responsibility.
// ---------------------------------------------------------------------------
ScoreResult computeLevelScore(int level, unsigned long durationMs,
                               int entityCount, int bossType, int numLeds) {
    int basePoints   = entityCount * 100 * level;
    int maxTimeBonus = basePoints * 3;
    int timeBonus    = 0;

    if (bossType == 3) {
        // Boss 3 always awards the full time bonus (score is time-independent).
        timeBonus = maxTimeBonus;
    } else {
        unsigned long targetTime;
        if (bossType == 2) {
            targetTime = 36000UL;
        } else {
            targetTime = 3000UL
                + (unsigned long)(numLeds * 15)
                + (unsigned long)(entityCount * 300);
        }
        if (durationMs <= targetTime) {
            timeBonus = maxTimeBonus;
        } else {
            float ratio = (float)targetTime / (float)durationMs;
            timeBonus   = (int)((float)maxTimeBonus * ratio);
        }
    }

    ScoreResult result;
    result.achieved    = basePoints + timeBonus;
    result.maxPossible = basePoints * 4;
    return result;
}

// ---------------------------------------------------------------------------
// shouldTriggerWin
// ---------------------------------------------------------------------------
bool shouldTriggerWin(int stateVal, bool enemiesEmpty, bool bossSegmentsEmpty) {
    if (stateVal == (int)STATE_PLAYING      && enemiesEmpty)      return true;
    if (stateVal == (int)STATE_BOSS_PLAYING && bossSegmentsEmpty) return true;
    return false;
}
