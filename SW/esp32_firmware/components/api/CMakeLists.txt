idf_component_register(SRCS
"src/lcd.cpp"
"src/esp_encoder.cpp"
"src/esp_stepper.cpp"
"src/esp_buttons.cpp"
"src/wifi.c"
"src/ota.cpp"

INCLUDE_DIRS
"inc"
"../drivers/inc"
"../menu/inc"
"../../include"

# dependancy
PRIV_REQUIRES
	nvs_flash
)
