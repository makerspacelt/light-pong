#include "c_types.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "mem.h"
#include "network.h"

static struct espconn *contServer;

void ICACHE_FLASH_ATTR initNetwork()
{
    EnterCritical();
    wifi_set_opmode(SOFTAP_MODE);

    struct softap_config sc;
    wifi_softap_get_config(&sc);

    os_printf(
            "SoftAP mode: \"%s\":\"%s\" @ %d %d/%d\n",
            sc.ssid,
            sc.password,
            wifi_get_channel(),
            sc.ssid_len,
            wifi_softap_dhcps_status()
    );
    ExitCritical();

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
    espconn_regist_time(contServer, 5, 0);
}

void ICACHE_FLASH_ATTR controllerConnected(void *arg)
{
    struct espconn *connection = (struct espconn *)arg;
    uint8_t *ip =  connection->proto.tcp->remote_ip;

    os_printf("Connected: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);

    // Keep alive experiment
    //sint8 setop = espconn_set_opt(connection, ESPCONN_KEEPALIVE);
    //uint32_t keeplive = 5;
    //sint8 status = espconn_set_keepalive(connection, ESPCONN_KEEPIDLE, &keeplive);
    //os_printf("KeepAlive status: %d-%d\n", setop, status);
}
