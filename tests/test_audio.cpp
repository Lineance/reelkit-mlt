#include "test_support.hpp"

#include <iostream>

int main(int argc, char **argv)
{
    try {
        mlt_test::Runtime runtime;
        Mlt::Profile profile;
        mlt_test::configureProfile(profile);
        const auto output = mlt_test::outputDirectory(argc, argv, "audio");
        constexpr int frames = 180;

        Mlt::Producer fullTone(profile, "tone", "");
        mlt_test::require(fullTone.is_valid(), "MLT full-level tone producer unavailable");
        fullTone.set("frequency", 440.0);
        fullTone.set("level", 0.0);
        fullTone.set("out", frames - 1);
        mlt_test::renderAudio(profile, fullTone, output / "tone-level-0.wav");

        Mlt::Producer sampleTone(profile, "tone", "");
        mlt_test::require(sampleTone.is_valid(), "MLT tone producer unavailable");
        sampleTone.set("frequency", 440.0);
        sampleTone.set("level", -24.0);
        sampleTone.set("out", frames - 1);
        mlt_test::renderAudio(profile, sampleTone, output / "tone-level-minus24.wav");

        Mlt::Producer low(profile, "tone", "");
        Mlt::Producer high(profile, "tone", "");
        mlt_test::require(low.is_valid() && high.is_valid(), "MLT tone producer unavailable");
        low.set("frequency", 440.0);
        low.set("level", -24.0);
        low.set("out", frames - 1);
        high.set("frequency", 660.0);
        high.set("level", -24.0);
        high.set("out", frames - 1);

        Mlt::Tractor mixGraph(profile);
        mixGraph.set_track(low, 0);
        mixGraph.set_track(high, 1);
        Mlt::Transition mix(profile, "mix");
        mlt_test::require(mix.is_valid(), "MLT audio mix transition unavailable");
        mix.set_in_and_out(0, frames - 1);
        mix.set("start", 0.0);
        mix.set("end", 0.5);
        mixGraph.plant_transition(mix, 0, 1);
        mixGraph.refresh();
        mlt_test::require(mixGraph.get_playtime() == frames, "Unexpected audio timeline duration");

        const auto wavPath = output / "mixed-tones.wav";
        mlt_test::writeMlt(profile, mixGraph, output / "mixed-tones.mlt");
        mlt_test::renderAudio(profile, mixGraph, wavPath);

        std::cout << "audio: mixed 440/660 Hz tracks -> " << frames << " frames at 30 fps\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "audio test failed: " << error.what() << '\n';
        return 1;
    }
}
