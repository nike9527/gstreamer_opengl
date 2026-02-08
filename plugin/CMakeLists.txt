cmake_minimum_required(VERSION 3.18)
project(my_element LANGUAGES C)  



# 如果是 MSVC，设置运行时库
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()


# GStreamer 路径设置 - 根据你的安装位置修正
set(GSTREAMER_DIR "D:/Program Files/gstreamer/1.0/msvc_x86_64")
set(GSTREAMER_LIBRARY_DIR "${GSTREAMER_DIR}/lib")
set(GSTREAMER_BIN_DIR "${GSTREAMER_DIR}/bin")

# 设置 GStreamer 相关变量
set(GSTREAMER_INCLUDE_DIRS
    "${GSTREAMER_DIR}/include/gstreamer-1.0"
    "${GSTREAMER_DIR}/include/glib-2.0"
    "${GSTREAMER_DIR}/lib/glib-2.0/include"
    "${GSTREAMER_DIR}/lib/gstreamer-1.0/include"
)

# 定义要链接的 GStreamer 库
set(GSTREAMER_LIBRARIES
    "${GSTREAMER_LIBRARY_DIR}/gstreamer-1.0.lib"
    "${GSTREAMER_LIBRARY_DIR}/gstbase-1.0.lib"
    "${GSTREAMER_LIBRARY_DIR}/gstapp-1.0.lib"
    "${GSTREAMER_LIBRARY_DIR}/gstvideo-1.0.lib"
    "${GSTREAMER_LIBRARY_DIR}/gstgl-1.0.lib"
    "${GSTREAMER_LIBRARY_DIR}/glib-2.0.lib"
    "${GSTREAMER_LIBRARY_DIR}/gobject-2.0.lib"
    "${GSTREAMER_LIBRARY_DIR}/gio-2.0.lib"
)

# 验证路径是否存在
message(STATUS "GStreamer directory: ${GSTREAMER_DIR}")
message(STATUS "GStreamer includes: ${GSTREAMER_INCLUDE_DIRS}")

# 添加定义
add_definitions(-DGST_PLUGIN_BUILD)
add_definitions(-DGST_EXPORTS)
# 添加可执行文件
add_library(plugin MODULE gstmyelement.c)

# 设置包含目录
target_include_directories(plugin PRIVATE
    ${GSTREAMER_INCLUDE_DIRS}
)

# 设置链接库
target_link_libraries(plugin ${GSTREAMER_LIBRARIES})
