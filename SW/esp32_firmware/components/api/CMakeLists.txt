idf_component_register(SRCS
	"src/lcd.cpp"

INCLUDE_DIRS
	"inc"
	"../../include"

# dependancy
PRIV_REQUIRES
	nvs_flash
	app_update
	include
	drivers
	esp_timer
)
