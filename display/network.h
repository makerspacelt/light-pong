#ifndef NETWORK_H
#define NETWORK_H

#define SSID "LIGHT"
#define SSID_LEN 5

#define PASS "ABCDEF12"
#define PASS_LEN 8

#define CMD_PLAYER 0x01
#define CMD_BUTTON 0x02
#define CMD_SCORE 0x03

extern bool connected;

void initNetwork();

void CBConnected(void *arg);
void CBDisconnected(void *arg);
void CBReconnect(void *arg, sint8 err);
void CBDataReceived(void *arg, char *pdata, unsigned short len);

void EnterCritical();
void ExitCritical();
void ScoreReceived(int p1, int p2, int max);

#endif /* NETWORK_H */
