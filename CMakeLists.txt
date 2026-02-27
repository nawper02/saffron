cmake_minimum_required(VERSION 3.10)
project(Saffron C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(GNUInstallDirs)
find_package(SDL2 REQUIRED)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${SDL2_INCLUDE_DIRS})
file(GLOB EXAMPLE_SOURCES "examples/*.c")
foreach(EXAMPLE_SRC ${EXAMPLE_SOURCES})
    get_filename_component(TARGET_NAME ${EXAMPLE_SRC} NAME_WE)
    add_executable(${TARGET_NAME} ${EXAMPLE_SRC})
    target_link_libraries(${TARGET_NAME} ${SDL2_LIBRARIES} m)
    target_compile_definitions(${TARGET_NAME} PRIVATE 
        SAFFRON_ASSET_DIR="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/saffron/assets/"
    )
    install(TARGETS ${TARGET_NAME} 
            DESTINATION ${CMAKE_INSTALL_BINDIR})
endforeach()
install(FILES "saffron.h" 
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY "sf_extras" 
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "assets/" 
        DESTINATION ${CMAKE_INSTALL_DATADIR}/saffron/assets)
