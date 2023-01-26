# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

function(FONT_SUBSET OUT SRC TEXT)
    file(REAL_PATH ${SRC} FONT_FILE)
    file(REAL_PATH ${TEXT} TEXT_FILE)

    get_filename_component(FONT_NAME ${SRC} NAME_WE)
    get_filename_component(FONT_EXT ${SRC} EXT)

    set(OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${FONT_NAME}_subset${FONT_EXT})
    set(${OUT} ${OUTPUT_FILE} PARENT_SCOPE)

    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND pyftsubset
        ARGS    ${FONT_FILE}
                --text-file=${TEXT_FILE}
                --output-file=${OUTPUT_FILE}
        DEPENDS ${FONT_FILE}
                ${TEXT_FILE}
        COMMENT "Generating subset font: ${FONT_NAME}_subset${FONT_EXT}"
    )

    set_source_files_properties(
        ${OUTPUT_FILE}
        PROPERTIES GENERATED TRUE
    )
endfunction()
