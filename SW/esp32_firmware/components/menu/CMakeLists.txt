idf_component_register(SRCS
"src/menu.cpp"
"src/thread_cut.cpp"

INCLUDE_DIRS
"../../include"
"../drivers/inc"
"../api/inc"
"inc"

PRIV_REQUIRES
drivers
api
)

add_compile_definitions(
	M42_GEARED512_MOTOR
)