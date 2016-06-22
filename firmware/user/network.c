#include "c_types.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "mem.h"
#include "network.h"
#include "game.h"

static struct espconn *contServer;
struct espconn *soundManager;
struct espconn *master;

bool soundSending = false;

/**
 * Convert 32 bit int to array of 4 8 bit ints
 * 
 * @param ip
 * @param out
 */
static void ICACHE_FLASH_ATTR intToIp(uint32_t ip, uint8_t *out)
{
    *out++ = (uint8_t)ip;
    *out++ = (uint8_t)(ip >> 8);
    *out++ = (uint8_t)(ip >> 16);
    *out++ = (uint8_t)(ip >> 24);
}

void ICACHE_FLASH_ATTR masterConnected(void *arg)
{   
    // Keep connection alive (basically reconnect on failure))
    sint8 setop = espconn_set_opt(master, ESPCONN_KEEPALIVE);
    
    // Check every X seconds
    uint32_t keeplive = 1;
    sint8 status = espconn_set_keepalive(master, ESPCONN_KEEPIDLE, &keeplive);
    
    // Repeat check every X seconds
    keeplive = 1;
    sint8 status1 = espconn_set_keepalive(master, ESPCONN_KEEPINTVL, &keeplive);
    
    // Repeat check for X number of times, before calling reconcb
    keeplive = 1;
    sint8 status2 = espconn_set_keepalive(master, ESPCONN_KEEPCNT, &keeplive);
    
    os_printf("Master Connected, KeepAlive status: %d-%d-%d-%d\n", setop, status, status1, status2);
}

void ICACHE_FLASH_ATTR masterDisconnected(void *arg)
{
    os_printf("Master Disconnected\n");
}

void ICACHE_FLASH_ATTR masterReconnect(void *arg, sint8 err)
{
    os_printf("Master Reconnect: %d", err);
}

static void ICACHE_FLASH_ATTR openMaster()
{
    // Allocate connection
    master = (struct espconn *)os_zalloc(sizeof(struct espconn));
    os_memset(master, 0, sizeof (struct espconn));
    espconn_create(master);

    // Set type TCP
    master->type = ESPCONN_TCP;
    master->state = ESPCONN_NONE;

    // Get remote ip (gateway)
    struct ip_info remoteIp;
    wifi_get_ip_info(STATION_IF, &remoteIp);

    uint8_t serverIp[4];
    intToIp(remoteIp.gw.addr, serverIp);

    // Define server
    master->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
    os_memcpy(master->proto.tcp->remote_ip, serverIp, 4);
    master->proto.tcp->remote_port = 2048;

#ifndef NODEBUG
    // Register connected callback
    espconn_regist_connectcb(master, masterConnected);
    espconn_regist_disconcb(master, masterDisconnected);
    espconn_regist_reconcb(master, masterReconnect);
#endif

    // Connect to server
    sint8 status = espconn_connect(master);
    os_printf("Master do open connection: %d\n", status);
}

static void ICACHE_FLASH_ATTR wifiEventCallback(System_Event_t *evt)
{
    if (evt->event == EVENT_STAMODE_GOT_IP) {
        os_printf("WiFi Connected, ip ready\n");
        
        openMaster();
    }
}

void ICACHE_FLASH_ATTR initNetwork()
{
    struct station_config stationConf; 
    wifi_station_get_config(&stationConf);

    // Don't check MAC of AP
    stationConf.bssid_set = 0;

    // Well that's was a headache
    os_memset(&stationConf.ssid, 0, 32);
    os_memset(&stationConf.password, 0, 64);

    // Set SSID and pass, seems doesn't connect to open networks
    os_memcpy(&stationConf.ssid, "LIGHT", 5);
    os_memcpy(&stationConf.password, "ABCDEF12", 8);

    struct ip_info ipinfo;
    IP4_ADDR(&ipinfo.ip, 192, 168, 4, 2);
    IP4_ADDR(&ipinfo.gw, 192, 168, 4, 1);
    IP4_ADDR(&ipinfo.netmask, 255, 255, 255, 0);

    // Init WIFI client
    EnterCritical();
        wifi_set_opmode(STATION_MODE);
        wifi_station_dhcpc_stop();
        wifi_set_ip_info (0x00, &ipinfo);
        wifi_station_set_config(&stationConf);
    ExitCritical();

    os_timer_disarm(&soundTimer);
    os_timer_setfn(&soundTimer, (os_timer_func_t *)soundTimerCallback, NULL);

    wifi_set_event_handler_cb(wifiEventCallback);
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
            player = getPlayer(msg);

            player->assigned = 1;
            player->button = 1;
            player->connection = connection;

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