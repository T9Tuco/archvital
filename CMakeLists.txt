cmake_minimum_required(VERSION 3.20)
project(archvital VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/core/systemmonitor.cpp
    src/ui/sidebar.cpp
    src/ui/sparkline.cpp
    src/ui/toastwidget.cpp
    src/ui/toastmanager.cpp
    src/ui/metriccard.cpp
    src/sections/overviewsection.cpp
    src/sections/cpusection.cpp
    src/sections/memorysection.cpp
    src/sections/gpusection.cpp
    src/sections/disksection.cpp
    src/sections/networksection.cpp
    src/sections/tasksection.cpp
    src/sections/diagnosticssection.cpp
    src/sections/settingssection.cpp
)

add_executable(archvital ${SOURCES})

target_include_directories(archvital PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(archvital PRIVATE Qt6::Widgets Qt6::Core)

target_compile_options(archvital PRIVATE -Wall -Wextra -Wpedantic)

install(TARGETS archvital RUNTIME DESTINATION bin)
