idf_component_register(
    SRCS "led.c"
    INCLUDE_DIRS "include"
)
#target_link_options(${COMPONENT_LIB} INTERFACE "-u __led_link__")
#target_link_options(${COMPONENT_LIB} INTERFACE "-Wl,-u,__led_link__")

