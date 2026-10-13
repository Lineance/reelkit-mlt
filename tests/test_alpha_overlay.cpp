#include "test_support.hpp"

#include <iostream>

int main(int argc, char **argv)
{
    try {
        mlt_test::Runtime runtime;
        Mlt::Profile profile;
        mlt_test::configureProfile(profile);
        const auto output = mlt_test::outputDirectory(argc, argv, "alpha-overlay");
        constexpr int frames = 60;

        Mlt::Producer background(profile, "color:red");
        mlt_test::require(background.is_valid(), "Background producer unavailable");
        background.set("out", frames - 1);
        Mlt::Playlist track(profile);
        track.append(background, 0, frames - 1);

        Mlt::Filter overlay(profile, "qtext", "");
        mlt_test::require(overlay.is_valid(), "MLT qtext filter unavailable");
        overlay.set("argument", "Alpha overlay");
        overlay.set("fgcolour", "0x00ff0080");
        overlay.set("bgcolour", "0x00000000");
        overlay.set("opacity", 1.0);
        overlay.set_in_and_out(0, frames - 1);
        track.attach(overlay);

        Mlt::Tractor graph(profile);
        graph.set_track(track, 0);
        graph.refresh();
        mlt_test::require(graph.get_playtime() == frames, "Overlay changed timeline duration");
        const auto outputPath = output / "alpha-overlay.mp4";
        const auto mltPath = output / "alpha-overlay.mlt";
        mlt_test::writeMlt(profile, graph, mltPath);
        mlt_test::renderVideo(profile, graph, outputPath);

        Mlt::Producer reopened(profile, mltPath.c_str());
        mlt_test::require(reopened.is_valid(), "Alpha overlay MLT project did not reopen");
        mlt_test::require(reopened.get_playtime() == frames,
                          "Reopened alpha overlay changed timeline duration");
        mlt_test::require(std::filesystem::file_size(outputPath) > 0,
                          "Alpha overlay render is empty");

        std::cout << "alpha-overlay: semi-transparent qtext over a color producer\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "alpha overlay test failed: " << error.what() << '\n';
        return 1;
    }
}
