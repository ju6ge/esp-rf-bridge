set(COMPONENT_SRCS 
	state.c 
	rf_433mhz.c
	rf-bridge.c 
	decode.c
)
set(COMPONENT_ADD_INCLUDEDIRS "" )

register_component()

spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)
