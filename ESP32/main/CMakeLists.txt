idf_component_register(SRCS "thermostat.c" "network/wifi/wifi.c" "network/http/http.c" "device/TMP175_alt/tmp175.c" "device/relay/relay.c" "device/LED/LED.c" "device/LM35/LM35.c" "device/pushbutton/pushbutton.c" "controller/hysteresis/hysteresis.c" "controller/configuration/storage.c" "controller/configuration/handlers.c" "controller/estimator/estimator.c" INCLUDE_DIRS "")

# Create a SPIFFS image from the contents of the 'spiffs_image' directory
# that fits the partition named 'storage'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash'.
spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)

