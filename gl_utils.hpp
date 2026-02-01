#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    // 声明为外部常量数组
    extern const float vertices[16];      // 4个顶点 * 4个float
    extern const unsigned int indices[6]; // 2个三角形 * 3个索引

#ifdef __cplusplus
}
#endif