#include "c_types.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "mem.h"
#include "network.h"
#include "queue.h"

static os_timer_t ipTimer;
struct espconn *conn;
bool connected = 0;

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

static void ICACHE_FLASH_ATTR initConnection()
{
    // Allocate connection
    struct espconn *connection = (struct espconn *)os_zalloc(sizeof(struct espconn));
    os_memset(connection, 0, sizeof (struct espconn));
    espconn_create(connection);

    // Set type TCP
    connection->type = ESPCONN_TCP;
    connection->state = ESPCONN_NONE;

    // Get remote ip (gateway)
    struct ip_info remoteIp;
    wifi_get_ip_info(STATION_IF, &remoteIp);
    
    uint8_t serverIp[4];
    intToIp(remoteIp.gw.addr, serverIp);
    
    os_printf(
            "ServerIP: %d.%d.%d.%d\n",
            serverIp[0],
            serverIp[1],
            serverIp[2],
            serverIp[3]
    );
    
    // Define server
    connection->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
    os_memcpy(connection->proto.tcp->remote_ip, serverIp, 4);
    connection->proto.tcp->remote_port = 2048;
    
    // Register connected callback and error handler
    espconn_regist_connectcb(connection, CBConnected);
    espconn_regist_reconcb(connection, CBReconnect);

    // Connect to server
    espconn_connect(connection);
}

static void ICACHE_FLASH_ATTR checkConnection()
{
    struct ip_info ipconfig;
    os_timer_disarm(&ipTimer);
    
    wifi_get_ip_info(STATION_IF, &ipconfig);
    if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) {
        uint8_t ip[4];
        intToIp(ipconfig.ip.addr, ip);
        
        os_printf("Got IP: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
        
        // Connect controller port
        initConnection();
    } else {
        // Rearm connection check timer
        os_timer_arm(&ipTimer, 100, 0);
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
    os_memcpy(&stationConf.ssid, SSID, SSID_LEN);
    os_memcpy(&stationConf.password, PASS, PASS_LEN);
    
    struct ip_info ipinfo;
    IP4_ADDR(&ipinfo.ip, 192, 168, 4, 20 + DISPLAY);
    IP4_ADDR(&ipinfo.gw, 192, 168, 4, 2);
    IP4_ADDR(&ipinfo.netmask, 255, 255, 255, 0);

    
    // Init WIFI client
    EnterCritical();
        wifi_set_opmode(STATION_MODE);
        wifi_station_dhcpc_stop();
        wifi_set_ip_info (0x00, &ipinfo);
        wifi_station_set_config(&stationConf);
    ExitCritical();
   
    // Register connection check timer
    os_timer_disarm(&ipTimer);
    os_timer_setfn(&ipTimer, (os_timer_func_t *)checkConnection, NULL);
    os_timer_arm(&ipTimer, 100, 0);   
}


void ICACHE_FLASH_ATTR CBConnected(void *arg)
{
    conn = (struct espconn *)arg;
    
    espconn_regist_disconcb(conn, CBDisconnected);
    espconn_regist_recvcb(conn, CBDataReceived);
    
    // Keep connection alive (basically reconnect on failure))
    sint8 setop = espconn_set_opt(conn, ESPCONN_KEEPALIVE);
    
    // Check every X seconds
    uint32_t keeplive = 1;
    sint8 status = espconn_set_keepalive(conn, ESPCONN_KEEPIDLE, &keeplive);
    
    // Repeat check every X seconds
    keeplive = 1;
    sint8 status1 = espconn_set_keepalive(conn, ESPCONN_KEEPINTVL, &keeplive);
    
    // Repeat check for X number of times, before calling reconcb
    keeplive = 1;
    sint8 status2 = espconn_set_keepalive(conn, ESPCONN_KEEPCNT, &keeplive);
    
    os_printf("Connected, KeepAlive status: %d-%d-%d-%d\n", setop, status, status1, status2);
    
    connected = true;
    system_os_post(0, 0, 0);
}

void ICACHE_FLASH_ATTR CBDisconnected(void *arg)
{
    os_printf("Disconnected\n");
    connected = false;
}

void ICACHE_FLASH_ATTR CBReconnect(void *arg, sint8 err)
{
    os_printf("Reconnected: %d \n", err);
    connected = false;
    
    os_timer_arm(&ipTimer, 100, 0); 
}

void ICACHE_FLASH_ATTR CBDataReceived(void *arg, char *pdata, unsigned short len)
{
//    os_printf("DATA (%d): ", len);
//    uint8_t i;
//    for (i = 0; i < len; i++) {
//        os_printf("0x%02x ", pdata[i]);
//    }
//    os_printf("\n");
    
	int _score = 0;
    switch(*pdata++) {
        case CMD_SCORE:
            //skip a byte
            *pdata++;
            
            ScoreReceived(*pdata++, *pdata++, *pdata++);
            system_os_post(0, 0, 0);
            break;
    }
}

void EnterCritical()
{
}

void ExitCritical()
{
}