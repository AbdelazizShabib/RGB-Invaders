#include <stdio.h>
#include <unity.h>
#include "game_logic.h"

void setUp(void)    {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// STATE_PLAYING: win only when enemiesEmpty is true
// ---------------------------------------------------------------------------

void test_playing_enemies_present_no_win(void) {
    bool result = shouldTriggerWin((int)STATE_PLAYING, false, false);
    printf("  STATE_PLAYING  enemiesEmpty=false  bossEmpty=false  -> win=%s\n", result ? "YES" : "NO");
    TEST_ASSERT_FALSE(result);
}
void test_playing_enemies_empty_wins(void) {
    bool result = shouldTriggerWin((int)STATE_PLAYING, true, false);
    printf("  STATE_PLAYING  enemiesEmpty=true   bossEmpty=false  -> win=%s\n", result ? "YES" : "NO");
    TEST_ASSERT_TRUE(result);
}
void test_playing_boss_segments_empty_alone_no_win(void) {
    bool result = shouldTriggerWin((int)STATE_PLAYING, false, true);
    printf("  STATE_PLAYING  enemiesEmpty=false  bossEmpty=true   -> win=%s  (boss flag irrelevant)\n", result ? "YES" : "NO");
    TEST_ASSERT_FALSE(result);
}
void test_playing_both_empty_wins(void) {
    bool result = shouldTriggerWin((int)STATE_PLAYING, true, true);
    printf("  STATE_PLAYING  enemiesEmpty=true   bossEmpty=true   -> win=%s\n", result ? "YES" : "NO");
    TEST_ASSERT_TRUE(result);
}

// ---------------------------------------------------------------------------
// STATE_BOSS_PLAYING: win only when bossSegmentsEmpty is true
// ---------------------------------------------------------------------------

void test_boss_playing_segments_present_no_win(void) {
    bool result = shouldTriggerWin((int)STATE_BOSS_PLAYING, false, false);
    printf("  STATE_BOSS_PLAYING  enemiesEmpty=false  bossEmpty=false  -> win=%s\n", result ? "YES" : "NO");
    TEST_ASSERT_FALSE(result);
}
void test_boss_playing_segments_empty_wins(void) {
    bool result = shouldTriggerWin((int)STATE_BOSS_PLAYING, false, true);
    printf("  STATE_BOSS_PLAYING  enemiesEmpty=false  bossEmpty=true   -> win=%s\n", result ? "YES" : "NO");
    TEST_ASSERT_TRUE(result);
}
void test_boss_playing_enemies_empty_alone_no_win(void) {
    bool result = shouldTriggerWin((int)STATE_BOSS_PLAYING, true, false);
    printf("  STATE_BOSS_PLAYING  enemiesEmpty=true   bossEmpty=false  -> win=%s  (enemies flag irrelevant)\n", result ? "YES" : "NO");
    TEST_ASSERT_FALSE(result);
}
void test_boss_playing_both_empty_wins(void) {
    bool result = shouldTriggerWin((int)STATE_BOSS_PLAYING, true, true);
    printf("  STATE_BOSS_PLAYING  enemiesEmpty=true   bossEmpty=true   -> win=%s\n", result ? "YES" : "NO");
    TEST_ASSERT_TRUE(result);
}

// ---------------------------------------------------------------------------
// All non-game states must never trigger a win regardless of vector state
// ---------------------------------------------------------------------------

void test_state_menu_never_wins(void) {
    bool result = shouldTriggerWin((int)STATE_MENU, true, true);
    printf("  STATE_MENU           -> win=%s\n", result ? "YES" : "NO");
    TEST_ASSERT_FALSE(result);
}
void test_state_intro_never_wins(void) {
    bool result = shouldTriggerWin((int)STATE_INTRO, true, true);
    printf("  STATE_INTRO          -> win=%s\n", result ? "YES" : "NO");
    TEST_ASSERT_FALSE(result);
}
void test_state_level_completed_never_wins(void) {
    bool result = shouldTriggerWin((int)STATE_LEVEL_COMPLETED, true, true);
    printf("  STATE_LEVEL_COMPLETED-> win=%s\n", result ? "YES" : "NO");
    TEST_ASSERT_FALSE(result);
}
void test_state_game_finished_never_wins(void) {
    bool result = shouldTriggerWin((int)STATE_GAME_FINISHED, true, true);
    printf("  STATE_GAME_FINISHED  -> win=%s\n", result ? "YES" : "NO");
    TEST_ASSERT_FALSE(result);
}
void test_state_base_destroyed_never_wins(void) {
    bool result = shouldTriggerWin((int)STATE_BASE_DESTROYED, true, true);
    printf("  STATE_BASE_DESTROYED -> win=%s\n", result ? "YES" : "NO");
    TEST_ASSERT_FALSE(result);
}
void test_state_gameover_never_wins(void) {
    bool result = shouldTriggerWin((int)STATE_GAMEOVER, true, true);
    printf("  STATE_GAMEOVER       -> win=%s\n", result ? "YES" : "NO");
    TEST_ASSERT_FALSE(result);
}
void test_state_bonus_intro_never_wins(void) {
    bool result = shouldTriggerWin((int)STATE_BONUS_INTRO, true, true);
    printf("  STATE_BONUS_INTRO    -> win=%s\n", result ? "YES" : "NO");
    TEST_ASSERT_FALSE(result);
}
void test_state_bonus_playing_never_wins(void) {
    bool result = shouldTriggerWin((int)STATE_BONUS_PLAYING, true, true);
    printf("  STATE_BONUS_PLAYING  -> win=%s\n", result ? "YES" : "NO");
    TEST_ASSERT_FALSE(result);
}
void test_state_bonus_simon_never_wins(void) {
    bool result = shouldTriggerWin((int)STATE_BONUS_SIMON, true, true);
    printf("  STATE_BONUS_SIMON    -> win=%s\n", result ? "YES" : "NO");
    TEST_ASSERT_FALSE(result);
}

// ---------------------------------------------------------------------------
// Completely invalid state value must not cause a crash or false win
// ---------------------------------------------------------------------------
void test_invalid_state_never_wins(void) {
    bool r1 = shouldTriggerWin(99, true, true);
    bool r2 = shouldTriggerWin(-1, true, true);
    printf("  state=99 -> win=%s  |  state=-1 -> win=%s  (invalid states, no crash)\n",
           r1 ? "YES" : "NO", r2 ? "YES" : "NO");
    TEST_ASSERT_FALSE(r1);
    TEST_ASSERT_FALSE(r2);
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
