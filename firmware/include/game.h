#ifndef GAME_H
#define GAME_H

#define LEDS 20     // total ammount of leds
#define SAFEZONE 4
#define SPEED 30    // single frame speed in ms
#define TRAIL 1

#define MAX_SCORE 9 // score limit
#define SCORE_LEDS 1 // how much leds to light per 1 point

// Player1 uses GPIO0 from this esp instead of separate esp
#define EMULATE_P1

// Game modes definition
typedef enum {
    START, 
    MOVING,
    LAST_FRAME,
    SCORE_PHASE1,
    SCORE_PHASE2,
    WIN
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
    uint8_t button;
    struct espconn *connection;
    BOOL assigned;
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

void scoreTimerCallback(void *arg);
void frameTimerCallback(void *arg);
void winTimerCallback(void *arg);

Player *getPlayer(uint8_t nr);
Player *getPlayerByConnection(struct espconn *connection);

void callSound(uint8_t event);

#endif /* GAME_H */
