cmake_minimum_required(VERSION 3.10)
project(Saffron C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -ffast-math")
include(GNUInstallDirs)
find_package(SDL2 REQUIRED)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${SDL2_INCLUDE_DIRS})

# Build executables
file(GLOB EXAMPLE_SOURCES "sf_examples/*.c")
file(GLOB EXTRAS_SOURCES  "sf_extras/*.c")
foreach(EXAMPLE_SRC ${EXAMPLE_SOURCES} ${EXTRAS_SOURCES})
    get_filename_component(TARGET_NAME ${EXAMPLE_SRC} NAME_WE)
    add_executable(${TARGET_NAME} ${EXAMPLE_SRC})
    target_link_libraries(${TARGET_NAME} ${SDL2_LIBRARIES} m)
    target_compile_definitions(${TARGET_NAME} PRIVATE 
        SAFFRON_ASSET_DIR="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/saffron/sf_assets/"
    )
    install(TARGETS ${TARGET_NAME} 
            DESTINATION ${CMAKE_INSTALL_BINDIR})
endforeach()

# Build bitmap textures
file(GLOB_RECURSE ASSET_PNGS "sf_assets/*.png")
set(GENERATED_ASSETS "")
set(ASSET_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/sf_assets")
foreach(PNG_FILE ${ASSET_PNGS})
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/sf_assets" "${PNG_FILE}")
    get_filename_component(REL_DIR "${REL_PATH}" DIRECTORY)
    get_filename_component(FILE_NAME_WE "${PNG_FILE}" NAME_WE)
    set(OUT_DIR "${ASSET_OUTPUT_DIR}/${REL_DIR}")
    set(BMP_FILE "${OUT_DIR}/${FILE_NAME_WE}.bmp")
    file(MAKE_DIRECTORY "${OUT_DIR}")
    list(APPEND GENERATED_ASSETS "${BMP_FILE}")
    add_custom_command(
        OUTPUT "${BMP_FILE}"
        COMMAND sf_png2bmp "${PNG_FILE}" "${BMP_FILE}"
        DEPENDS sf_png2bmp "${PNG_FILE}"
        COMMENT "Converting texture: ${REL_PATH} -> .bmp"
    )
endforeach()
add_custom_target(generate_assets ALL DEPENDS ${GENERATED_ASSETS})

install(FILES "saffron.h" 
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY "sf_extras/" 
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "sf_assets/" 
        DESTINATION ${CMAKE_INSTALL_DATADIR}/saffron/sf_assets)
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/sf_assets/" 
        DESTINATION ${CMAKE_INSTALL_DATADIR}/saffron/sf_assets)
