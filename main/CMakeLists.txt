# Embed the server root certificate into the final binary
FILE(GLOB MyCSources *.c)
idf_component_register(SRCS ${MyCSources}
                    INCLUDE_DIRS "." "include" 
                    EMBED_TXTFILES ${project_dir}/main/howsmyssl_com_root_cert.pem)