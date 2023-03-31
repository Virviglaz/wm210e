idf_component_register(SRCS
	"src/menu.cpp"
	"src/thread_cut.cpp"
	"src/motor_ctrl.cpp"
	"src/cutting.cpp"

INCLUDE_DIRS
	"inc"
	"../../include"
REQUIRES
	driver
PRIV_REQUIRES
	include
	drivers
	api
	app_update
	ota
	wifi
	esp_timer
)

add_compile_definitions(
	M42_GEARED512_MOTOR
)
