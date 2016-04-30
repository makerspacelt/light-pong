#ifndef NETWORK_H
#define NETWORK_H

#define CMD_PLAYER 0x01
#define CMD_BUTTON 0x02

void initNetwork();
void startTcpController();

void controllerConnected(void *arg);
void controllerDisconnected(void *arg);
void controllerReconnected(void *arg, sint8 err);
void controllerDataReceived(void *arg, char *pdata, unsigned short len);
void controllerDataSent(void *arg);

#endif /* NETWORK_H */
