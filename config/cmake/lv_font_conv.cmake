# Copyright (c) 2022-2023 XiNGRZ
# SPDX-License-Identifier: MIT

function(LV_FONT_CONV OUT SRC)
    set(options NO_COMPRESS NO_PREFILTER NO_KERNING)
    set(one_value_keywords NAME SIZE BPP)
    set(multi_value_keywords RANGE)
    cmake_parse_arguments(LV_FONT_CONV
        "${options}"
        "${one_value_keywords}"
        "${multi_value_keywords}"
        ${ARGN}
    )

    file(REAL_PATH ${SRC} FONT_SRC)

    set(FONT_OUT ${CMAKE_CURRENT_BINARY_DIR}/${LV_FONT_CONV_NAME}_${LV_FONT_CONV_SIZE}.c)
    set(${OUT} ${FONT_OUT} PARENT_SCOPE)

    set(LV_FONT_CONV_OPTS)

    if(LV_FONT_CONV_NO_COMPRESS)
        list(APPEND LV_FONT_CONV_OPTS "--no-compress")
    endif()

    if(LV_FONT_CONV_NO_PREFILTER)
        list(APPEND LV_FONT_CONV_OPTS "--no-prefilter")
    endif()

    if(LV_FONT_CONV_NO_KERNING)
        list(APPEND LV_FONT_CONV_OPTS "--no-kerning")
    endif()

    if (LV_FONT_CONV_RANGE)
        string(JOIN "," LV_FONT_CONV_RANGE ${LV_FONT_CONV_RANGE})
    else()
        set(LV_FONT_CONV_RANGE "0x0000-0xFFFF")
    endif()

    add_custom_command(
        OUTPUT ${FONT_OUT}
        COMMAND lv_font_conv
        ARGS    --size ${FONT_SIZE}
                --output "${FONT_OUT}"
                --bpp ${LV_FONT_CONV_BPP}
                --format lvgl
                --font "${FONT_SRC}"
                --range ${LV_FONT_CONV_RANGE}
                ${LV_FONT_CONV_OPTS}
        DEPENDS ${FONT_SRC}
                ${LV_FONT_CONV_RANGE_FILE}
        COMMENT "Generating font in LVGL format: ${FONT_NAME}_${FONT_SIZE}.c"
    )

    set_source_files_properties(
        ${FONT_OUT}
        PROPERTIES GENERATED TRUE
    )
endfunction()
