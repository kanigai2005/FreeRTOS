# CMakeLists.txt for the main component of the ESP32 custom OS project
idf_component_register(SRCS "watchdog_manager.c" 
                            "intelligent_engine.c" 
                            "main.c" 
                            "system_health.c" 
                            "storage_guard.c" # or system_guard.c if renamed
                       INCLUDE_DIRS ".")