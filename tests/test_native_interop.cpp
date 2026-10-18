#include "test_support.hpp"
#include <reelkit/mlt/reelkit_mlt.hpp>

#include <cmath>
#include <iostream>

int main(int argc, char **argv)
{
    try {
        reelkit::mlt::Runtime runtime;
        mlt_test::require(runtime.hasProducer("color") && runtime.hasFilter("qtext")
                              && runtime.hasTransition("luma") && runtime.hasConsumer("avformat"),
                          "Runtime service catalog omitted required MLT modules");
        reelkit::mlt::Profile profile(runtime, reelkit::mlt::ProfileSpec{
                                                  .width = 640,
                                                  .height = 360,
                                                  .frameRate = {30, 1},
                                                  .sampleAspectNumerator = 1,
                                                  .sampleAspectDenominator = 1,
                                                  .displayAspectNumerator = 16,
                                                  .displayAspectDenominator = 9,
                                                  .progressive = true,
                                                  .colorspace = std::nullopt,
                                              });
        const auto output = mlt_test::outputDirectory(argc, argv, "native-interop");
        reelkit::mlt::Timeline timeline(profile);
        auto video = timeline.addTrack(reelkit::mlt::TrackKind::Video, "main");

        Mlt::Producer red(profile.native(), "color:red");
        Mlt::Producer blue(profile.native(), "color:blue");
        mlt_test::require(red.is_valid() && blue.is_valid(), "Native MLT producers unavailable");
        red.set("out", 59);
        blue.set("out", 59);
        const reelkit::mlt::FrameRange sourceRange(0, 60);
        mlt_test::require(sourceRange.mltIn() == 0 && sourceRange.mltOut() == 59,
                          "Half-open frame range did not convert to MLT in/out frames");

        video.append(red, sourceRange);
        video.append(blue, sourceRange);
        video.dissolve(0, 10);
        mlt_test::require(video.native().count() == 3,
                          "Native Playlist does not reflect wrapper clip/transition edits");
        mlt_test::require(timeline.durationFrames() == 110,
                          "Wrapper timeline returned the wrong post-dissolve duration");

        Mlt::Filter rawTitle(profile.native(), "qtext", "");
        mlt_test::require(rawTitle.is_valid(), "Raw MLT qtext filter unavailable");
        rawTitle.set("argument", "Raw MLT++ call");
        rawTitle.set("fgcolour", "0xffffffff");
        rawTitle.set("bgcolour", "0x00000000");
        rawTitle.set_in_and_out(0, 109);
        mlt_test::require(video.native().attach(rawTitle) == 0,
                          "Could not attach a raw MLT filter through native()");

        auto helperTitle = video.addText(reelkit::mlt::TextSpec{
            .text = "reelkit-mlt helper",
            .family = "Sans",
            .foreground = "0xffffffff",
            .background = "0x00000000",
            .outlineColor = "0x00000000",
            .size = 24,
            .weight = 400,
            .outline = 0,
            .padding = 4,
            .opacity = 1.0,
        });
        mlt_test::require(helperTitle.is_valid() && video.native().filter_count() == 2,
                          "High-level text helper did not mutate the same native Playlist");
        video.native().set("meta.reelkit_interop", "shared-graph");
        mlt_test::require(std::string(video.native().get("meta.reelkit_interop"))
                              == "shared-graph",
                          "Raw MLT property mutation was not visible through native()");

        const auto mltPath = output / "mixed-api.mlt";
        const auto mp4Path = output / "mixed-api.mp4";
        const auto reloadedPath = output / "mixed-api-reloaded.mp4";
        timeline.saveMltXml(mltPath);
        auto renderSpec = reelkit::mlt::RenderSpec::mp4(mp4Path);
        renderSpec.enableAudio = false;
        const auto rendered = timeline.render(renderSpec);
        mlt_test::require(rendered.frames == 110, "RenderResult has the wrong timeline duration");
        mlt_test::require(rendered.frameRate.numerator == 30 && rendered.frameRate.denominator == 1,
                          "RenderResult has the wrong frame rate");

        auto document = reelkit::mlt::MltDocument::load(profile, mltPath);
        mlt_test::require(document.durationFrames() == 110,
                          "Loaded MLT document changed the timeline duration");
        auto reloadedSpec = reelkit::mlt::RenderSpec::mp4(reloadedPath);
        reloadedSpec.enableAudio = false;
        const auto reloaded = document.render(reloadedSpec);
        mlt_test::require(reloaded.frames == rendered.frames,
                          "Rendered MLT XML snapshot changed timeline duration");

        std::cout << "native interop: raw MLT++ and convenience calls share one graph; XML reload passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "native interop test failed: " << error.what() << '\n';
        return 1;
    }
}
