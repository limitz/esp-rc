idf_component_register(SRCS 
	"rc.c"
	"lipo.c"
	"adc.c"
	"servo.c"
	INCLUDE_DIRS .
	REQUIRES "lwip" "esp_adc_cal" "wpa_supplicant" "esp_wifi"
	)


