idf_component_register(
    SRCS "main.cpp" "lvgl_amoled.c" "music_player_ui.c"
    INCLUDE_DIRS ".")

# # 添加Arduino USB CDC支持
# target_compile_definitions(${COMPONENT_LIB} PRIVATE
#     ARDUINO_USB_CDC_ON_BOOT=1
#     ARDUINO_USB_MODE=0
#     CONFIG_TINYUSB_CDC_ENABLED=1
# )
# 这里应该不行，要在上一级目录 CMakeLists.txt 加 Arduino 的相关宏

set_source_files_properties(
    ${LV_DEMOS_SOURCES}
    PROPERTIES COMPILE_OPTIONS
    -DLV_LVGL_H_INCLUDE_SIMPLE)
