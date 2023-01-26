# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

function(GENERATE_UI_STRINGS INPUT)
    set(options)
    set(one_value_keywords OUTPUT_DEFS OUTPUT_TEXT)
    set(multi_value_keywords)
    cmake_parse_arguments(UI_STRINGS
        "${options}"
        "${one_value_keywords}"
        "${multi_value_keywords}"
        ${ARGN}
    )

    file(REAL_PATH ${INPUT} INPUT)

    get_filename_component(NAME ${INPUT} NAME_WE)

    set(DEFS_FILE ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.h)
    set(TEXT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.txt)

    add_custom_command(
        OUTPUT  ${DEFS_FILE} ${TEXT_FILE}
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/ui_strings.py
        ARGS    --input ${INPUT}
                --output-defs ${DEFS_FILE}
                --output-text ${TEXT_FILE}
        DEPENDS ${INPUT}
        COMMENT "Generating UI strings: ${NAME}.h, ${NAME}.txt"
    )

    set_source_files_properties(
        ${DEFS_FILE}
        ${TEXT_FILE}
        PROPERTIES GENERATED TRUE
    )

    set(${UI_STRINGS_OUTPUT_DEFS} ${DEFS_FILE} PARENT_SCOPE)
    set(${UI_STRINGS_OUTPUT_TEXT} ${TEXT_FILE} PARENT_SCOPE)
endfunction()
