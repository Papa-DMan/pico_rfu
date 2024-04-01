#include <FreeRTOS.h>
//#include <mbedtls/pem.h>
//#include <mbedtls/pk.h>
//#include <mbedtls/rsa.h>
#include <pico/rand.h>
#include <pico/stdlib.h>
#include <queue.h>
#include <stdio.h>
#include <task.h>
#include <time.h>

#include <array>
#include <cstring>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "EEPROM.h"
//#include "core_json.h"
#include "dhcpserver.h"
#include "dnsserver.h"
#include "mongoose.h"
#include "net.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"
#include "piodmx.h"

// default config values
static EEPROMClass eeprom;
struct rfu_config_t {
    char hostname[32] = "rfunit";
    size_t hostname_len = 6;
    char ssid[32] = "RemoteFocus";
    size_t ssid_len = 11;
    char password[64] = "12345678";
    size_t password_len = 8;
    char web_password[64] = "12345678";
    size_t web_password_len = 8;
    bool ap_mode = true;
    bool dmx_loop = true;
    uint8_t checksum = 111;
};
rfu_config_t rfu_config;

uint8_t calcCheckSum(rfu_config_t data) {
    uint8_t checksum = 0;
    for (int i = 0; i < 32; i++) {
        checksum += data.hostname[i];
    }
    for (int i = 0; i < 32; i++) {
        checksum += data.ssid[i];
    }
    for (int i = 0; i < 64; i++) {
        checksum += data.password[i];
    }
    for (int i = 0; i < 64; i++) {
        checksum += data.web_password[i];
    }
    checksum += data.hostname_len;
    checksum += data.ssid_len;
    checksum += data.password_len;
    checksum += data.web_password_len;
    checksum += data.ap_mode;
    checksum += data.dmx_loop;
    return checksum;
}


void loadConfig() {
    rfu_config_t config;
    eeprom.get(0, config);
    if (config.checksum == calcCheckSum(config)) {
        memset(&rfu_config, 0, sizeof(rfu_config_t));
        memcpy(&rfu_config, &config, sizeof(rfu_config_t));
    }
}

static ip4_addr_t gw, mask;
static dhcp_server_t dhcp;
static dns_server_t dns;
static QueueHandle_t tcpQueue = NULL;
static QueueHandle_t dmxQueue = NULL;
static std::set<uint16_t> captured;

static DMX dmx;

void dmx_loop(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1) {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(16));
        if (dmx.busy()) {
            continue;
        }
        dmx.sendDMX();
    }
}

void dmx_task(void* pvParameters) {
    if (rfu_config.dmx_loop)
        xTaskCreate(dmx_loop, "dmx_loop", 2048, NULL, 3, NULL);
    uint8_t zero[512];
    memset(zero, 0, 512);
    xQueueSend(dmxQueue, zero, 0);
    while (1) {
        uint8_t data[512];
        xQueueReceive(dmxQueue, data, portMAX_DELAY);
        while (dmx.busy()) {
            vTaskDelay(1);
        }
        dmx.forceBusy(true);
        dmx.unsafeWriteBuffer(data);
        dmx.forceBusy(false);
        if (!rfu_config.dmx_loop)
            dmx.sendDMX();
    }
}

/**
 * @brief Dechiphers the key string and generates a DMX frame to be sent
 * @param keys The key string buffer to be parsed
 * @param keysLength The length of the key string
 * @post The dmxQueue is populated with the DMX frame to be sent
 */
void processKeys(char* keys, size_t keysLength) {
    char* token;
    std::vector<char*> tokens;
    std::vector<uint16_t> channels;
    bool isLEVEL = false;
    bool isTHRU = false;
    uint8_t dmxFrame[512];
    memset(dmxFrame, 0, 512);
    dmx.getshadowbuff(dmxFrame);

    token = strtok(keys, " ");
    while (token != nullptr) {
        tokens.push_back(token);
        token = strtok(NULL, " ");
    }
    for (const auto& t : tokens) {
        if (strncmp(t, "release", 7) == 0) {
            memset(dmxFrame, 0, 512);
            captured.clear();
            break;
        } else if (strncmp(t, "AND", 3) == 0) {
            continue;
        } else if (strncmp(t, "AT", 2) == 0) {
            isLEVEL = true;
            continue;
        } else if (strncmp(t, "FULL", 4) == 0) {
            for (auto d : channels) {
                dmxFrame[d] = 255;
            }
            captured.insert(channels.begin(), channels.end());
            channels.clear();
            isLEVEL = false;
            continue;
        } else if (strncmp(t, "THRU", 4) == 0) {
            isTHRU = true;
        } else {
            if (isLEVEL) {
                uint16_t level = atoi(t);
                for (auto d : channels) {
                    dmxFrame[d] = level;
                }
                captured.insert(channels.begin(), channels.end());
                channels.clear();
                isLEVEL = false;
            } else if (isTHRU) {
                uint16_t channel = atoi(t);
                if (channel >= 1 && channel <= 512)
                    for (int i = channels.back() + 1; i <= channel; i++) {
                        channels.push_back(i);
                    }

                isTHRU = false;
            } else {
                uint16_t channel = atoi(t);
                channels.push_back(channel);
            }
        }
    }
    xQueueSend(dmxQueue, dmxFrame, portMAX_DELAY);
}

/**
 * @brief Writes the config to the EEPROM must be spawned with configMAX_PRIORITIES - 1 priority
 * @param pvParameters Unused
 * @post The config is written to the EEPROM
 */
void write_config_task(void* pvParameters) {
    #define AIRCR_Register (*((volatile uint32_t*)(PPB_BASE + 0x0ED0C)))
    // enter critical section
    taskENTER_CRITICAL();
    eeprom.put(0, rfu_config);
    eeprom.commit();
    taskEXIT_CRITICAL();
    vTaskDelay(5000);
    AIRCR_Register = 0x5FA0004;
    vTaskDelete(NULL);
}

static struct mg_mgr mgr;

void mongoose_task(void* pvParameters) {
    mg_mgr_init(&mgr);
    web_init(&mgr);
    while(true) {
        mg_mgr_poll(&mgr, 10);
    }
}

void wifi_init_task(void*) {
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_USA)) {                  // init wifi module with country code
        printf("CYW43 initalization failed, Reseting...\n");
        AIRCR_Register = 0x5FA0004;
    }
    cyw43_wifi_pm(&cyw43_state, 0xA11140);                                   // disable powersave mode
    if (rfu_config.ap_mode) {
        cyw43_arch_enable_ap_mode(rfu_config.ssid, rfu_config.password, CYW43_AUTH_WPA2_AES_PSK);  // enable AP mode

        IP_ADDR4(ip_2_ip4(&gw), 192, 168, 4, 1);         // set IP address
        IP_ADDR4(ip_2_ip4(&mask), 255, 255, 255, 0);     // set netmask
        netif_set_addr(netif_default, &gw, &mask, &gw);  // set netif ip netmask and gateway
        dhcp_server_init(&dhcp, &gw, &mask);             // start DHCP server
        dns_server_init(&dns, &gw);                      // start DNS server
        netif_set_hostname(netif_default, "rfunit");     // set hostname
    } else {
        vTaskDelay(1000);
        cyw43_arch_enable_sta_mode();                                                                  // enable STA mode
        cyw43_arch_wifi_connect_async(rfu_config.ssid, rfu_config.password, CYW43_AUTH_WPA2_AES_PSK);  // connect to AP
        printf("Connecting to %s\n", rfu_config.ssid);
        // wait for connection or timeout after 10 seconds
        uint8_t timeout = 0;
        while (cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) != CYW43_LINK_UP && timeout < 30) {
            printf("Status: %d\n", cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA));               // wait for connection
            vTaskDelay(1000);
            timeout++;
        }
        if (timeout >= 30) {
            printf("Connection timed out, falling back to default config\n");
            rfu_config.checksum++;                                                      // iterate checksum to force default config
            xTaskCreate(write_config_task, "write_config_task", 1024, NULL, configMAX_PRIORITIES - 1, NULL);
            vTaskDelete(NULL);
        } else {
            printf("\n");
            printf("IP Address: %s\n", ip4addr_ntoa(&netif_default->ip_addr));          // print IP address
            netif_set_hostname(netif_default, rfu_config.hostname);                     // set hostname
            dhcp_start(netif_default);                                                  // start DHCP client
            printf("dhcp started\n");
            while (!dhcp_supplied_address(netif_default)) {                             // wait for DHCP to finish
                vTaskDelay(1000);
                printf(".");
            }
        }
    }
    printf("IP Address: %s\n", ip4addr_ntoa(&netif_default->ip_addr));                  // print IP address

    dmxQueue = xQueueCreate(5, 512);                                                    // create queue for DMX frames
    dmx.begin(2);                                                                       // init DMX on pin 2

    xTaskCreate(dmx_task, "DMX", 1024, NULL, 2, NULL);                                  // create task to listen for DMX frames
    xTaskCreate(mongoose_task, "mongoose", 2048, NULL, 2, NULL);                         // create task for mongoose

    vTaskDelete(NULL);
}

int main() {
    stdio_init_all();
    timer_hw->dbgpause = 0;
    eeprom.begin(sizeof(rfu_config_t));
    loadConfig();
    tcpQueue = xQueueCreate(5, 2048);

    xTaskCreate(wifi_init_task, "wifi_init_task", 1024, NULL, 1, NULL);
    vTaskStartScheduler();
}