#include "test_support.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

namespace {
struct AudioMetrics {
    double peak = 0.0;
    double rms = 0.0;
};

uint16_t readLe16(const unsigned char *data)
{
    return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

uint32_t readLe32(const unsigned char *data)
{
    return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8)
           | (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
}

AudioMetrics readFloatWavMetrics(const std::filesystem::path &path)
{
    std::ifstream input(path, std::ios::binary);
    mlt_test::require(input.good(), "Could not open float WAV output");
    char riff[4];
    char wave[4];
    uint32_t ignored = 0;
    input.read(riff, 4);
    input.read(reinterpret_cast<char *>(&ignored), sizeof(ignored));
    input.read(wave, 4);
    mlt_test::require(input.good() && std::string(riff, 4) == "RIFF"
                          && std::string(wave, 4) == "WAVE",
                      "Output is not a RIFF/WAVE file");

    uint16_t audioFormat = 0;
    uint16_t channels = 0;
    uint16_t bitsPerSample = 0;
    uint32_t sampleRate = 0;
    bool floatFormat = false;
    std::vector<unsigned char> samples;
    char chunkId[4];
    while (input.read(chunkId, 4)) {
        unsigned char sizeBytes[4];
        input.read(reinterpret_cast<char *>(sizeBytes), 4);
        if (!input)
            break;
        const uint32_t size = readLe32(sizeBytes);
        if (std::string(chunkId, 4) == "fmt ") {
            std::vector<unsigned char> formatChunk(size);
            input.read(reinterpret_cast<char *>(formatChunk.data()), size);
            if (size >= 16) {
                audioFormat = readLe16(formatChunk.data());
                channels = readLe16(formatChunk.data() + 2);
                sampleRate = readLe32(formatChunk.data() + 4);
                bitsPerSample = readLe16(formatChunk.data() + 14);
                floatFormat = audioFormat == 3
                              || (audioFormat == 0xfffe && size >= 40
                                  && readLe16(formatChunk.data() + 24) == 3);
            }
        } else if (std::string(chunkId, 4) == "data") {
            samples.resize(size);
            input.read(reinterpret_cast<char *>(samples.data()), size);
        } else {
            input.seekg(size, std::ios::cur);
        }
        if (size & 1)
            input.seekg(1, std::ios::cur);
    }

    mlt_test::require(floatFormat && channels == 2 && sampleRate == 48000
                          && bitsPerSample == 32 && samples.size() >= sizeof(float),
                      "WAV output is not 48 kHz stereo float PCM");
    double sumSquares = 0.0;
    size_t count = 0;
    AudioMetrics result;
    for (size_t offset = 0; offset + sizeof(float) <= samples.size(); offset += sizeof(float)) {
        float value = 0.0f;
        std::memcpy(&value, samples.data() + offset, sizeof(value));
        const double magnitude = std::abs(static_cast<double>(value));
        result.peak = std::max(result.peak, magnitude);
        sumSquares += magnitude * magnitude;
        ++count;
    }
    result.rms = std::sqrt(sumSquares / static_cast<double>(count));
    return result;
}

double toneFramePeak(Mlt::Profile &profile, double level, int frames)
{
    Mlt::Producer tone(profile, "tone", "");
    mlt_test::require(tone.is_valid(), "MLT tone producer unavailable for PCM inspection");
    tone.set("frequency", 440.0);
    tone.set("level", level);
    tone.set("out", frames - 1);
    tone.seek(0);

    Mlt::Frame *frame = tone.get_frame();
    mlt_test::require(frame != nullptr, "MLT tone producer returned no frame");
    mlt_audio_format format = mlt_audio_float;
    int frequency = 48000;
    int channels = 2;
    int samples = 0;
    auto *audio = static_cast<float *>(frame->get_audio(format, frequency, channels, samples));
    mlt_test::require(audio != nullptr && format == mlt_audio_float && samples > 0,
                      "Could not read float PCM from MLT tone frame");

    double peak = 0.0;
    for (int i = 0; i < samples * channels; ++i)
        peak = std::max(peak, std::abs(static_cast<double>(audio[i])));
    delete frame;
    return peak;
}
} // namespace

int main(int argc, char **argv)
{
    try {
        mlt_test::Runtime runtime;
        Mlt::Profile profile;
        mlt_test::configureProfile(profile);
        const auto output = mlt_test::outputDirectory(argc, argv, "audio");
        constexpr int frames = 180;

        const double fullProducerPeak = toneFramePeak(profile, 0.0, frames);
        const double attenuatedProducerPeak = toneFramePeak(profile, -24.0, frames);
        const double expectedRatio = std::pow(10.0, -24.0 / 20.0);
        mlt_test::require(fullProducerPeak > 0.9, "Full-level tone producer is unexpectedly quiet");
        mlt_test::require(std::abs((attenuatedProducerPeak / fullProducerPeak) - expectedRatio) < 0.01,
                          "Tone producer level does not scale its PCM by the expected dB ratio");

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
        mlt_test::require(std::abs(sampleTone.get_double("level") + 24.0) < 0.01,
                          "The producer did not retain its level property");
        mlt_test::require(std::abs(sampleTone.anim_get_double("level", 0, frames) + 24.0) < 0.01,
                          "The producer animation API returned a different level");
        mlt_test::renderAudio(profile, sampleTone, output / "tone-level-minus24.wav");

        Mlt::Producer floatTone(profile, "tone", "");
        mlt_test::require(floatTone.is_valid(), "MLT float tone producer unavailable");
        floatTone.set("frequency", 440.0);
        floatTone.set("level", -24.0);
        floatTone.set("out", frames - 1);
        mlt_test::renderAudio(profile,
                              floatTone,
                              output / "tone-level-minus24-float.wav",
                              "pcm_f32le",
                              "f32le");
        const AudioMetrics singleFloatMetrics
            = readFloatWavMetrics(output / "tone-level-minus24-float.wav");
        const double expectedPeak = std::pow(10.0, -24.0 / 20.0);
        mlt_test::require(std::abs(singleFloatMetrics.peak - expectedPeak) < 0.003,
                          "Float WAV consumer changed the tone's -24 dB peak");
        mlt_test::require(std::abs(singleFloatMetrics.rms - expectedPeak / std::sqrt(2.0)) < 0.003,
                          "Float WAV consumer changed the tone's RMS level");

        Mlt::Producer convertedTone(profile, "tone", "");
        mlt_test::require(convertedTone.is_valid(), "MLT converted tone producer unavailable");
        convertedTone.set("frequency", 440.0);
        convertedTone.set("level", -24.0);
        convertedTone.set("out", frames - 1);
        mlt_test::renderAudio(profile,
                              convertedTone,
                              output / "tone-level-minus24-float-to-s16.wav",
                              "pcm_s16le",
                              "f32le");

        Mlt::Producer aacTone(profile, "tone", "");
        mlt_test::require(aacTone.is_valid(), "MLT AAC tone producer unavailable");
        aacTone.set("frequency", 440.0);
        aacTone.set("level", -24.0);
        aacTone.set("out", frames - 1);
        mlt_test::renderAudio(profile,
                              aacTone,
                              output / "tone-level-minus24.m4a",
                              "aac",
                              "f32le",
                              "ipod");

        Mlt::Producer animatedTone(profile, "tone", "");
        mlt_test::require(animatedTone.is_valid(), "MLT animated tone producer unavailable");
        animatedTone.set("frequency", 440.0);
        animatedTone.set("out", frames - 1);
        animatedTone.anim_set("level", -24.0, 0, frames);
        mlt_test::require(animatedTone.is_anim("level"), "Tone level animation was not stored");
        mlt_test::require(std::abs(animatedTone.anim_get_double("level", 0, frames) + 24.0) < 0.01,
                          "Tone level animation returned a different value");
        mlt_test::renderAudio(profile, animatedTone, output / "tone-animated-level.wav");

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

        Mlt::Producer lowFloatMix(profile, "tone", "");
        Mlt::Producer highFloatMix(profile, "tone", "");
        mlt_test::require(lowFloatMix.is_valid() && highFloatMix.is_valid(),
                          "MLT float mix tone producers unavailable");
        lowFloatMix.set("frequency", 440.0);
        lowFloatMix.set("level", -24.0);
        lowFloatMix.set("out", frames - 1);
        highFloatMix.set("frequency", 660.0);
        highFloatMix.set("level", -24.0);
        highFloatMix.set("out", frames - 1);
        Mlt::Tractor floatMixGraph(profile);
        floatMixGraph.set_track(lowFloatMix, 0);
        floatMixGraph.set_track(highFloatMix, 1);
        Mlt::Transition floatMix(profile, "mix");
        mlt_test::require(floatMix.is_valid(), "MLT float mix transition unavailable");
        floatMix.set_in_and_out(0, frames - 1);
        floatMix.set("start", 0.0);
        floatMix.set("end", 0.5);
        floatMixGraph.plant_transition(floatMix, 0, 1);
        floatMixGraph.refresh();
        const auto floatMixPath = output / "mixed-tones-float.wav";
        mlt_test::renderAudio(profile, floatMixGraph, floatMixPath, "pcm_f32le", "f32le");
        const AudioMetrics mixMetrics = readFloatWavMetrics(floatMixPath);
        mlt_test::require(mixMetrics.peak > 0.02 && mixMetrics.peak < 0.10,
                          "Float mixed-tone output is silent or unexpectedly clipped");
        mlt_test::require(mixMetrics.rms > 0.01 && mixMetrics.rms < 0.08,
                          "Float mixed-tone RMS is outside the expected range");

        Mlt::Producer lowAac(profile, "tone", "");
        Mlt::Producer highAac(profile, "tone", "");
        mlt_test::require(lowAac.is_valid() && highAac.is_valid(),
                          "MLT AAC mix tone producers unavailable");
        lowAac.set("frequency", 440.0);
        lowAac.set("level", -24.0);
        lowAac.set("out", frames - 1);
        highAac.set("frequency", 660.0);
        highAac.set("level", -24.0);
        highAac.set("out", frames - 1);
        Mlt::Tractor aacGraph(profile);
        aacGraph.set_track(lowAac, 0);
        aacGraph.set_track(highAac, 1);
        Mlt::Transition aacMix(profile, "mix");
        mlt_test::require(aacMix.is_valid(), "MLT AAC mix transition unavailable");
        aacMix.set_in_and_out(0, frames - 1);
        aacMix.set("start", 0.0);
        aacMix.set("end", 0.5);
        aacGraph.plant_transition(aacMix, 0, 1);
        aacGraph.refresh();
        mlt_test::require(aacGraph.get_playtime() == frames,
                          "AAC audio timeline duration changed");
        mlt_test::renderAudio(profile,
                              aacGraph,
                              output / "mixed-tones.m4a",
                              "aac",
                              "f32le",
                              "ipod");

        std::cout << "audio: mixed 440/660 Hz tracks -> " << frames << " frames at 30 fps\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "audio test failed: " << error.what() << '\n';
        return 1;
    }
}
