#include "c_types.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "mem.h"
#include "network.h"
#include "game.h"

struct espconn *master;

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
    
    espconn_regist_recvcb(master, controllerDataReceived);
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
    IP4_ADDR(&ipinfo.ip, 192, 168, 4, 3);
    IP4_ADDR(&ipinfo.gw, 192, 168, 4, 2);
    IP4_ADDR(&ipinfo.netmask, 255, 255, 255, 0);

    // Init WIFI client
    EnterCritical();
        wifi_set_opmode(STATION_MODE);
        wifi_station_dhcpc_stop();
        wifi_set_ip_info (0x00, &ipinfo);
        wifi_station_set_config(&stationConf);
    ExitCritical();
    
    wifi_set_event_handler_cb(wifiEventCallback);
}

void ICACHE_FLASH_ATTR controllerDataReceived(void *arg, char *pdata, unsigned short len)
{
    struct espconn *connection = (struct espconn *)arg;

    char cmd = *pdata++;
    char msg = *pdata;
    Player *player;
    
    os_printf("GOT DATA cmd: 0x%02x data: ", cmd);
    uint8_t i;
    for (i = 0; i < len - 1; i++) {
        os_printf("0x%02x ", pdata[i]);
    }
    os_printf("\n");

    switch (cmd) {
        case CMD_BUTTON:
            player = getPlayer(msg);
            *pdata++;
            
            // skip button number for now
            uint8_t nr = *pdata++;
            player->buttons[nr] = *pdata++;
            
            os_printf("PLAYER BUTTON: %d-%d, 0x%02x\n", player->nr,  nr, player->buttons[nr]);
            break;     
    }
}
