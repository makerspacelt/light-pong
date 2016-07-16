#ifndef NETWORK_H
#define NETWORK_H

#define SSID "LIGHT"
#define SSID_LEN 5

#define PASS "ABCDEF12"
#define PASS_LEN 8

#define CMD_PLAYER 0x01
#define CMD_BUTTON 0x02

#ifndef BUTTON
    #define BUTTON 1
#endif

extern bool connected;
extern uint8_t player;
extern uint8_t button;

void initNetwork();
sint8 sendButtonData(uint8_t state);

void CBConnected(void *arg);
void CBDisconnected(void *arg);
void CBReconnect(void *arg, sint8 err);


#endif /* NETWORK_H */
