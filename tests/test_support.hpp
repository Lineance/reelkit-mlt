#pragma once

#include <mlt++/Mlt.h>

#include <filesystem>
#include <stdexcept>
#include <string>

namespace mlt_test {

class Runtime {
public:
    Runtime()
    {
        if (Mlt::Factory::init() == nullptr)
            throw std::runtime_error("MLT factory initialization failed");
    }

    ~Runtime() { Mlt::Factory::close(); }

    Runtime(const Runtime &) = delete;
    Runtime &operator=(const Runtime &) = delete;
};

inline void require(bool condition, const std::string &message)
{
    if (!condition)
        throw std::runtime_error(message);
}

inline void configureProfile(Mlt::Profile &profile)
{
    profile.set_width(640);
    profile.set_height(360);
    profile.set_frame_rate(30, 1);
    profile.set_sample_aspect(1, 1);
    profile.set_display_aspect(16, 9);
    profile.set_progressive(1);
    profile.set_explicit(1);
}

inline std::filesystem::path outputDirectory(int argc, char **argv, const char *scenario)
{
    const auto root = argc > 1 ? std::filesystem::path(argv[1])
                               : std::filesystem::path("test-artifacts");
    const auto directory = root / scenario;
    std::filesystem::create_directories(directory);
    return directory;
}

inline void configureVideoConsumer(Mlt::Consumer &consumer)
{
    consumer.set("an", 1);
    consumer.set("audio_off", 1);
    consumer.set("vcodec", "libx264");
    consumer.set("pix_fmt", "yuv420p");
    consumer.set("crf", 23);
    consumer.set("preset", "ultrafast");
    consumer.set("real_time", -1);
}

inline void renderVideo(Mlt::Profile &profile,
                        Mlt::Service &graph,
                        const std::filesystem::path &path)
{
    Mlt::Consumer consumer(profile, "avformat", path.c_str());
    require(consumer.is_valid(), "MLT avformat consumer unavailable");
    configureVideoConsumer(consumer);
    require(consumer.connect(graph) == 0, "Could not connect video graph to consumer");
    require(consumer.run() == 0, "MLT video rendering failed");
    require(std::filesystem::is_regular_file(path) && std::filesystem::file_size(path) > 0,
            "Video output was not created");
}

inline void configureAudioConsumer(Mlt::Consumer &consumer,
                                  const char *codec = "pcm_s16le",
                                  const char *sampleFormat = "s16",
                                  const char *container = "wav")
{
    consumer.set("vn", 1);
    consumer.set("an", 0);
    consumer.set("audio_off", 0);
    consumer.set("f", container);
    consumer.set("acodec", codec);
    consumer.set("mlt_audio_format", sampleFormat);
    consumer.set("ar", 48000);
    consumer.set("channels", 2);
    consumer.set("real_time", -1);
}

inline void renderAudio(Mlt::Profile &profile,
                        Mlt::Service &graph,
                        const std::filesystem::path &path,
                        const char *codec = "pcm_s16le",
                        const char *sampleFormat = "s16",
                        const char *container = "wav")
{
    Mlt::Consumer consumer(profile, "avformat", path.c_str());
    require(consumer.is_valid(), "MLT avformat audio consumer unavailable");
    configureAudioConsumer(consumer, codec, sampleFormat, container);
    require(consumer.connect(graph) == 0, "Could not connect audio graph to consumer");
    require(consumer.run() == 0, "MLT audio rendering failed");
    require(std::filesystem::is_regular_file(path) && std::filesystem::file_size(path) > 0,
            "Audio output was not created");
}

inline void writeMlt(Mlt::Profile &profile,
                     Mlt::Service &graph,
                     const std::filesystem::path &path)
{
    Mlt::Consumer consumer(profile, "xml", path.c_str());
    require(consumer.is_valid(), "MLT XML consumer unavailable");
    require(consumer.connect(graph) == 0, "Could not connect graph to XML consumer");
    require(consumer.run() == 0, "MLT XML serialization failed");
    require(std::filesystem::is_regular_file(path) && std::filesystem::file_size(path) > 0,
            "MLT XML output was not created");
}

} // namespace mlt_test
