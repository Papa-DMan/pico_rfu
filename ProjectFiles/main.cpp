#include <FreeRTOS.h>
#include <queue.h>
#include <stdio.h>
#include <task.h>

#include "lwip/ip4_addr.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "piodmx.h"
#include "hardware/timer.h"
#include "pico/util/datetime.h"
#include <time.h>
#include "lwip/tcpip.h"
#include "lwip/api.h"
#include "lwip/opt.h"
#include <utility>
#include <vector>
#include <array>
#include <map>
#include "lwip/apps/fs.h"
#include "lwip/apps/httpd.h"
#include "lwip/apps/mdns.h"
//#include "fsdata.h"
#include "dhcpserver.h"
#include "dnsserver.h"
#include "http_state.h"
#include "core_json.h"
#include <cstring>
#include <set>

#include <pico/rand.h>
#include <mbedtls/rsa.h>
#include <mbedtls/pk.h>
#include <mbedtls/pem.h>
#include "encryption_keys.h"

#define NUMDMX 1
#define SSID            "RemoteFocus"
#define PASSWORD        "12345678"
#define AP_MODE         true

static ip4_addr_t gw, mask;
static dhcp_server_t dhcp;
static dns_server_t dns;
static QueueHandle_t tcpQueue = NULL;
static QueueHandle_t dmxQueue = NULL;
static std::set<uint16_t> captured;
enum HTTPDREQ {
    API,
    APIKEYS,
    APIAUTH
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


void dmx_task(void *pvParameters) {
    xTaskCreate(dmx_loop, "dmx_loop", 2048, NULL, 3, NULL);
    uint8_t zero[512];
    memset(zero, 0, 512);
    xQueueSend(dmxQueue, zero, 0);
    while(1) {
        uint8_t data[512];
        xQueueReceive(dmxQueue, data, portMAX_DELAY);
        while (dmx.busy()) {
            vTaskDelay(1);
        }
        dmx.unsafeWriteBuffer(data);
    }
}

/**
 * @brief Callback function for the httpd server receiving a POST request
 * @post The httpdreq vairable is set to the appropriate value
 * @return ERR_OK if the request is valid, ERR_VAL otherwise
*/
err_t httpd_post_begin(void * connection, const char *uri, const char * http_request, u16_t http_request_len, int content_len, char * response_uri, u16_t response_uri_len, u8_t *post_auto_wnd) {
    if (strncmp(uri, "/api", 4) == 0) {
        if (strncmp(uri, "/api/auth", 9) == 0) {
            httpdreq = HTTPDREQ::APIAUTH;
            return ERR_OK;
        }
        if (strncmp(uri, "/api/keys", 9) == 0) {
            httpdreq = HTTPDREQ::APIKEYS;
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
    
    token = strtok(keys, " ");
    while (token != nullptr) {
        tokens.push_back(token);
        token = strtok(NULL, " ");
    }
    for (const auto& t : tokens) {
        if (strncmp(t, "release", 7) == 0) {
            break;
        }
        else if (strncmp(t, "AND", 3) == 0) {
            continue;
        }
        else if (strncmp(t, "AT", 2) == 0) {
            isLEVEL = true;
            continue;
        }
        else if (strncmp(t, "FULL" , 4) == 0) {
            for (auto d : channels) {
                dmxFrame[d] = 255;
            }
            captured.insert(channels.begin(), channels.end());
            channels.clear();
            isLEVEL = false;
            continue;
        }
        else if (strncmp(t, "THROUGH", 7) == 0) {
            isTHRU = true;
        }
        else {
            if (isLEVEL) {
                uint16_t level = atoi(t);
                for (auto d : channels) {
                    dmxFrame[d] = level;
                }
                captured.insert(channels.begin(), channels.end());
                channels.clear();
                isLEVEL = false;
            } else
            if (isTHRU) {
                uint16_t channel = atoi(t);
                for (int i = channels.back() + 1; i <= channel; i++) {
                    channels.push_back(i);
                }

                isTHRU = false;
            }
            else {
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
    char *value;
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

char * decryptPassword(char* password, size_t password_len) {
    unsigned char result[MBEDTLS_MPI_MAX_SIZE];
    size_t olen;
    size_t decryptedPassword_len;
    mbedtls_pk_init( &pk );
    mbedtls_pem_init(&pem);
    int readPem = mbedtls_pk_parse_key(&pk, private_key, sizeof(private_key), NULL, 0);
    if (readPem != 0) {
        return nullptr;
    }
    int status = mbedtls_pk_decrypt(&pk, (const unsigned char*)password, password_len, result, &olen, sizeof(result),nullptr, nullptr);
    if (status != 0) {
        return nullptr;
    }
    return (char*)result;
}


/**
 * 
 * @brief Parses/verifies the api/auth request and returns the appropriate response code
 * @todo Implement encryption via mbedtls
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
    if (strncmp(value, PASSWORD, value_len) == 0) {
        return 200;
    } 
    return 400;
}


/**
 * @brief Called when data is received from the client
 * @post The response header is written to the connection and the packed buffer is freed
 * @param connection The connection pointer to the client
 * @param p The packed buffer containing the data (NEEDS TO BE FREED)
 * @post The response header is written to the connection and the packed buffer is freed
 * @return ERR_OK
*/
err_t httpd_post_receive_data(void* connection, struct pbuf *p) {
    char* data = (char*)p->payload;
    switch (httpdreq) {
        case HTTPDREQ::APIAUTH: {
            if (parseAPIAuthRequest(data, p->len) == 200) {
                snprintf(((http_state*)connection)->hdrs[0], ((http_state*)connection)->hdr_content_len[0], "HTTP/1.1 200 OK");
            }
            else {
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
    http_state *hs = (http_state*)connection;
    snprintf(hs->hdrs[1], hs->hdr_content_len[1], "Content-type: text/html");
    snprintf(hs->hdrs[2], hs->hdr_content_len[2], "");
    snprintf(response_uri, response_uri_len, "/index.html");
}

void wifi_init_task(void *) {
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_USA)) {                                      //init wifi module with country code
        //printf("CYW43 initalization failed\n");
    }
    cyw43_wifi_pm(&cyw43_state, 0xA11140);                                                      //disable powersave mode
    #if AP_MODE
    cyw43_arch_enable_ap_mode(SSID, PASSWORD, CYW43_AUTH_WPA2_AES_PSK);                         //enable AP mode

    IP_ADDR4(ip_2_ip4(&gw), 192, 168, 4, 1);                                                    //set IP address                           
    IP_ADDR4(ip_2_ip4(&mask), 255, 255, 255, 0);                                                //set netmask       
    netif_set_addr(netif_default, &gw, &mask, &gw);                                             //set netif ip netmask and gateway    
    dhcp_server_init(&dhcp, &gw, &mask);                                                        //start DHCP server
    dns_server_init(&dns, &gw);                                                                 //start DNS server              
    netif_set_hostname(netif_default, "rfunit");                                                //set hostname
    #else
    cyw43_arch_enable_sta_mode();                                                               //enable STA mode
    cyw43_arch_wifi_connect_async(SSID, PASSWORD, CYW43_AUTH_WPA2_AES_PSK);                     //connect to AP
    while(cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) != CYW43_LINK_UP) {              //wait for connection
        vTaskDelay(1000);
    }
    netif_set_hostname(netif_default, "rfunit");                                                //set hostname
    dhcp_start(netif_default);                                                                  //start DHCP client
    while(!dhcp_supplied_address(netif_default)) {                                              //wait for DHCP to finish
        vTaskDelay(1000);
    }
    #endif
    //mdns_resp_init();
    //mdns_resp_add_netif(netif_default, "rfunit");

    dmxQueue = xQueueCreate(5, 512);                                        //create queue for DMX frames
    dmx.begin(5, 6);
    xTaskCreate(dmx_task, "DMX", 1024, NULL, 2, NULL);                      //create task to listen for DMX frames
    //xTaskCreate(httpd_task, "HTTPD", 4096, httpd, 2, NULL);                 //create task to listen for HTTP requests
    httpd_init();

    vTaskDelete(NULL);
}



int main() {
    stdio_init_all();
    timer_hw->dbgpause = 0;
    tcpQueue = xQueueCreate(5, 2048);

    xTaskCreate(wifi_init_task, "wifi_init_task", 1024, NULL, 1, NULL);
    vTaskStartScheduler();
}