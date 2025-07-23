idf_component_register(
    SRCS "main.cpp" "lvgl_amoled.c" "music_player_ui.c"
    INCLUDE_DIRS ".")
# 先暂时不需要显式写 REQUIRES espressif__freetype esp_lcd_qspi_amoled
# 貌似会增加所有依赖，写了反而少，会找不到没写进来的依赖

# # 添加Arduino USB CDC支持
# target_compile_definitions(${COMPONENT_LIB} PRIVATE
#     ARDUINO_USB_CDC_ON_BOOT=1
#     ARDUINO_USB_MODE=0
#     CONFIG_TINYUSB_CDC_ENABLED=1
# )
# 这里应该不行，要在上一级目录 CMakeLists.txt 加 Arduino 的相关宏

# 而且还有个问题Arduino的Serial拔掉线以后只供电，就无法正常运行程序了，
set_source_files_properties(
    ${LV_DEMOS_SOURCES}
    PROPERTIES COMPILE_OPTIONS
    -DLV_LVGL_H_INCLUDE_SIMPLE)

# Solution from https://esp32.com/viewtopic.php?t=35419
idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
idf_component_get_property(freetype_lib espressif__freetype COMPONENT_LIB)
target_link_libraries(${lvgl_lib} PUBLIC ${freetype_lib})
