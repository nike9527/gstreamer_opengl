#include "GstOpenGLPlayer.hpp"

extern GLuint create_shader_program();
extern const unsigned int *get_screen_quad_indices();
extern const float *get_screen_quad_vertices();
GstOpenGLPlayer::GstOpenGLPlayer(int width, int height)
    : window_width_(width), window_height_(height),
      window_(nullptr), vao_(0), vbo_(0), ebo_(0),
      shader_program_(0), texture_id_(0),
      texture_width_(0), texture_height_(0),
      pipeline_(nullptr), appsink_(nullptr), bus_(nullptr),
      is_running_(false), has_new_frame_(false)
{
}

GstOpenGLPlayer::~GstOpenGLPlayer()
{
    stop();
}
void GstOpenGLPlayer::createTestImage()
{
    // 创建棋盘格纹理
    for (int y = 0; y < texture_height_; ++y)
    {
        for (int x = 0; x < texture_width_; ++x)
        {
            int index = (y * texture_width_ + x) * 4;
            bool isRed = ((x / 32) % 2) ^ ((y / 32) % 2);

            if (isRed)
            {
                texture_data_[index] = 255;   // R
                texture_data_[index + 1] = 0; // G
                texture_data_[index + 2] = 0; // B
                texture_data_[index + 2] = 0; // A
            }
            else
            {
                texture_data_[index] = 0;       // R
                texture_data_[index + 1] = 0;   // G
                texture_data_[index + 2] = 255; // B
                texture_data_[index + 2] = 0;   // A
            }
        }
    }
}
bool GstOpenGLPlayer::create_window()
{
    // 初始化 GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // 设置OpenGL版本和配置
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);                 // OpenGL主版本3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);                 // OpenGL次版本3
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 核心模式
    // 创建窗口
    window_ = glfwCreateWindow(window_width_, window_height_, "GStreamer + OpenGL Video Player", nullptr, nullptr);
    glfwSwapInterval(1); // 启用垂直同步
    if (!window_)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_); // 设置为当前上下文
    glfwSwapInterval(1);             // 启用垂直同步

    // 初始化 GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) // 初始化GLAD（加载OpenGL函数指针)
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    glViewport(0, 0, window_width_, window_height_);
    return true;
}

bool GstOpenGLPlayer::init_opengl()
{
    // 创建着色器程序
    shader_program_ = create_shader_program();

    if (!shader_program_)
    {
        std::cerr << "Failed to create program" << std::endl;
        return false;
    }

    // 创建 VAO, VBO, EBO
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);

    // 配置顶点数据
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    const void *vertices = (const void *)get_screen_quad_vertices();
    glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(float), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    const void *indices = (const void *)get_screen_quad_indices();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(int), indices, GL_STATIC_DRAW);

    // 位置属性
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // 纹理坐标属性
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // 创建纹理
    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);

    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 初始化为黑色纹理
    texture_width_ = 640;
    texture_height_ = 480;
    texture_data_.resize(texture_width_ * texture_height_ * 4, 0);
    // createTestImage();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_width_, texture_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data_.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glUniform1i(glGetUniformLocation(shader_program_, "videoTexture"), 0);
    glUseProgram(shader_program_);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

bool GstOpenGLPlayer::initialize(const std::string &video_source)
{
    // 初始化 window
    if (!create_window())
    {
        std::cerr << "Failed to initialize window" << std::endl;
        return false;
    }
    // 初始化 OpenGL
    if (!init_opengl())
    {
        std::cerr << "Failed to initialize OpenGL" << std::endl;
        return false;
    }

    // 初始化 GStreamer
    gst_init(nullptr, nullptr);

    // 创建 GStreamer 管道
    if (!create_pipeline(video_source))
    {
        std::cerr << "Failed to create GStreamer pipeline" << std::endl;
        return false;
    }

    is_running_ = true;
    return true;
}

bool GstOpenGLPlayer::create_pipeline(const std::string &source)
{
    std::string pipeline_str;

    if (source.empty() || source == "test")
    {
        // 测试视频源
        pipeline_str = "videotestsrc pattern=snow ! "
                       "video/x-raw,width=640,height=480 ! "
                       "queue ! appsink name=sink emit-signals=true sync=false";
    }
    else if (source.find("rtsp://") == 0 || source.find("rtmp://") == 0)
    {
        // 网络流
        pipeline_str = "rtspsrc location=" + source + " latency=0 ! "
                                                      "rtph264depay ! h264parse ! avdec_h264 ! "
                                                      "videoconvert ! video/x-raw,format=RGBA ! "
                                                      "queue ! appsink name=sink emit-signals=true sync=false";
    }
    else if (source.find("http://") == 0 || source.find("https://") == 0)
    {
        // HTTP 流
        pipeline_str = "souphttpsrc location=" + source + " ! "
                                                          "decodebin ! videoconvert ! video/x-raw,format=RGBA ! "
                                                          "queue ! appsink name=sink emit-signals=true sync=false";
    }
    else
    {
        // 本地文件
        // pipeline_str = "filesrc location=" + source + " ! "
        //                                               "decodebin ! videoconvert !"
        //                                               "video/x-raw,format=RGBA ! "
        //                                               "queue ! appsink name=sink emit-signals=true sync=true";
        // pipeline_str = "filesrc location=" + source + " ! "
        //                                               "decodebin name=dec ! "
        //                                               "queue ! videoconvert ! video/x-raw,format=RGBA ! appsink name=sink emit-signals=true sync=true ";

        pipeline_str = "filesrc location=" + source + " ! "
                                                      "matroskademux name=dec ! "
                                                      "queue ! vorbisdec ! audioresample ! autoaudiosink dec. !"
                                                      "queue ! vp8dec ! videoconvert ! textoverlay name=overlay font-desc=\"Sans Bold 10\" ! video/x-raw,format=RGBA ! appsink name=sink emit-signals=true sync=true";
        // pipeline_str = "filesrc location=" + source + " ! "
        //                                               "qtdemux name=dec "
        //                                               "dec.video_0  ! queue ! decodebin ! videoconvert ! video/x-raw,format=RGBA ! appsink name=sink emit-signals=true sync=true";
    }
    std::cout << "Creating pipeline: " << pipeline_str << std::endl;

    GError *error = nullptr;
    pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error);

    if (error)
    {
        std::cerr << "Failed to create pipeline: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }

    if (!pipeline_)
    {
        std::cerr << "Pipeline is null" << std::endl;
        return false;
    }

    // 获取 appsink
    appsink_ = gst_bin_get_by_name(GST_BIN(pipeline_), "sink");
    if (!appsink_)
    {
        std::cerr << "Failed to get appsink element" << std::endl;
        return false;
    }

    // 设置 appsink 属性
    g_object_set(appsink_, "emit-signals", TRUE, "max-buffers", 1, "drop", TRUE, nullptr);

    // 连接新样本信号
    g_signal_connect(appsink_, "new-sample", G_CALLBACK(new_sample_callback), this);

    // 获取总线并连接消息回调
    bus_ = gst_element_get_bus(pipeline_);

    gst_bus_add_watch(bus_, bus_callback, this);

    return true;
}

GstFlowReturn GstOpenGLPlayer::new_sample_callback(GstElement *sink, gpointer data)
{
    GstOpenGLPlayer *player = static_cast<GstOpenGLPlayer *>(data);

    // 拉取样本 阻塞直到有样本可用、EOS 或超时
    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    if (!sample)
    {
        g_printerr("Failed to pull sample\n");
        return GST_FLOW_ERROR;
    }

    // 获取 buffer
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstCaps *caps = gst_sample_get_caps(sample);

    if (buffer && caps)
    {
        // 解析视频信息
        GstVideoInfo info;
        gst_video_info_from_caps(&info, caps);

        int width = GST_VIDEO_INFO_WIDTH(&info);
        int height = GST_VIDEO_INFO_HEIGHT(&info);

        // 映射 buffer
        GstMapInfo map;
        if (gst_buffer_map(buffer, &map, GST_MAP_READ))
        {
            // std::lock_guard<std::mutex> lock(player->texture_mutex_);
            // 检查纹理尺寸是否需要更新
            if (width != player->texture_width_ || height != player->texture_height_)
            {
                player->texture_width_ = width;
                player->texture_height_ = height;
                player->texture_data_.resize(width * height * 4);
            }

            // 复制数据（假设数据是 RGBA 格式）
            size_t data_size = map.size;
            size_t expected_size = width * height * 4;

            if (data_size >= expected_size)
            {
                memcpy(player->texture_data_.data(), map.data, expected_size);
                player->has_new_frame_ = true;
            }
            gst_buffer_unmap(buffer, &map);
        }
    }

    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

gboolean GstOpenGLPlayer::bus_callback(GstBus *bus, GstMessage *msg, gpointer data)
{
    GstOpenGLPlayer *player = static_cast<GstOpenGLPlayer *>(data);

    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_ERROR:
    {
        GError *err = nullptr;
        gchar *debug = nullptr;
        gst_message_parse_error(msg, &err, &debug);

        std::cerr << "GStreamer error: " << err->message << std::endl;
        if (debug)
        {
            std::cerr << "Debug info: " << debug << std::endl;
            g_free(debug);
        }

        g_error_free(err);
        player->stop();
        break;
    }
    case GST_MESSAGE_EOS:
        std::cout << "End of stream" << std::endl;
        player->stop();
        break;
    case GST_MESSAGE_STATE_CHANGED:
    {
        GstState old_state, new_state, pending_state;
        gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);

        if (GST_MESSAGE_SRC(msg) == GST_OBJECT(player->pipeline_))
        {
            std::cout << "Pipeline state changed from "
                      << gst_element_state_get_name(old_state)
                      << " to " << gst_element_state_get_name(new_state)
                      << std::endl;
        }
        break;
    }
    default:
        break;
    }

    return TRUE;
}
// 更新显示的文本
void GstOpenGLPlayer::update_overlay_text()
{

    gchar *text = NULL;

    // 获取当前播放位置
    gint64 position = 0;
    if (!gst_element_query_position(pipeline_, GST_FORMAT_TIME, &position))
    {
        position = 0;
    }
    printf("更新----");
    // 计算时间（时:分:秒.毫秒）
    guint hours = (guint)(position / (3600 * GST_SECOND));
    guint minutes = (guint)((position % (3600 * GST_SECOND)) / (60 * GST_SECOND));
    guint seconds = (guint)((position % (60 * GST_SECOND)) / GST_SECOND);
    guint milliseconds = (guint)((position % GST_SECOND) / GST_MSECOND);

    // 计算持续时间
    if (duration_ns <= 0)
    {
        gst_element_query_duration(pipeline_, GST_FORMAT_TIME, &duration_ns);
    }
    guint dur_hours = (guint)(duration_ns / (3600 * GST_SECOND));
    guint dur_minutes = (guint)((duration_ns % (3600 * GST_SECOND)) / (60 * GST_SECOND));
    guint dur_seconds = (guint)((duration_ns % (60 * GST_SECOND)) / GST_SECOND);

    // 生成显示文本
    text = g_strdup_printf(
        "时间: %02u:%02u:%02u.%03u / %02u:%02u:%02u\n"
        "帧率: %.1f fps\n"
        "状态: %s",
        hours, minutes, seconds, milliseconds,
        dur_hours, dur_minutes, dur_seconds,
        current_fps,
        is_running_ ? "播放中" : "暂停");

    g_object_set(textoverlay, "text", text, NULL);
    g_free(text);
}

// 计算帧率
void GstOpenGLPlayer::calculate_fps()
{
}
void GstOpenGLPlayer::update_display(gpointer user_data)
{
    GstOpenGLPlayer *player = (GstOpenGLPlayer *)user_data;

    // 更新帧率显示（每秒更新一次）
    static guint update_counter = 0;
    update_counter++;

    if (update_counter % 30 == 0)
    { // 假设30fps，每秒更新一次
        player->update_overlay_text();
    }

    printf("更新");
    return;
}

void GstOpenGLPlayer::render_frame()
{
    // 更新纹理（如果有新帧）
    if (!has_new_frame_)
        return;
    updateTextureData();
    // 清除颜色缓冲区
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id_);

    // render container
    glUseProgram(shader_program_);
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    // -------------------------------------------------------------------------------
    glfwSwapBuffers(window_);

    // 在开始渲染循环前等待一下，让GPU完成纹理上传
    // std::this_thread::sleep_for(std::chrono::milliseconds(33));
    // 解绑
    glBindVertexArray(0);
    glUseProgram(0);
}
// 更新纹理数据（用于视频帧）
void GstOpenGLPlayer::updateTextureData()
{

    // std::lock_guard<std::mutex> lock(texture_mutex_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    if (has_new_frame_)
    {
        // 重新分配纹理内存
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_width_, texture_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data_.data());
        has_new_frame_ = false;
        std::cout << "Texture resized to: " << texture_width_ << "x" << texture_height_ << std::endl;
    }
    else
    {
        // 更新纹理数据（更高效的方式）
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture_width_, texture_height_, GL_RGBA, GL_UNSIGNED_BYTE, texture_data_.data());
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    has_new_frame_ = false;
}
void GstOpenGLPlayer::run()
{
    if (!is_running_)
        return;
    // 设置定时器更新显示
    guint timer_id = g_timeout_add(1000 / 30, (GSourceFunc)update_display, this); // 30Hz更新
    gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    std::cout << "Pipeline started" << std::endl;
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    std::thread gst_loop_thread([loop]()
                                { g_main_loop_run(loop); });
    // 获取textoverlay元素
    textoverlay = gst_bin_get_by_name(GST_BIN(pipeline_), "overlay");
    while (is_running_ && !glfwWindowShouldClose(window_))
    {
        glfwPollEvents();
        render_frame();
        // 小延迟以减少 CPU 使用率
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    // 清理
    g_source_remove(timer_id);
    gst_element_set_state(pipeline_, GST_STATE_NULL);
    g_main_loop_quit(loop);
    gst_loop_thread.join();
    g_main_loop_unref(loop);

    stop();
}

void GstOpenGLPlayer::stop()
{
    is_running_ = false;

    // 停止管道
    if (pipeline_)
    {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
    }

    // 清理 GStreamer 资源
    cleanup_pipeline();

    // 清理 OpenGL 资源
    cleanup_opengl();

    // 销毁窗口
    if (window_)
    {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }

    glfwTerminate();
}

void GstOpenGLPlayer::cleanup_pipeline()
{
    if (bus_)
    {
        gst_object_unref(bus_);
        bus_ = nullptr;
    }

    if (appsink_)
    {
        gst_object_unref(appsink_);
        appsink_ = nullptr;
    }

    if (pipeline_)
    {
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
    }
}

void GstOpenGLPlayer::cleanup_opengl()
{
    if (texture_id_)
    {
        glDeleteTextures(1, &texture_id_);
        texture_id_ = 0;
    }

    if (ebo_)
    {
        glDeleteBuffers(1, &ebo_);
        ebo_ = 0;
    }

    if (vbo_)
    {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }

    if (vao_)
    {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }

    if (shader_program_)
    {
        glDeleteProgram(shader_program_);
        shader_program_ = 0;
    }
}
