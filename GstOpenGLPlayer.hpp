#pragma once
#include "gst/gst.h"
#include "gst/video/video.h"
#include "gst/gl/gl.h"
#include "gst/app/gstappsink.h"

#include "GLFW/glfw3.h"
#include "glad/glad.h"

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <vector>
class GstOpenGLPlayer
{

public:
    GstOpenGLPlayer(int width = 800, int height = 600);
    ~GstOpenGLPlayer();
    bool initialize(const std::string &video_source = "");
    void run();
    void stop();

private:
    // GStreamer 回调
    static GstFlowReturn new_sample_callback(GstElement *sink, gpointer data);
    static gboolean bus_callback(GstBus *bus, GstMessage *msg, gpointer data);
    static void update_display(gpointer app);
    void update_overlay_text();
    void calculate_fps();
    bool create_window();
    // OpenGL 相关
    bool init_opengl();
    void render_frame();
    void cleanup_opengl();

    // GStreamer 相关
    bool create_pipeline(const std::string &source);
    void cleanup_pipeline();
    void updateTextureData();
    // opengl渲染测试
    void createTestImage();

private:
    int window_width_;
    int window_height_;

    // GLFW 窗口
    GLFWwindow *window_;

    // OpenGL 资源
    GLuint vao_;
    GLuint vbo_;
    GLuint ebo_;
    GLuint shader_program_;
    GLuint texture_id_;

    // 纹理参数
    int texture_width_;
    int texture_height_;
    std::vector<uint8_t> texture_data_;
    std::mutex texture_mutex_;

    // GStreamer 资源
    GstElement *pipeline_;
    GstElement *appsink_;
    GstBus *bus_;
    gint64 duration_ns;
    gdouble current_fps;
    GstElement *textoverlay;

    // 控制标志
    bool is_running_;
    bool has_new_frame_;
};
