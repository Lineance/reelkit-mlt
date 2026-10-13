#include "test_support.hpp"

#include <iostream>

int main(int argc, char **argv)
{
    try {
        mlt_test::Runtime runtime;
        Mlt::Profile profile;
        mlt_test::configureProfile(profile);
        const auto output = mlt_test::outputDirectory(argc, argv, "timeline");
        constexpr int clipFrames = 90;
        constexpr int dissolveFrames = 15;
        constexpr int expectedFrames = clipFrames * 2 - dissolveFrames;

        Mlt::Producer red(profile, "color:red");
        Mlt::Producer blue(profile, "color:blue");
        mlt_test::require(red.is_valid() && blue.is_valid(), "Color producers unavailable");
        red.set("out", clipFrames - 1);
        blue.set("out", clipFrames - 1);

        Mlt::Playlist playlist(profile);
        playlist.append(red, 10, clipFrames + 9);
        playlist.append(blue, 0, clipFrames - 1);
        Mlt::Transition dissolve(profile, "luma");
        mlt_test::require(dissolve.is_valid(), "Luma transition unavailable");
        mlt_test::require(playlist.mix(0, dissolveFrames, &dissolve) == 0,
                          "Could not add dissolve");
        playlist.optimise();
        mlt_test::require(playlist.get_playtime() == expectedFrames,
                          "Unexpected timeline duration after source trims and dissolve");

        const auto mltPath = output / "timeline.mlt";
        const auto renderPath = output / "timeline.mp4";
        const auto reopenedPath = output / "timeline-xml-roundtrip.mp4";
        mlt_test::writeMlt(profile, playlist, mltPath);
        mlt_test::renderVideo(profile, playlist, renderPath);

        Mlt::Producer reopened(profile, mltPath.c_str());
        mlt_test::require(reopened.is_valid(), "Serialized MLT project did not reopen");
        mlt_test::require(reopened.get_playtime() == expectedFrames,
                          "Reopened MLT project changed the timeline duration");
        mlt_test::renderVideo(profile, reopened, reopenedPath);

        std::cout << "timeline: source trims + " << dissolveFrames << "-frame dissolve -> "
                  << expectedFrames << " frames; MLT XML reopened\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "timeline test failed: " << error.what() << '\n';
        return 1;
    }
}
