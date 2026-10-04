# Auto-generated CMakeLists.txt for C++ Project
# Project: theSpeaker (3 boards)

# Collect all source files using glob patterns
file(GLOB_RECURSE ALL_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

# Engine-generated PAL code from the sibling src/ directory
# Only include if corresponding headers exist (avoids broken stubs)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src")
    file(GLOB ENGINE_SRC_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/*.c"
    )
    foreach(_esf ${ENGINE_SRC_FILES})
        get_filename_component(_esf_name ${_esf} NAME_WE)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src/${_esf_name}.h" OR
           EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/${_esf_name}.h")
            list(APPEND ALL_SOURCES ${_esf})
        endif()
    endforeach()
endif()

# Exclude backup/baseline files from compilation
list(FILTER ALL_SOURCES EXCLUDE REGEX ".*\\.baselines/.*")
list(FILTER ALL_SOURCES EXCLUDE REGEX ".*/test/.*")

# Exclude PAL mDNS if dataflow_wifi is not present (avoids pulling mdns managed component)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dataflow_wifi.c" AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dataflow_wifi.cpp")
    list(FILTER ALL_SOURCES EXCLUDE REGEX ".*/pal_mdns\\.c$")
endif()

# Build include directories list - only include existing directories
set(INCLUDE_DIRS ".")
foreach(subdir include src base sensors actuators displays communication input middleware osal periphery_state hal msl pal nal)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}")
        list(APPEND INCLUDE_DIRS "${subdir}")
    endif()
endforeach()

# Include engine-generated headers from sibling src/ directory
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src")
    list(APPEND INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../src")
endif()

# Register component with all sources
idf_component_register(
    SRCS ${ALL_SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES 
        bt
        esp-tls
        esp_adc
        esp_common
        esp_driver_dma
        esp_driver_gpio
        esp_driver_i2c
        esp_driver_i2s
        esp_driver_spi
        esp_driver_uart
        esp_event
        esp_http_client
        esp_http_server
        esp_lcd
        esp_netif
        esp_system
        esp_timer
        esp_websocket_client
        esp_wifi
        freertos
        log
        lwip
        mbedtls
        nvs_flash
)

# Enable C++ features only for C++ files (avoid -frtti error on C files)
# Also suppress missing-field-initializers warnings (ESP-IDF structs get extended across versions)
get_target_property(COMP_SOURCES ${COMPONENT_LIB} SOURCES)
foreach(src ${COMP_SOURCES})
    if(src MATCHES "\\.(cpp|cxx|cc)$")
        set_source_files_properties(${src} PROPERTIES COMPILE_FLAGS "-fexceptions -frtti -Wno-error=missing-field-initializers")
    endif()
    if(src MATCHES "\\.c$")
        set_source_files_properties(${src} PROPERTIES COMPILE_FLAGS "-Wno-error=missing-field-initializers")
    endif()
endforeach()
