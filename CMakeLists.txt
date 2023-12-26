cmake_minimum_required(VERSION 3.26)
project(mqtt_slave)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

#thêm thư mục chứa thư viện
include_directories(host utils)

# Tạo thư viện tĩnh từ các tệp trong thư mục lib
add_library(MyLibrary STATIC
        host/sm_cmd.h
        host/sm_host.c
        host/sm_host.h
        utils/CRC.c
        utils/CRC.h
        utils/linux/linux_serial.h
        utils/linux/linux_serial.c
)


# Thêm các nguồn và liên kết với thư viện tĩnh
add_executable(mqtt_slave main.cpp)
target_link_libraries(mqtt_slave PRIVATE MyLibrary pthread libmosquitto)

add_subdirectory(lib)
add_subdirectory(src)
