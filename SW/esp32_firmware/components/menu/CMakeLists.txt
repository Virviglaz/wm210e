idf_component_register(SRCS
"src/menu.cpp"
"src/thread_cut.cpp"
"src/motor_ctrl.cpp"
"src/cutting.cpp"

INCLUDE_DIRS
"../../include"
"../drivers/inc"
"../api/inc"
"inc"

PRIV_REQUIRES
drivers
api
app_update
)

add_compile_definitions(
	M42_GEARED512_MOTOR
)
