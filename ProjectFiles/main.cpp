#include <FreeRTOS.h>
#include <mbedtls/pem.h>
#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>
#include <pico/rand.h>
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
#include "core_json.h"
#include "dhcpserver.h"
#include "dnsserver.h"
#include "hardware/timer.h"
#include "hardware/watchdog.h"
#include "lwip/api.h"
#include "lwip/apps/fs.h"
#include "lwip/apps/httpd.h"
#include "lwip/apps/mdns.h"
#include "lwip/ip4_addr.h"
#include "lwip/opt.h"
#include "lwip/tcpip.h"
#include "http_state.h"
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
enum HTTPDREQ {
    API,
    APIKEYS,
    APIAUTH,
    APICONF
};

static mbedtls_pem_context pem;
static mbedtls_pk_context pk;

static HTTPDREQ httpdreq = API;

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
 * @brief Callback function for the httpd server receiving a POST request
 * @post The httpdreq vairable is set to the appropriate value
 * @return ERR_OK if the request is valid, ERR_VAL otherwise
 */
err_t httpd_post_begin(void* connection, const char* uri, const char* http_request, u16_t http_request_len, int content_len, char* response_uri, u16_t response_uri_len, u8_t* post_auto_wnd) {
    if (strncmp(uri, "/api", 4) == 0) {
        if (strncmp(uri, "/api/auth", 9) == 0) {
            httpdreq = HTTPDREQ::APIAUTH;
            return ERR_OK;
        }
        if (strncmp(uri, "/api/keys", 9) == 0) {
            httpdreq = HTTPDREQ::APIKEYS;
            return ERR_OK;
        }
        if (strncmp(uri, "/api/conf", 9) == 0) {
            httpdreq = HTTPDREQ::APICONF;
            return ERR_OK;
        }
        httpdreq = API;
        return ERR_OK;
    }
    return ERR_VAL;
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
 * @brief Parses/verifies the api/keys request and calls processKeys
 * @param json The json buffer to be parsed
 * @param json_len The length of the json buffer
 * @post The dmxQueue is populated with the DMX frame to be sent
 */
void parseAPIKeysRequest(char* json, int json_len) {
    char* value;
    size_t value_len;
    JSONStatus_t result;
    result = JSON_Validate(json, json_len);
    if (result != JSONSuccess)
        return;
    result = JSON_Search(json, json_len, "keys", 4, &value, &value_len);
    if (result != JSONSuccess)
        return;
    processKeys(value, value_len);
}

/**
 * @brief Decrypts the password sent by the client
 * @param password The password buffer to be decrypted
 * @param password_len The length of the password buffer
 * @post the result buffer is populated with the decrypted password and needs to be freed by the caller
 * @return The decrypted password if the password is valid, nullptr otherwise
 *
 */
char* decryptPassword(char* password, size_t password_len) {
    return password;
}

/**
 *
 * @brief Parses/verifies the api/auth request and returns the appropriate response code
 * @param json The json buffer to be parsed
 * @param json_len The length of the json buffer
 * @return 200 if the password is correct, 400 otherwise
 *
 */
uint16_t parseAPIAuthRequest(char* json, int json_len) {
    char* value;
    size_t value_len;
    JSONStatus_t result;
    result = JSON_Validate(json, json_len);
    if (result != JSONSuccess)
        return 400;
    result = JSON_Search(json, json_len, "password", 8, &value, &value_len);
    if (result != JSONSuccess)
        return 400;
    char* passwd = decryptPassword(value, value_len);
    if (strncmp(passwd, rfu_config.web_password, rfu_config.web_password_len) == 0) {
        return 200;
    }
    return 400;
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

uint16_t parseAPIConfRequest(char* json, int json_len) {
    char* value;
    size_t value_len;
    JSONStatus_t result;
    rfu_config_t config;
    memset(&config, 0, sizeof(rfu_config_t));
    result = JSON_Validate(json, json_len);
    if (result != JSONSuccess)
        return 400;
    result = JSON_Search(json, json_len, "hostname", 8, &value, &value_len);
    if (result != JSONSuccess)
        return 400;
    strncpy(config.hostname, value, value_len);
    config.hostname_len = value_len;
    result = JSON_Search(json, json_len, "ssid", 4, &value, &value_len);
    if (result != JSONSuccess)
        return 400;
    strncpy(config.ssid, value, value_len);
    config.ssid_len = value_len;
    result = JSON_Search(json, json_len, "password", 8, &value, &value_len);
    if (result != JSONSuccess)
        return 400;
    strncpy(config.password, value, value_len);
    config.password_len = value_len;
    result = JSON_Search(json, json_len, "web_password", 12, &value, &value_len);
    if (result != JSONSuccess)
        return 400;
    strncpy(config.web_password, value, value_len);
    config.web_password_len = value_len;
    result = JSON_Search(json, json_len, "ap_mode", 7, &value, &value_len);
    if (result != JSONSuccess)
        return 400;
    (strncmp(value, "true", value_len) == 0) ? config.ap_mode = true : config.ap_mode = false;
    result = JSON_Search(json, json_len, "dmx_loop", 8, &value, &value_len);
    if (result != JSONSuccess)
        return 400;
    (strncmp(value, "true", value_len) == 0) ? config.dmx_loop = true : config.dmx_loop = false;
    config.checksum = calcCheckSum(config);
    memset(&rfu_config, 0, sizeof(rfu_config_t));
    memcpy(&rfu_config, &config, sizeof(rfu_config_t));
    xTaskCreate(write_config_task, "write_config_task", 1024, NULL, configMAX_PRIORITIES - 1, NULL);
    return 200;
}

/**
 * @brief Called when data is received from the client
 * @post The response header is written to the connection and the packed buffer is freed
 * @param connection The connection pointer to the client
 * @param p The packed buffer containing the data (NEEDS TO BE FREED)
 * @post The response header is written to the connection and the packed buffer is freed
 * @return ERR_OK
 */
err_t httpd_post_receive_data(void* connection, struct pbuf* p) {
    char* data = (char*)p->payload;
    switch (httpdreq) {
        case HTTPDREQ::APIAUTH: {
            if (parseAPIAuthRequest(data, p->len) == 200) {
                snprintf(((http_state*)connection)->hdrs[0], ((http_state*)connection)->hdr_content_len[0], "HTTP/1.1 200 OK");
            } else {
                snprintf(((http_state*)connection)->hdrs[0], ((http_state*)connection)->hdr_content_len[0], "HTTP/1.1 400 Bad Request");
            }
            break;
        }
        case HTTPDREQ::APIKEYS: {
            parseAPIKeysRequest(data, p->len);
            snprintf(((http_state*)connection)->hdrs[0], ((http_state*)connection)->hdr_content_len[0], "HTTP/1.1 200 OK");
            break;
        }
        case HTTPDREQ::API: {
            snprintf(((http_state*)connection)->hdrs[0], ((http_state*)connection)->hdr_content_len[0], "HTTP/1.1 200 OK");
            break;
        }
        case HTTPDREQ::APICONF: {
            if (parseAPIConfRequest(data, p->len) == 200) {
                snprintf(((http_state*)connection)->hdrs[0], ((http_state*)connection)->hdr_content_len[0], "HTTP/1.1 200 OK");
            } else {
                snprintf(((http_state*)connection)->hdrs[0], ((http_state*)connection)->hdr_content_len[0], "HTTP/1.1 400 Bad Request");
            }
            break;
        }
    }
    pbuf_free(p);
    return ERR_OK;
}

/**
 * @brief Called when the client has finished sending data
 * @post The response header is written to the connection and the connection is closed
 * @param connection The connection pointer to the client
 * @param response_uri The buffer to write the response uri to
 * @param response_uri_len The length of the response uri buffer
 *
 */
void httpd_post_finished(void* connection, char* response_uri, u16_t response_uri_len) {
    http_state* hs = (http_state*)connection;
    snprintf(hs->hdrs[1], hs->hdr_content_len[1], "Content-type: text/html");
    snprintf(hs->hdrs[2], hs->hdr_content_len[2], "");
    snprintf(response_uri, response_uri_len, "/index.html");
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
    httpd_init();

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