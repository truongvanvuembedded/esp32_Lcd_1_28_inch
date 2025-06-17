idf_component_register(SRCS "HTTP.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp-tls esp_http_client common json)
