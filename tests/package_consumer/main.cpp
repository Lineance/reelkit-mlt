#include <reelkit/mlt/reelkit_mlt.hpp>

#include <iostream>

int main()
{
    try {
        reelkit::mlt::Runtime runtime;
        reelkit::mlt::ProfileSpec settings;
        settings.width = 640;
        settings.height = 360;
        settings.frameRate = {30, 1};
        reelkit::mlt::Profile profile(runtime, settings);
        reelkit::mlt::Timeline timeline(profile);
        auto video = timeline.addTrack(reelkit::mlt::TrackKind::Video, "consumer-test");

        Mlt::Producer source(profile.native(), "color:red");
        if (!source.is_valid())
            return 2;
        video.append(source, reelkit::mlt::FrameRange(0, 30));
        if (timeline.durationFrames() != 30)
            return 3;

        std::cout << "downstream CMake consumer linked reelkit_mlt successfully\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
