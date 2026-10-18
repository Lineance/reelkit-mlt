#include <editor_core/editor_core.hpp>

#include <iostream>

int main()
{
    try {
        editor_core::Runtime runtime;
        editor_core::ProfileSpec settings;
        settings.width = 640;
        settings.height = 360;
        settings.frameRate = {30, 1};
        editor_core::Profile profile(runtime, settings);
        editor_core::Timeline timeline(profile);
        auto video = timeline.addTrack(editor_core::TrackKind::Video, "consumer-test");

        Mlt::Producer source(profile.native(), "color:red");
        if (!source.is_valid())
            return 2;
        video.append(source, editor_core::FrameRange(0, 30));
        if (timeline.durationFrames() != 30)
            return 3;

        std::cout << "downstream CMake consumer linked editor_core successfully\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
