#ifndef GAME_H
#define GAME_H

#define LEDS 20     // total ammount of leds
#define SAFEZONE 5
#define SPEED 50    // single frame speed in ms

volatile os_timer_t frameTimer;
volatile os_timer_t scoreTimer;

void prepareGame();
void inputMonitor(os_event_t *events);

void scoreTimerCallback(void *arg);
void frameTimerCallback(void *arg);

#endif /* GAME_H */
