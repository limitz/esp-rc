#include <rc.h>
#include <adc.h>
#include <nvs_flash.h>


static joystick_t j = JOYSTICK;
static rc_identity_t s_peer = {0};

typedef struct { char string[32]; } pub_payload_t;
typedef struct { int16_t x; int16_t y; } pri_payload_t;

enum 
{
	PACKET_TYPE_PUBLIC = RC_PACKET_TYPE_CUSTOM + 1,
	PACKET_TYPE_PRIVATE,
};

int on_accept(const rc_identity_t* peer)
{
	ESP_LOGW(__func__, "Accepting peer %s (%02x:%02x:%02x:%02x:%02x:%02x)", 
			peer->name,
			peer->addr[0], peer->addr[1], peer->addr[2], 
			peer->addr[3], peer->addr[4], peer->addr[5]);
	
	memcpy(&s_peer, peer, sizeof(rc_identity_t));
	s_peer.connected = 1;

	RC.advertise();

	return RC_ACCEPT;
}

int on_receive(const rc_packet_t* packet)
{
	const uint8_t* addr = packet->header.addr_src;

	ESP_LOGW(__func__, "MESSAGE [TYPE:%d OFFSET:%d LEN:%d] FROM %02x:%02x:%02x:%02x:%02x:%02x",
		packet->header.type, packet->header.offset, packet->header.length,
		addr[0], addr[1], addr[2], 
		addr[3], addr[4], addr[5]);
	
	ESP_LOG_BUFFER_HEX(__func__, packet->payload, packet->header.length);

	switch(packet->header.type)
	{
		case PACKET_TYPE_PUBLIC:
			ESP_LOGW(__func__, "Public packet says: %s", 
					((pub_payload_t*)packet->payload)->string);
			break;

		case PACKET_TYPE_PRIVATE:
			break;
	
		default:
			ESP_LOGE(__func__, "Unknown packet type %d", 
					packet->header.type);
			break;
	}
	ESP_LOGI(__func__, "---");
	
	return ESP_OK;
}

void app_main()
{
	// NVS NEEDS TO BE INITIALIZED BEFORE INITIALIZING RC
	ESP_ERROR_CHECK(nvs_flash_init());

	strcpy(RC.identity.name, "LMTZ");
	RC.identity.features[0] = 13;
	RC.identity.features[1] = 66;

	RC.on_accept = on_accept;
	RC.on_receive = on_receive;
	
	RC.init();

	adc_init(&j.x);
	adc_init(&j.y);

	for (;;)
	{
		adc_update(&j.x);
		adc_update(&j.y);

		

		if (s_peer.connected)
		{
			pri_payload_t p = { 
				.x = j.y.result, 
				.y = j.x.result, 
			};
			RC.unicast(s_peer.addr, PACKET_TYPE_PRIVATE, &p, sizeof(p));
		}
		else
		{
			RC.advertise();
			vTaskDelay(200);
			pub_payload_t p = { .string = "Hello?" };
			RC.broadcast(PACKET_TYPE_PUBLIC, &p, sizeof(p));
		}
		vTaskDelay(5);
	}

	RC.deinit();
}
