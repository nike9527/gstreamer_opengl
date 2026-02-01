cmake_minimum_required(VERSION 3.18)
project(cuda_gstreamer LANGUAGES C CXX CUDA)

set(CMAKE_CXX_STANDARD 17)


# 如果是 MSVC，设置运行时库
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# 查找 OpenGL
find_package(OpenGL REQUIRED)
# 查找 CUDA
if(NOT CUDA_TOOLKIT_ROOT_DIR)
    set(CUDA_TOOLKIT_ROOT_DIR "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.8")
endif()
set(GLFW_DIR "${CMAKE_SOURCE_DIR}/GLFW")
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


# 添加可执行文件
add_executable(cuda_gstreamer
    main.cpp
    glad/glad.c
    gl_utils.cpp
    GstOpenGLPlayer.cpp
)

# 设置包含目录
target_include_directories(cuda_gstreamer PRIVATE
    ${CMAKE_SOURCE_DIR}/glad/include
    ${CMAKE_SOURCE_DIR}/GLFW/include
    ${GSTREAMER_INCLUDE_DIRS}
)

# 设置链接目录
target_link_directories(cuda_gstreamer PRIVATE
    "${GLFW_DIR}/lib"
    "${GSTREAMER_LIBRARY_DIR}"
)

# 设置链接库
target_link_libraries(cuda_gstreamer
    glfw3
    OpenGL::GL
    ${GSTREAMER_LIBRARIES}
    opengl32.lib
    gdi32.lib
)
# 添加预处理器定义
target_compile_definitions(cuda_gstreamer PRIVATE
    GST_USE_UNSTABLE_API
    GLFW_INCLUDE_NONE  # 避免 GLFW 包含 OpenGL 头文件
)

# 如果是 CUDA 项目，设置 CUDA 属性
set_target_properties(cuda_gstreamer PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)

# 添加环境变量设置（用于调试）
if(WIN32)
    # 运行时需要设置 PATH
    # add_custom_command(TARGET cuda_gstreamer POST_BUILD
    #     COMMENT "Copying required DLLs..."
    #     COMMAND ${CMAKE_COMMAND} -E copy_if_different
    #         "${GSTREAMER_BIN_DIR}/gstreamer-1.0.dll"
    #         $<TARGET_FILE_DIR:cuda_gstreamer>
    #     COMMAND ${CMAKE_COMMAND} -E copy_if_different
    #         "${GSTREAMER_BIN_DIR}/glib-2.0.dll"
    #         $<TARGET_FILE_DIR:cuda_gstreamer>
    #     COMMAND ${CMAKE_COMMAND} -E copy_if_different
    #         "${GSTREAMER_BIN_DIR}/gobject-2.0.dll"
    #         $<TARGET_FILE_DIR:cuda_gstreamer>
    #     COMMAND ${CMAKE_COMMAND} -E copy_if_different
    #         "${GSTREAMER_BIN_DIR}/gio-2.0.dll"
    #         $<TARGET_FILE_DIR:cuda_gstreamer>
    # )
endif()