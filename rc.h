#pragma once

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/timers.h>
#include <freertos/task.h>

#include <esp_system.h>
#include <esp_log.h>
#include <esp_event.h>
#include <tcpip_adapter.h>
#include <esp_wifi.h>

#include <esp_now.h>
#define esp_now_peer_exists esp_now_is_peer_exist

#define RC_BROADCAST_ADDRESS { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#define RC_KEY_SIZE ESP_NOW_KEY_LEN

#define RC_PACKET_MTU (CONFIG_LMTZ_RC_PACKET_PAYLOAD_SIZE + sizeof(rc_packet_header_t))
#define RC_PACKET_QUEUE_LEN CONFIG_LMTZ_RC_PACKET_QUEUE_LEN

#define RC_ACCEPT_ANY ((int (*)(const rc_identity_t*)) 0x1)
#define RC_ACCEPT_NONE NULL

enum 
{
	RC_PACKET_TYPE_IDENTITY = 0x00,
	RC_PACKET_TYPE_DATA = 0x01,

	RC_PACKET_TYPE_CUSTOM
};

enum
{
	RC_PACKET_FLAG_NONE = 0x00,
	RC_PACKET_FLAG_BROADCAST = 0x01,
	RC_PACKET_FLAG_READY = 0x02,
};

enum
{
	RC_ERROR  = 0xFF,
	RC_FAIL   = 0xFF,
	RC_OK     = 0x00,
	RC_ACCEPT = 0x00,
	RC_DENY,
};

typedef struct
{
	uint8_t addr[6];
	uint8_t connected;
	char    name[24];
	uint8_t features[8];

} __attribute__((packed)) 
rc_identity_t;

typedef struct _rc_packet_header_t
{
	uint8_t addr_src[6]; // 6
	uint8_t addr_dst[6]; // 12

	uint8_t type;        // 13
	uint8_t length;      // 14
	uint16_t offset;     // 16
}
__attribute__((packed))
rc_packet_header_t;

#define RC_PACKET_MAX_PAYLOAD (RC_PACKET_MTU - sizeof(rc_packet_header_t))

typedef struct
{
	rc_packet_header_t header;
	uint8_t payload[RC_PACKET_MAX_PAYLOAD];
}
__attribute__((packed))
rc_packet_t;

typedef struct
{
	rc_identity_t identity;

	int (*on_accept)(const rc_identity_t* identity);
	int (*on_receive)(const rc_packet_t* packet);
 
	int (*init)();
	int (*deinit)();
	int (*advertise)();
	int (*broadcast)(int type, const void* payload, size_t len);
	int (*unicast)(const uint8_t* addr, int type, const void* payload, size_t len);

	QueueHandle_t queue, send_queue;
	TaskHandle_t task;
} rc_driver_t;

extern rc_driver_t RC;
