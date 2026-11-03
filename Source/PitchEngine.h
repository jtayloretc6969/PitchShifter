#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

// Two overlapping, complementary raised-cosine grains. All storage is allocated
// in prepare(); process() is bounded and performs no allocation or locking.
class PitchEngine {
public:
    void prepare(double sampleRate) {
        sr = sampleRate;
        latency = static_cast<int>(std::lround(sr * 0.016));
        for (auto& channel : ring) channel.assign(static_cast<size_t>(std::ceil(sr * 0.1)) + 8, 0.0f);
        transientRing.assign(ring[0].size(), 0.0f);
        reset();
    }
    void reset() {
        for (auto& channel : ring) std::fill(channel.begin(), channel.end(), 0.0f);
        std::fill(transientRing.begin(), transientRing.end(), 0.0f);
        position = 0; phase = 0; ratio = 1; mix = 1; gain = 1; envelope = 0; attack = 0;
        window = sr * 0.026; previous = 0; sinceCrossing = 0; period = 0;
    }
    int getLatency() const { return latency; }
    // mode: 0 Tone, 1 Beat, 2 Hybrid. Hybrid returns transient energy to
    // the latency-aligned dry path; it does not claim source separation.
    void process(float* const* channels, int count, int samples, float semitones,
                 int mode, float wet, float gainDb) {
        if (ring[0].empty()) return;
        count = std::clamp(count, 1, 2);
        const double targetRatio = std::pow(2.0, std::clamp(semitones, -24.0f, 24.0f) / 12.0);
        const double targetWindow = sr * (mode == 1 ? 0.008 : mode == 2 ? 0.018 : 0.026);
        const double smooth = 1.0 - std::exp(-1.0 / (sr * 0.015));
        const double release = std::exp(-1.0 / (sr * 0.025));
        const float targetGain = std::pow(10.0f, std::clamp(gainDb, -24.0f, 12.0f) / 20.0f);
        for (int i = 0; i < samples; ++i) {
            ratio += smooth * (targetRatio - ratio);
            mix += smooth * (std::clamp(wet, 0.0f, 1.0f) - mix);
            gain += smooth * (targetGain - gain);
            // Align grain spacing to a measured tonal period. This avoids the
            // severe comb cancellation fixed-length grains can cause on flute.
            const float input = channels[0][i];
            sinceCrossing += 1;
            if (previous <= 0 && input > 0) {
                const double crossing = input / (input - previous);
                const double measured = sinceCrossing - crossing;
                if (measured > sr / 2500 && measured < sr / 65)
                    period = period <= 0 ? measured : period + 0.2 * (measured - period);
                sinceCrossing = crossing;
            }
            previous = input;
            if (sinceCrossing > sr * 0.05) period = 0;
            double desired = targetWindow;
            if (period > 0 && period * 2 < latency * 1.9) {
                const double cycles = std::max(1.0, std::min(std::round(targetWindow / (2 * period)), std::floor(latency * 1.9 / (2 * period))));
                desired = 2 * cycles * period;
            }
            window += smooth * (desired - window);
            float peak = 0;
            for (int ch = 0; ch < count; ++ch) {
                ring[static_cast<size_t>(ch)][position] = channels[ch][i];
                peak = std::max(peak, std::abs(channels[ch][i]));
            }
            const double novelty = std::max(0.0, peak - envelope * 1.6);
            envelope = std::max(static_cast<double>(peak), envelope * release);
            const float transient = static_cast<float>(std::clamp(novelty * 4.0, 0.0, 1.0));
            transientRing[position] = transient;
            // Read transient control at the same fixed latency as the dry audio.
            const auto controlIndex = (position + ring[0].size() - static_cast<size_t>(latency)) % ring[0].size();
            attack = std::max(static_cast<double>(transientRing[controlIndex]), attack * release);
            const double p2 = phase < 0.5 ? phase + 0.5 : phase - 0.5;
            constexpr double pi = 3.14159265358979323846;
            const double weight = 0.5 - 0.5 * std::cos(2.0 * pi * phase);
            const double delay1 = latency + (phase - 0.5) * window;
            const double delay2 = latency + (p2 - 0.5) * window;
            for (int ch = 0; ch < count; ++ch) {
                const float dry = read(ch, latency);
                float shifted = static_cast<float>(weight * read(ch, delay1) + (1.0 - weight) * read(ch, delay2));
                // At unity, provide an exact delayed copy, without comb filtering.
                const double shiftAmount = std::min(1.0, std::abs(ratio - 1.0) * 100.0);
                shifted = static_cast<float>(dry + shiftAmount * (shifted - dry));
                const double effectiveWet = mix * (mode == 2 ? 1.0 - 0.65 * attack : 1.0);
                channels[ch][i] = static_cast<float>((dry + effectiveWet * (shifted - dry)) * gain);
            }
            phase += (1.0 - ratio) / window;
            phase -= std::floor(phase);
            position = (position + 1) % ring[0].size();
        }
    }
private:
    float read(int ch, double delay) const {
        double index = static_cast<double>(position) - delay;
        if (index < 0) index += static_cast<double>(ring[static_cast<size_t>(ch)].size());
        const auto a = static_cast<size_t>(index);
        const auto b = (a + 1) % ring[static_cast<size_t>(ch)].size();
        const auto fraction = static_cast<float>(index - static_cast<double>(a));
        return ring[static_cast<size_t>(ch)][a] + fraction * (ring[static_cast<size_t>(ch)][b] - ring[static_cast<size_t>(ch)][a]);
    }
    std::array<std::vector<float>, 2> ring;
    std::vector<float> transientRing;
    size_t position = 0;
    int latency = 768;
    double sr = 48000, phase = 0, ratio = 1, mix = 1, gain = 1, window = 1248;
    double envelope = 0, attack = 0;
    double previous = 0, sinceCrossing = 0, period = 0;
};
