#ifndef GAME_H
#define GAME_H

#define LEDS 300     // total ammount of leds
#define SAFEZONE 30
#define SPEED_MAX 4    // single minimum frame speed in ms
#define SPEED_START 14
#define SPEED_INC 2
#define TRAIL 2

#define MAX_SCORE 10 // score limit
#define SCORE_LEDS 14 // how much leds to light per 1 point

// Player1 uses GPIO0 from this esp instead of separate esp
//#define EMULATE_P1

// Game modes definition
typedef enum {
    START, 
    MOVING,
    LAST_FRAME,
    SCORE_PHASE1,
    SCORE_PHASE2,
    WIN,
    CLEAR,
    CLEAR2
} game_mode;

// Sound events definition
typedef enum {
    SOUND_DUMMY,
    SOUND_SCORE, 
    SOUND_SCORE_FIRST,
    SOUND_PONG,
    SOUND_VICTORY,
    SOUND_START,
    SOUND_MUSIC
} sound_event;

// Player definition
typedef struct {
    uint8_t nr;
    uint8_t buttons[10];
    uint8_t score;
} Player;

volatile os_timer_t frameTimer;
volatile os_timer_t scoreTimer;
volatile os_timer_t winTimer;

extern game_mode gameMode;
extern Player player1;
extern Player player2;

void prepareGame();
void inputMonitor(os_event_t *events);
void incSpeed();

void scoreTimerCallback(void *arg);
void frameTimerCallback(void *arg);
void winTimerCallback(void *arg);

Player *getPlayer(uint8_t nr);
Player *getPlayerByConnection(struct espconn *connection);

void sendEvent(uint8_t event, uint8_t playerScored);

#endif /* GAME_H */
