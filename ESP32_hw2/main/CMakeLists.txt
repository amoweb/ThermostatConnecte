idf_component_register(SRCS "thermostat.c"
                    INCLUDE_DIRS "." "include")

# Créer une image SPIFFS à partir du contenu du répertoire 'spiffs_image' 
# qui correspond à la partition nommée 'storage'. FLASH_IN_PROJECT indique 
# que l'image générée doit être flassée lorsque tout le projet est flashé
# vers la cible avec 'idf.py -p PORT flash'.
spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)
