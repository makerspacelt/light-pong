#ifndef NETWORK_H
#define NETWORK_H

#define SSID "LightPong"
#define SSID_LEN 9

#define PASS "ABCDEF12"
#define PASS_LEN 8

#define CMD_PLAYER 0x01
#define CMD_BUTTON 0x02


void initNetwork();
void sendButtonData(uint8_t state);

void CBConnected(void *arg);
void CBDisconnected(void *arg);
void CBReconnect(void *arg, sint8 err);
void CBDataReceived(void *arg, char *pdata, unsigned short len);


#endif /* NETWORK_H */
