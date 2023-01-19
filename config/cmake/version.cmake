find_package(Git QUIET)

if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --abbrev=7 --always
        WORKING_DIRECTORY ${ZEPHYR_BASE}
        OUTPUT_VARIABLE ZEPHYR_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --abbrev=7 --always
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE ZMK_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --abbrev=7 --always
        WORKING_DIRECTORY ${ZMK_CONFIG}
        OUTPUT_VARIABLE APP_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

if(NOT ZEPHYR_VERSION)
    message(WARNING "ZEPHYR_VERSION not detected")
    set(ZEPHYR_VERSION "unknown")
endif()

if(NOT ZMK_VERSION)
    message(WARNING "ZMK_VERSION not detected")
    set(ZMK_VERSION "unknown")
endif()

if(NOT APP_VERSION)
    message(WARNING "APP_VERSION not detected")
    set(APP_VERSION "unknown")
endif()
