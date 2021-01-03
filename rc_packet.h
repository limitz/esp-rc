#pragma once

#define RC_PACKET_ALLOW_FRAGMENTS true
#define RC_PACKET_FRAG_QUEUE_SIZE 64
#define RC_PACKET_FRAG_DATA_LEN 200

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


enum rc_packet_flag_t
{
	RC_PACKET_FLAG_NONE    = 0,
	//RC_PACKET_FLAG_MALLOC  = 1 << 0,
	//RC_PACKET_FLAG_DMA     = 1 << 1,
};

#pragma pack(push, 1)
typedef struct 
{
	uint8_t type;
	uint8_t index;
	uint16_t length;
	uint8_t priority;
	uint8_t retransmit;
	uint8_t flags;

	struct 
	{
		uint8_t index;
		uint8_t total;
	} frag;

	void* data;

} rc_packet_header_t;
#pragma pack(pop)

typedef struct
{
	struct
	{
		struct
		{
			uint32_t queued;
			uint32_t sent;
			uint32_t lost;
		} bytes;

		struct
		{
			uint16_t  queued;
			uint32_t sent;
			uint32_t lost;
		} frags;
	
		struct
		{
			uint16_t queued;
			uint32_t sent;
			uint32_t lost;
		} packets;
	} info;

	QueueHandle_t queue;
	uint64_t ack[16];
	rc_packet_t *packets[16];

} rc_packet_frag_ctx_t;

typedef strct
{
	rc_packet_frag_ctx_t frag;
	rc_packet_frag_ctx_t defrag;
} rc_packet_fragger_t;

