# Embed the server root certificate into the final binary
FILE(GLOB MyCSources *.c)
idf_component_register(SRCS ${MyCSources}
                    INCLUDE_DIRS "." "include" "../components/esp_littlefs/include" "../components/esp_littlefs/src" "../components/esp_littlefs/src/littlefs" "../components/esp_littlefs/src/littlefs/bd"
                    EMBED_TXTFILES ${project_dir}/main/howsmyssl_com_root_cert.pem)