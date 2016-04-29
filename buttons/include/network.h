#ifndef NETWORK_H
#define NETWORK_H

#define SSID "LightPong"
#define SSID_LEN 9

#define PASS "ABCDEF12"
#define PASS_LEN 8


void initNetwork();

void CBConnected(void *arg);
void CBDisconnected(void *arg);
void CBReconnect(void *arg, sint8 err);
void CBDataReceived(void *arg, char *pdata, unsigned short len);
void CBDataSent(void *arg);


#endif /* NETWORK_H */
