set(req driver freertos log)

idf_component_register(
    SRCS dht.c
    INCLUDE_DIRS .
    REQUIRES ${req}
)
