get_filename_component(dir ${CMAKE_CURRENT_LIST_FILE} PATH)
FILE(GLOB_RECURSE library_sources ${dir}/ESP32-audioI2S/src/*.cpp)

idf_component_register(SRCS ${library_sources}      
                    REQUIRES "ESP32-audioI2S"
                    INCLUDE_DIRS "ESP32-audioI2S/src"
                    REQUIRES wear_levelling arduino-esp32
)
