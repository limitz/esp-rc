#include <rc.h>
#define TAG "RC"
#define MACADDR_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MACADDR_ARG(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4],(a)[5] 

static void rc_send_cb(const uint8_t* addr, esp_now_send_status_t status)
{
	xQueueSend(RC.send_queue, &status, 1);
}

static void rc_recv_cb(const uint8_t* addr, const uint8_t* data, int len)
{
	if (len < sizeof(rc_packet_header_t)) 
	{
		ESP_LOGE(__func__, "Unsufficient data");
		return;
	}
	if (len > RC_PACKET_MTU) 
	{
		ESP_LOGE(__func__, "Too much data");
		return;
	}

	rc_packet_t* packet = (rc_packet_t*) data;
	//TODO check validity of packet
	xQueueSend(RC.queue, packet, 1);
}

static void rc_task(void* param)
{
	rc_packet_t packet;
	
	while (pdTRUE == xQueueReceive(RC.queue, &packet, portMAX_DELAY))
	{
		switch (packet.header.type)
		{
			case RC_PACKET_TYPE_IDENTITY:
				if ( !esp_now_peer_exists(packet.header.addr_src) 
					&& ( RC_ACCEPT_ANY == RC.on_accept 
						|| ( RC.on_accept
							&& RC_ACCEPT == RC.on_accept(
								(rc_identity_t*) packet.payload)) 
								//TODO EXPECTS NO SPOOFING, fine for now
				)){
					esp_now_peer_info_t peer = {
						.channel = CONFIG_LMTZ_RC_CHANNEL,
						.ifidx = ESPNOW_WIFI_IF,
						.encrypt = true,
					};
					memcpy(peer.lmk, CONFIG_LMTZ_RC_LMK, RC_KEY_SIZE);
					memcpy(peer.peer_addr, packet.header.addr_src, 6);
					ESP_ERROR_CHECK(esp_now_add_peer(&peer));
				}
				break;

			default:
				if (RC.on_receive) 
					RC.on_receive(&packet);
				break;
		}
	}
	ESP_LOGE(__func__, "STOPPED");
}

int rc_init()
{
	//esp_netif_init();
	tcpip_adapter_init();

	wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&config) );
	ESP_ERROR_CHECK( esp_wifi_set_ps(WIFI_PS_NONE));

	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	ESP_ERROR_CHECK( esp_wifi_set_mode(ESPNOW_WIFI_MODE) );
	ESP_ERROR_CHECK( esp_wifi_start());

#if CONFIG_LMTZ_RC_LONGRANGE	
	ESP_ERROR_CHECK( 
			esp_wifi_set_protocol(
				ESPNOW_WIFI_IF, 
				  WIFI_PROTOCOL_11B
				| WIFI_PROTOCOL_11G
				| WIFI_PROTOCOL_11N
				| WIFI_PROTOCOL_LR) 
		);
	
#endif

	esp_efuse_mac_get_default(RC.identity.addr);

	if (0 == RC.queue)
	{
		RC.queue = xQueueCreate(RC_PACKET_QUEUE_LEN,  RC_PACKET_MTU);
		assert(RC.queue);
	}

	if (0 == RC.send_queue)
	{
		RC.send_queue = xQueueCreate(32, sizeof(esp_now_send_status_t));
		assert(RC.send_queue);
	}

	ESP_ERROR_CHECK( esp_now_init());
	ESP_ERROR_CHECK( esp_now_register_recv_cb(rc_recv_cb) );
	ESP_ERROR_CHECK( esp_now_register_send_cb(rc_send_cb) );
	ESP_ERROR_CHECK( esp_now_set_pmk((const uint8_t*)CONFIG_LMTZ_RC_PMK) );
	
	esp_now_peer_info_t broadcast = {
		.ifidx = ESPNOW_WIFI_IF,
		.channel = CONFIG_LMTZ_RC_CHANNEL,
		.encrypt = false,
		.peer_addr = RC_BROADCAST_ADDRESS,
	};
	ESP_ERROR_CHECK( esp_now_add_peer(&broadcast) );

	xTaskCreate(rc_task, "radio task", 4096, NULL, 4, &RC.task);
	return ESP_OK;
}

int rc_send_packet(const uint8_t* addr_dst, const rc_packet_t* packet)
{
	esp_now_send_status_t status;
	do 
	{
		esp_now_send(addr_dst, (const uint8_t*) packet, packet->header.length + sizeof(rc_packet_header_t));
	}
	while (pdTRUE == xQueueReceive(RC.send_queue, &status, 60000) && status);

	return status;
}


int rc_unicast(const uint8_t* addr_dst, int type, const void* payload, size_t len)
{
	int err;
	rc_packet_t packet;
	memcpy(packet.header.addr_dst, addr_dst, 6);
	memcpy(packet.header.addr_src, RC.identity.addr, 6);
	packet.header.type = type;
	
	for (int offset=0; offset < len; offset+= RC_PACKET_MAX_PAYLOAD)
	{
		int bytes = len - offset;
		if (bytes > RC_PACKET_MAX_PAYLOAD) bytes = RC_PACKET_MAX_PAYLOAD;
		packet.header.length =  bytes;
		packet.header.offset = offset;
		memcpy(packet.payload, ((const uint8_t*)payload)+offset, packet.header.length);
		
		err = rc_send_packet(addr_dst, &packet);
		if (err) return err;
	}
	return ESP_OK;
}

int rc_broadcast(int type, const void* payload, size_t len)
{
	uint8_t broadcast[6]  = RC_BROADCAST_ADDRESS;
	return rc_unicast(broadcast, type, payload, len);
}

int rc_advertise()
{
	return rc_broadcast(RC_PACKET_TYPE_IDENTITY, &RC.identity, sizeof(rc_identity_t));
}

int rc_deinit(rc_driver_t* self)
{
	vTaskDelete(RC.task);
	
	vSemaphoreDelete(RC.queue);
	RC.queue = 0;

	vSemaphoreDelete(RC.send_queue);
	RC.send_queue = 0;
	
	esp_now_deinit();
	return ESP_OK;
}


rc_driver_t RC =  
{
	.identity = {
		.name = { 0 },
		.addr = { 0 },
		.features = { 0 },
		.connected = 0
	},
	.on_accept = RC_ACCEPT_ANY,

	.init = rc_init,
	.deinit = rc_deinit,
	.unicast = rc_unicast,
	.broadcast = rc_broadcast,
	.advertise = rc_advertise,
};

