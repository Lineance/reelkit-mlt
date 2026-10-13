#include "test_support.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

int main(int argc, char **argv)
{
    try {
        mlt_test::Runtime runtime;
        Mlt::Profile profile;
        mlt_test::configureProfile(profile);
        const auto output = mlt_test::outputDirectory(argc, argv, "text-keyframes");
        constexpr int frames = 90;

        Mlt::Producer background(profile, "color:blue");
        mlt_test::require(background.is_valid(), "Background producer unavailable");
        background.set("out", frames - 1);
        Mlt::Playlist track(profile);
        track.append(background, 0, frames - 1);

        Mlt::Filter title(profile, "qtext", "");
        mlt_test::require(title.is_valid(), "MLT qtext filter unavailable");
        title.set("argument", "Animated title");
        title.set("fgcolour", "0xffffffff");
        title.set("bgcolour", "0x00000000");
        title.set_in_and_out(0, frames - 1);
        title.anim_set("opacity", 0.0, 0, frames);
        title.anim_set("opacity", 1.0, 20, frames);
        mlt_test::require(title.is_anim("opacity"), "Opacity keyframes were not stored");
        mlt_test::require(std::abs(title.anim_get_double("opacity", 0, frames)) < 0.01,
                          "Opacity keyframe start value is incorrect");
        mlt_test::require(std::abs(title.anim_get_double("opacity", 20, frames) - 1.0) < 0.01,
                          "Opacity keyframe end value is incorrect");
        track.attach(title);

        Mlt::Tractor graph(profile);
        graph.set_track(track, 0);
        graph.refresh();
        mlt_test::require(graph.get_playtime() == frames, "Text animation changed timeline duration");
        const auto outputPath = output / "animated-title.mp4";
        const auto mltPath = output / "animated-title.mlt";
        mlt_test::writeMlt(profile, graph, mltPath);
        mlt_test::renderVideo(profile, graph, outputPath);

        Mlt::Producer reopened(profile, mltPath.c_str());
        mlt_test::require(reopened.is_valid(), "Animated title MLT project did not reopen");
        mlt_test::require(reopened.get_playtime() == frames,
                          "Reopened text animation changed timeline duration");
        std::ifstream xmlFile(mltPath);
        const std::string xml((std::istreambuf_iterator<char>(xmlFile)),
                              std::istreambuf_iterator<char>());
        mlt_test::require(xml.find("name=\"opacity\">0=0;20=1</property>") != std::string::npos,
                          "Serialized MLT project omitted the opacity keyframes");

        std::cout << "text-keyframes: qtext opacity 0->1 over 20 frames\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "text keyframe test failed: " << error.what() << '\n';
        return 1;
    }
}
