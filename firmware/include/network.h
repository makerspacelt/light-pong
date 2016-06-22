#ifndef NETWORK_H
#define NETWORK_H

#define CMD_PLAYER 0x01
#define CMD_BUTTON 0x02
#define CMD_SOUND  0x03
#define CMD_SCORE  0x04

extern struct espconn *soundManager;
extern struct espconn *master;

extern bool soundSending;
volatile os_timer_t soundTimer;

void initNetwork();
void startTcpController();

void controllerConnected(void *arg);
void controllerDisconnected(void *arg);
void controllerReconnected(void *arg, sint8 err);
void controllerDataReceived(void *arg, char *pdata, unsigned short len);
void controllerDataSent(void *arg);
void soundTimerCallback(void *arg);

void masterConnected(void *arg);
void masterDisconnected(void *arg);
void masterReconnect(void *arg, sint8 err);

#endif /* NETWORK_H */
