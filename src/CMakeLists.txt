#
# Polygon-4 DB Tool
#

source_group("MOC files" ".*automoc\\.cpp")

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_definitions(-DUSE_QT)

add_custom_target(version ALL
    COMMAND git rev-list HEAD --count > version.h_
    COMMAND ${CMAKE_COMMAND} -E copy_if_different version.h_ ${CMAKE_CURRENT_BINARY_DIR}/version.h.in
)
set_target_properties(version PROPERTIES FOLDER Helpers)

file(GLOB db_tool_src "*.cpp" "*.h")

set(db_tool_src
    ${db_tool_src}

    resources/qm.qrc
    resources/rc.qrc
)

set(db_tool_rc
    resources/rc.qrc
)

file(GLOB_RECURSE dbmgr_src "${DATA_MANAGER_DIR}/src/*")
set(translate_src
    ${db_tool_src}
    ${dbmgr_src}
    ${MOCSrcs}
    ${UISrcs}

    resources/en_US.ts
    resources/ru_RU.ts
)

set(translations_src_original ${CMAKE_CURRENT_SOURCE_DIR}/resources/qm.qrc)
set(translations_src ${CMAKE_CURRENT_BINARY_DIR}/resources/qm.qrc)

add_custom_command(
    OUTPUT ${translations_src}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${translations_src_original} ${translations_src}
    MAIN_DEPENDENCY ${translations_src_original}
)

#qt5_create_translation(qm_files ${translate_src})
add_custom_target(translations DEPENDS ${qm_files})

qt5_add_resources(db_tool_resources ${db_tool_rc})
qt5_add_resources(db_tool_resources_qm ${translations_src})

#
# target
#

include_directories(${Qt5Core_INCLUDE_DIRS})
include_directories(${Qt5Widgets_INCLUDE_DIRS})
include_directories(${Qt5Multimedia_INCLUDE_DIRS})

set(SOLUTION_FOLDER Extern)

add_executable(db_tool WIN32 ${db_tool_src} ${MOCSrcs} ${UISrcs} ${db_tool_resources} ${db_tool_resources_qm} resources/res.rc)
target_link_libraries(db_tool Qt5::Widgets pvt.lzwdgc.polygon4.data_manager.data_manager)
add_dependencies(db_tool translations version)
set_target_properties(db_tool PROPERTIES AUTOMOC True)
