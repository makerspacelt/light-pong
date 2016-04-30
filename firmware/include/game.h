#ifndef GAME_H
#define GAME_H

#define LEDS 300     // total ammount of leds
#define SAFEZONE 40
#define SPEED 4    // single frame speed in ms

struct Player {
    uint8_t button;
    struct espconn *connection;
};

volatile os_timer_t frameTimer;
volatile os_timer_t scoreTimer;

extern uint8_t player2Button;
extern struct Player player1;
extern struct Player player2;

void prepareGame();
void inputMonitor(os_event_t *events);

void scoreTimerCallback(void *arg);
void frameTimerCallback(void *arg);

#endif /* GAME_H */
