#ifndef NETWORK_H
#define NETWORK_H

void initNetwork();
void startTcpController();

void controllerConnected(void *arg);
void controllerDisconnected(void *arg);
void controllerReconnected(void *arg, sint8 err);
void controllerDataReceived(void *arg, char *pdata, unsigned short len);
void controllerDataSent(void *arg);

#endif /* NETWORK_H */
