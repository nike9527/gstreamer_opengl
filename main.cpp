#include "GstOpenGLPlayer.hpp"
int main(int argc, char *argv[])
{
    // std::string video_source = "https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm"; // 默认使用测试源
    // std::string video_source = "./yoga.mp4";        // 默认使用测试源
    // std::string video_source = "./sample_720p.mp4"; // 默认使用测试源
    std::string video_source = "./test.webm"; // 默认使用测试源

    if (argc > 1)
    {
        video_source = argv[1];
    }

    std::cout << "Starting GStreamer + OpenGL video player..." << std::endl;
    std::cout << "Video source: " << video_source << std::endl;

    GstOpenGLPlayer player(1280, 720);

    if (!player.initialize(video_source))
    {
        std::cerr << "Failed to initialize player" << std::endl;
        return 1;
    }

    player.run();

    std::cout << "Player stopped" << std::endl;
    return 0;
}