#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include "glad/glad.h"

// 顶点着色器源码
const char *vertex_shader_source = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

// 片段着色器源码
const char *fragment_shader_source = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D videoTexture;

void main() {
    FragColor = texture(videoTexture, TexCoord);
    // 如果需要颜色转换（YUV到RGB），可以在这里添加
    // FragColor.rgb = vec3(dot(FragColor.rgb, vec3(0.299, 0.587, 0.114)));
}
)";

// 全屏四边形顶点数据
const float vertices[] = {
    // 位置          // 纹理坐标
    -1.0f, 1.0f, 0.0f, 0.0f,  // 左上
    -1.0f, -1.0f, 0.0f, 1.0f, // 左下
    1.0f, -1.0f, 1.0f, 1.0f,  // 右下
    1.0f, 1.0f, 1.0f, 0.0f    // 右上
};

const unsigned int indices[] = {
    0, 1, 2, // 第一个三角形
    0, 2, 3  // 第二个三角形
};

// 编译着色器
GLuint compile_shader(GLenum type, const char *source)
{

    GLuint shader = glCreateShader(type);        // 创建着色器对象
    glShaderSource(shader, 1, &source, nullptr); // 设置着色器源代码
    glCompileShader(shader);                     // 编译着色器
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success); // 检查编译错误
    if (!success)
    {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, nullptr, info_log);
        std::cerr << "Shader compilation failed:\n"
                  << info_log << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}
// 创建着色器程序
GLuint create_shader_program()
{
    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_shader_source); // 编译顶点着色器
    if (!vertex_shader)
        return 0;

    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_source); // 编译顶点着色器
    if (!fragment_shader)
    {
        glDeleteShader(vertex_shader);
        return 0;
    }

    GLuint program = glCreateProgram();       // 创建程序对象
    glAttachShader(program, vertex_shader);   // 附加顶点着色器到程序
    glAttachShader(program, fragment_shader); // 附加顶点着色器到程序
    glLinkProgram(program);                   // 链接着色器程序

    glDeleteShader(vertex_shader);   // 立即删除着色器对象
    glDeleteShader(fragment_shader); // 立即删除着色器对象
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success); // 检查链接错误
    if (!success)
    {
        char info_log[512];
        glGetProgramInfoLog(program, 512, nullptr, info_log);
        std::cerr << "Shader program linking failed:\n"
                  << info_log << std::endl;
    }
    return program;
}

const float *get_screen_quad_vertices()
{
    return vertices;
}

const unsigned int *get_screen_quad_indices()
{
    return indices;
}
