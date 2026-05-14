#include <unity.h>
#include "game_logic.h"

void setUp(void)    {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// STATE_PLAYING: win only when enemiesEmpty is true
// ---------------------------------------------------------------------------

void test_playing_enemies_present_no_win(void) {
    TEST_ASSERT_FALSE(shouldTriggerWin((int)STATE_PLAYING, false, false));
}
void test_playing_enemies_empty_wins(void) {
    TEST_ASSERT_TRUE(shouldTriggerWin((int)STATE_PLAYING, true, false));
}
void test_playing_boss_segments_empty_alone_no_win(void) {
    // bossSegmentsEmpty is irrelevant in STATE_PLAYING
    TEST_ASSERT_FALSE(shouldTriggerWin((int)STATE_PLAYING, false, true));
}
void test_playing_both_empty_wins(void) {
    TEST_ASSERT_TRUE(shouldTriggerWin((int)STATE_PLAYING, true, true));
}

// ---------------------------------------------------------------------------
// STATE_BOSS_PLAYING: win only when bossSegmentsEmpty is true
// ---------------------------------------------------------------------------

void test_boss_playing_segments_present_no_win(void) {
    TEST_ASSERT_FALSE(shouldTriggerWin((int)STATE_BOSS_PLAYING, false, false));
}
void test_boss_playing_segments_empty_wins(void) {
    TEST_ASSERT_TRUE(shouldTriggerWin((int)STATE_BOSS_PLAYING, false, true));
}
void test_boss_playing_enemies_empty_alone_no_win(void) {
    // enemiesEmpty is irrelevant in STATE_BOSS_PLAYING
    TEST_ASSERT_FALSE(shouldTriggerWin((int)STATE_BOSS_PLAYING, true, false));
}
void test_boss_playing_both_empty_wins(void) {
    TEST_ASSERT_TRUE(shouldTriggerWin((int)STATE_BOSS_PLAYING, true, true));
}

// ---------------------------------------------------------------------------
// All non-game states must never trigger a win regardless of vector state
// ---------------------------------------------------------------------------

void test_state_menu_never_wins(void) {
    TEST_ASSERT_FALSE(shouldTriggerWin((int)STATE_MENU, true, true));
}
void test_state_intro_never_wins(void) {
    TEST_ASSERT_FALSE(shouldTriggerWin((int)STATE_INTRO, true, true));
}
void test_state_level_completed_never_wins(void) {
    TEST_ASSERT_FALSE(shouldTriggerWin((int)STATE_LEVEL_COMPLETED, true, true));
}
void test_state_game_finished_never_wins(void) {
    TEST_ASSERT_FALSE(shouldTriggerWin((int)STATE_GAME_FINISHED, true, true));
}
void test_state_base_destroyed_never_wins(void) {
    TEST_ASSERT_FALSE(shouldTriggerWin((int)STATE_BASE_DESTROYED, true, true));
}
void test_state_gameover_never_wins(void) {
    TEST_ASSERT_FALSE(shouldTriggerWin((int)STATE_GAMEOVER, true, true));
}
void test_state_bonus_intro_never_wins(void) {
    TEST_ASSERT_FALSE(shouldTriggerWin((int)STATE_BONUS_INTRO, true, true));
}
void test_state_bonus_playing_never_wins(void) {
    TEST_ASSERT_FALSE(shouldTriggerWin((int)STATE_BONUS_PLAYING, true, true));
}
void test_state_bonus_simon_never_wins(void) {
    TEST_ASSERT_FALSE(shouldTriggerWin((int)STATE_BONUS_SIMON, true, true));
}

// ---------------------------------------------------------------------------
// Completely invalid state value must not cause a crash or false win
// ---------------------------------------------------------------------------
void test_invalid_state_never_wins(void) {
    TEST_ASSERT_FALSE(shouldTriggerWin(99, true, true));
    TEST_ASSERT_FALSE(shouldTriggerWin(-1, true, true));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_playing_enemies_present_no_win);
    RUN_TEST(test_playing_enemies_empty_wins);
    RUN_TEST(test_playing_boss_segments_empty_alone_no_win);
    RUN_TEST(test_playing_both_empty_wins);
    RUN_TEST(test_boss_playing_segments_present_no_win);
    RUN_TEST(test_boss_playing_segments_empty_wins);
    RUN_TEST(test_boss_playing_enemies_empty_alone_no_win);
    RUN_TEST(test_boss_playing_both_empty_wins);
    RUN_TEST(test_state_menu_never_wins);
    RUN_TEST(test_state_intro_never_wins);
    RUN_TEST(test_state_level_completed_never_wins);
    RUN_TEST(test_state_game_finished_never_wins);
    RUN_TEST(test_state_base_destroyed_never_wins);
    RUN_TEST(test_state_gameover_never_wins);
    RUN_TEST(test_state_bonus_intro_never_wins);
    RUN_TEST(test_state_bonus_playing_never_wins);
    RUN_TEST(test_state_bonus_simon_never_wins);
    RUN_TEST(test_invalid_state_never_wins);
    return UNITY_END();
}
