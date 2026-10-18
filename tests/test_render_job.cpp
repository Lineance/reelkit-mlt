#include "test_support.hpp"
#include <editor_core/editor_core.hpp>

#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char **argv)
{
    try {
        editor_core::Runtime runtime;
        editor_core::ProfileSpec settings;
        settings.width = 640;
        settings.height = 360;
        settings.frameRate = {30, 1};
        editor_core::Profile profile(runtime, settings);
        const auto output = mlt_test::outputDirectory(argc, argv, "render-job");

        editor_core::Timeline timeline(profile);
        auto video = timeline.addTrack(editor_core::TrackKind::Video, "long synthetic source");
        Mlt::Producer color(profile.native(), "color:green");
        mlt_test::require(color.is_valid(), "Color producer unavailable");
        color.set("out", 2999);
        video.append(color, editor_core::FrameRange(0, 3000));

        auto spec = editor_core::RenderSpec::mp4(output / "cancelled.mp4");
        spec.enableAudio = false;
        spec.consumerProperties["real_time"] = "1";
        auto job = timeline.startRender(spec);
        job.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        mlt_test::require(job.isRunning(), "Long render stopped before cancellation was requested");
        const auto progress = job.progress();
        mlt_test::require(progress.totalFrames == 3000 && progress.fraction >= 0.0
                              && progress.fraction <= 1.0,
                          "RenderJob progress is outside its expected range");
        job.cancel();
        const auto result = job.wait();
        mlt_test::require(result.cancelled, "RenderJob did not report cancellation");
        mlt_test::require(!job.isRunning(), "Cancelled RenderJob is still running");

        std::cout << "render-job: progress observed and cancellation completed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "render job test failed: " << error.what() << '\n';
        return 1;
    }
}
