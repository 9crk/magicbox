deps_config := \
	/ihoment/home/zhouhua/ESP8266/ESP8266_RTOS_SDK/components/app_update/Kconfig \
	/ihoment/home/zhouhua/ESP8266/ESP8266_RTOS_SDK/components/aws_iot/Kconfig \
	/ihoment/home/zhouhua/ESP8266/ESP8266_RTOS_SDK/components/esp8266/Kconfig \
	/ihoment/home/zhouhua/ESP8266/ESP8266_RTOS_SDK/components/esp_http_client/Kconfig \
	/ihoment/home/zhouhua/ESP8266/ESP8266_RTOS_SDK/components/esp_http_server/Kconfig \
	/ihoment/home/zhouhua/ESP8266/ESP8266_RTOS_SDK/components/freertos/Kconfig \
	/ihoment/home/zhouhua/ESP8266/ESP8266_RTOS_SDK/components/libsodium/Kconfig \
	/ihoment/home/zhouhua/ESP8266/ESP8266_RTOS_SDK/components/log/Kconfig \
	/ihoment/home/zhouhua/ESP8266/ESP8266_RTOS_SDK/components/lwip/Kconfig \
	/ihoment/home/zhouhua/ESP8266/ESP8266_RTOS_SDK/components/mdns/Kconfig \
	/ihoment/home/zhouhua/ESP8266/ESP8266_RTOS_SDK/components/mqtt/Kconfig \
	/ihoment/home/zhouhua/ESP8266/ESP8266_RTOS_SDK/components/newlib/Kconfig \
	/ihoment/home/zhouhua/ESP8266/ESP8266_RTOS_SDK/components/pthread/Kconfig \
	/ihoment/home/zhouhua/ESP8266/ESP8266_RTOS_SDK/components/ssl/Kconfig \
	/ihoment/home/zhouhua/ESP8266/ESP8266_RTOS_SDK/components/tcpip_adapter/Kconfig \
	/ihoment/home/zhouhua/ESP8266/ESP8266_RTOS_SDK/components/wifi_provisioning/Kconfig \
	/ihoment/home/zhouhua/ESP8266/ESP8266_RTOS_SDK/components/wpa_supplicant/Kconfig \
	/ihoment/home/zhouhua/ESP8266/ESP8266_RTOS_SDK/components/bootloader/Kconfig.projbuild \
	/ihoment/home/zhouhua/ESP8266/ESP8266_RTOS_SDK/components/esptool_py/Kconfig.projbuild \
	/ihoment/home/zhouhua/ESP8266/ESP8266_RTOS_SDK/components/partition_table/Kconfig.projbuild \
	/ihoment/home/zhouhua/ESP8266/ESP8266_RTOS_SDK/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
