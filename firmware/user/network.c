#include "c_types.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "mem.h"
#include "network.h"

static struct espconn *contServer;

void ICACHE_FLASH_ATTR initNetwork()
{
    struct softap_config sc;
    wifi_softap_get_config(&sc);
    
    os_memset(&sc.ssid, 0, 32);
    os_memset(&sc.password, 0, 64);
    os_memcpy(&sc.ssid, "LightPong", 9);
    os_memcpy(&sc.password, "ABCDEF12", 8);
    
    sc.ssid_len = 9;
    sc.channel = 9;
    sc.authmode = AUTH_WPA2_PSK;
    sc.max_connection = 8;
    
    EnterCritical();
        wifi_set_opmode(SOFTAP_MODE);
        wifi_softap_set_config(&sc);
    ExitCritical();
    
    os_printf(
            "SoftAP mode: \"%s\":\"%s\" @ %d int: %d %d/%d\n",
            sc.ssid,
            sc.password,
            wifi_get_channel(),
            sc.beacon_interval,
            sc.ssid_len,
            wifi_softap_dhcps_status()
    );

    startTcpController();
}

void ICACHE_FLASH_ATTR startTcpController()
{
    // Allocate server
    contServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
    os_memset(contServer, 0, sizeof (struct espconn));
    espconn_create(contServer);

    // Set type TCP
    contServer->type = ESPCONN_TCP;
    contServer->state = ESPCONN_NONE;

    // Set listening port
    contServer->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
    contServer->proto.tcp->local_port = 2048;

    espconn_regist_connectcb(contServer, controllerConnected);

    // Start listening
    sint8 status = espconn_accept(contServer);
    os_printf("Server created status: %d\n", status);

    // Idle time
    espconn_regist_time(contServer, 3600, 0);
}

void ICACHE_FLASH_ATTR controllerConnected(void *arg)
{
    struct espconn *connection = (struct espconn *)arg;
    uint8_t *ip =  connection->proto.tcp->remote_ip;

    os_printf("Connected: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
    
    espconn_regist_disconcb(connection, controllerDisconnected);
    espconn_regist_reconcb(connection, controllerReconnected);
    espconn_regist_recvcb(connection, controllerDataReceived);
    espconn_regist_sentcb(connection, controllerDataSent);
    
    uint8_t data[] = {0x01, 0x02};
    espconn_send(connection, data, 2);
}

void ICACHE_FLASH_ATTR controllerDisconnected(void *arg)
{
    os_printf("Disconnected\n");
}

void ICACHE_FLASH_ATTR controllerReconnected(void *arg, sint8 err)
{
    os_printf("Reconnected\n");
}

void ICACHE_FLASH_ATTR controllerDataReceived(void *arg, char *pdata, unsigned short len)
{
    os_printf("GOT data\n");
}

void ICACHE_FLASH_ATTR controllerDataSent(void *arg)
{
    os_printf("SENT data\n");
}