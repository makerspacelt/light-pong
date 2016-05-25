#include "c_types.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "mem.h"
#include "network.h"
#include "game.h"

static struct espconn *contServer;
struct espconn *soundManager;
bool soundSending = false;
uint8_t lastConnected = 0;

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
    
    os_timer_disarm(&soundTimer);
    os_timer_setfn(&soundTimer, (os_timer_func_t *)soundTimerCallback, NULL);

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
    sint8 conn_stat = espconn_tcp_set_max_con_allow(contServer, 5);
    
    os_printf("Server created status: %d connections: %d\n", status, conn_stat);

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
}

void ICACHE_FLASH_ATTR controllerDisconnected(void *arg)
{
    os_timer_disarm(&soundTimer);
    os_printf("Disconnected\n");
}

void ICACHE_FLASH_ATTR controllerReconnected(void *arg, sint8 err)
{
    os_printf("Reconnected\n");
}

void ICACHE_FLASH_ATTR controllerDataReceived(void *arg, char *pdata, unsigned short len)
{
    struct espconn *connection = (struct espconn *)arg;
    char response[] = {0, 0, 0};
    char cmd = *pdata++;
    char msg = *pdata;
    Player *player;
    
    os_printf("GOT DATA: 0x%02x 0x%02x\n", cmd, msg);
    
    switch (cmd) {
        case CMD_PLAYER:           
            player = getPlayer(msg, lastConnected);

            player->assigned = 1;
            player->button = 1;
            player->connection = connection;
            
            lastConnected = player->nr;

            if (msg == 0) {
                response[0] = CMD_PLAYER;
                response[1] = player->nr;
            }
            
            break;
        case CMD_BUTTON:
            player = getPlayerByConnection(connection);
            player->button = msg;
            
            os_printf("PLAYER BUTTON: %d, 0x%02x\n", player->nr, msg);
            break;
        case CMD_SOUND:
            os_printf("Sound manager connected\n");
            soundManager = connection;
            soundSending = false;
            
            os_timer_arm(&soundTimer, 200, 1);
            break;       
    }
    
    if (response[0]) {
        os_printf("RESPONDING: 0x%02x 0x%02x 0x%02x\n", response[0], response[1], response[2]);
        espconn_send(connection, response, 3);
    }
}

void ICACHE_FLASH_ATTR controllerDataSent(void *arg)
{
    struct espconn *connection = (struct espconn *)arg;
    
    if (connection == soundManager) {
        soundSending = false;
        os_printf("S PING\n");
    }
}

void ICACHE_FLASH_ATTR soundTimerCallback(void *arg)
{
    if (soundSending) {
        return;
    }
    uint8_t data[1]= {0};
    sint8 status = espconn_send(soundManager, data, 1);
    if (status == 0) {
        soundSending = true;
    }
}