#include <mlt++/Mlt.h>

#include <filesystem>
#include <iostream>
#include <string>

namespace {
constexpr int kFramesPerClip = 90;
constexpr int kTransitionFrames = 15;

void configureAvformat(Mlt::Consumer &consumer)
{
    consumer.set("an", 1);
    consumer.set("audio_off", 1);
    consumer.set("vcodec", "libx264");
    consumer.set("pix_fmt", "yuv420p");
    consumer.set("crf", 23);
    consumer.set("preset", "ultrafast");
    consumer.set("real_time", -1);
}

bool render(const std::string &mp4Path, const std::string &mltPath)
{
    if (!Mlt::Factory::init()) {
        std::cerr << "MLT factory initialization failed\n";
        return false;
    }

    bool ok = false;
    {
        Mlt::Profile profile;
        profile.set_width(640);
        profile.set_height(360);
        profile.set_frame_rate(30, 1);
        profile.set_sample_aspect(1, 1);
        profile.set_display_aspect(16, 9);
        profile.set_progressive(1);
        profile.set_explicit(1);

        Mlt::Producer red(profile, "color:red");
        Mlt::Producer blue(profile, "color:blue");
        if (!red.is_valid() || !blue.is_valid()) {
            std::cerr << "MLT color producer unavailable\n";
            return false;
        }

        red.set("out", kFramesPerClip - 1);
        blue.set("out", kFramesPerClip - 1);

        Mlt::Playlist video(profile);
        video.append(red, 0, kFramesPerClip - 1);
        video.append(blue, 0, kFramesPerClip - 1);

        Mlt::Transition dissolve(profile, "luma");
        if (!dissolve.is_valid() || video.mix(0, kTransitionFrames, &dissolve) != 0) {
            std::cerr << "Could not create the dissolve transition\n";
            return false;
        }
        video.optimise();

        const int duration = video.get_playtime();
        if (duration <= 0) {
            std::cerr << "MLT produced an empty timeline\n";
            return false;
        }

        Mlt::Filter title(profile, "qtext", "");
        if (!title.is_valid()) {
            std::cerr << "MLT qtext filter unavailable\n";
            return false;
        }
        title.set("argument", "MLT++ WSL render test");
        title.set("fgcolour", "0xffffffff");
        title.set("bgcolour", "0x00000000");
        title.set("size", 34);
        title.set("pad", 10);
        title.set_in_and_out(0, duration - 1);
        video.attach(title);

        Mlt::Tractor timeline(profile);
        timeline.set_track(video, 0);
        timeline.refresh();

        Mlt::Consumer xml(profile, "xml", mltPath.c_str());
        if (!xml.is_valid() || xml.connect(timeline) != 0 || xml.run() != 0) {
            std::cerr << "MLT XML serialization failed\n";
            return false;
        }

        Mlt::Consumer output(profile, "avformat", mp4Path.c_str());
        if (!output.is_valid()) {
            std::cerr << "MLT avformat consumer unavailable\n";
            return false;
        }
        configureAvformat(output);
        if (output.connect(timeline) != 0 || output.run() != 0) {
            std::cerr << "MLT avformat rendering failed\n";
            return false;
        }

        Mlt::Producer reopened(profile, mltPath.c_str());
        if (!reopened.is_valid()) {
            std::cerr << "Could not reopen the serialized MLT project\n";
            return false;
        }
        std::filesystem::path roundTripPath(mp4Path);
        roundTripPath.replace_filename(roundTripPath.stem().string() + "-xml-roundtrip.mp4");
        Mlt::Consumer roundTrip(profile, "avformat", roundTripPath.c_str());
        if (!roundTrip.is_valid()) {
            std::cerr << "MLT avformat consumer unavailable for XML round trip\n";
            return false;
        }
        configureAvformat(roundTrip);
        if (roundTrip.connect(reopened) != 0 || roundTrip.run() != 0) {
            std::cerr << "Rendering the reopened MLT project failed\n";
            return false;
        }

        std::cout << "Rendered " << duration << " frames at 30 fps and reopened the MLT XML\n";
        ok = true;
    }
    Mlt::Factory::close();
    return ok;
}
} // namespace

int main(int argc, char **argv)
{
    if (argc != 3) {
        std::cerr << "Usage: mlt_render_poc <output.mp4> <output.mlt>\n";
        return 2;
    }

    const std::filesystem::path mp4Path(argv[1]);
    const std::filesystem::path mltPath(argv[2]);
    std::filesystem::create_directories(mp4Path.parent_path());
    std::filesystem::create_directories(mltPath.parent_path());

    return render(mp4Path.string(), mltPath.string()) ? 0 : 1;
}
