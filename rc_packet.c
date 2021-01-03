#include "rc_packet.h"

int rc_packet_fragger_push(rc_packet_fragger_ctx_t self, rc_packet_t* packet)
{
	if (self->packets[packet->index & 0x0F]) 
	{
		//TODO force overwrite flag
		return ESP_BUSY;
	}

	uint16_t len = packet->length;
	while (len)
	{
		uint8_t frag_len = (len / 3) > 
	}

}
