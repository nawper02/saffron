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
        SF_SRC_ASSET_PATH="${CMAKE_CURRENT_SOURCE_DIR}/sf_assets"
        SF_BUILD_ASSET_PATH="${CMAKE_CURRENT_BINARY_DIR}/sf_assets"
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
        DESTINATION ${CMAKE_INSTALL_DATADIR}/saffron/sf_assets
        PATTERN "*.png"       EXCLUDE
        PATTERN "sf_generated" EXCLUDE)
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/sf_assets/"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/saffron/sf_assets)

# Python for compile time generations and such
find_package(Python3 QUIET COMPONENTS Interpreter)

# Keep saffron.h implementation function order synced with header order
if(Python3_FOUND)
    set(SF_REORGANIZE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/sf_utils/sf_reorganize.py")
    set(SAFFRON_HEADER       "${CMAKE_CURRENT_SOURCE_DIR}/saffron.h")

    add_custom_target(reorganize_saffron ALL
        COMMAND ${Python3_EXECUTABLE}
                "${SF_REORGANIZE_SCRIPT}"
                "${SAFFRON_HEADER}"
                --in-place
                --backup
        DEPENDS "${SAFFRON_HEADER}"
                "${SF_REORGANIZE_SCRIPT}"
        COMMENT "Reorganizing saffron.h implementation order"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()

# Regenerate docs whenever saffron.h changes
if(Python3_FOUND)
    add_custom_command(
        OUTPUT  "${CMAKE_CURRENT_SOURCE_DIR}/sf_docs/saffron_manual.md"
        COMMAND ${Python3_EXECUTABLE}
                "${CMAKE_CURRENT_SOURCE_DIR}/sf_docs/gen_docs.py"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/saffron.h"
                "${CMAKE_CURRENT_SOURCE_DIR}/sf_docs/gen_docs.py"
        COMMENT "Generating sf_docs/saffron_manual.md"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    add_custom_target(generate_docs ALL
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/sf_docs/saffron_manual.md"
    )
endif()
