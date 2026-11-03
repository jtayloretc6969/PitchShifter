#include "PitchEngine.h"
#include <cstdlib>
#include <iostream>
#include <random>
#include <chrono>
static constexpr double pi = 3.14159265358979323846;
static void require(bool ok, const char* message) { if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); } }
static void run(PitchEngine& engine, std::vector<float>& a, std::vector<float>& b, float pitch, int mode, int block = 127, float wet = 1) {
    for (size_t offset = 0; offset < a.size(); offset += static_cast<size_t>(block)) {
        float* ptrs[] = {a.data() + offset, b.data() + offset};
        engine.process(ptrs, 2, std::min(block, static_cast<int>(a.size() - offset)), pitch, mode, wet, 0);
    }
}
static double magnitude(const std::vector<float>& data, double freq, double sr) {
    double re = 0, im = 0;
    for (size_t i = data.size() / 2; i < data.size(); ++i) {
        const double angle = 2 * pi * freq * static_cast<double>(i) / sr;
        re += data[i] * std::cos(angle); im += data[i] * std::sin(angle);
    }
    return std::hypot(re, im) / (data.size() / 2);
}
int main() {
    for (double sr : {44100.0, 48000.0, 96000.0}) {
        PitchEngine engine; engine.prepare(sr);
        std::vector<float> original(12000), a, b;
        std::mt19937 random(42); std::uniform_real_distribution<float> noise(-0.5f, 0.5f);
        for (auto& v : original) v = noise(random);
        a = original; b = original; run(engine, a, b, 0, 2);
        for (size_t i = static_cast<size_t>(engine.getLatency()); i < a.size(); ++i) {
            require(std::abs(a[i] - original[i - static_cast<size_t>(engine.getLatency())]) < 1e-6, "unity latency-aligned transparency");
            require(a[i] == b[i], "stereo coherence");
        }
        engine.reset(); a = original; b = original; run(engine, a, b, 0, 2);
        engine.reset(); auto c = original, d = original; run(engine, c, d, 0, 2, 512);
        require(a == c, "block partition invariance");
        engine.reset(); a = original; b = original; run(engine, a, b, 7, 0, 63);
        engine.reset(); c = original; d = original; run(engine, c, d, 7, 0, 512);
        require(a == c, "shifted block partition invariance");
        for (int mode = 0; mode < 3; ++mode) for (float pitch : {-24.0f, -12.0f, -7.0f, 7.0f, 12.0f, 24.0f}) {
            engine.prepare(sr); a.resize(static_cast<size_t>(sr));
            for (size_t i = 0; i < a.size(); ++i) a[i] = static_cast<float>(0.2 * std::sin(2 * pi * 440 * static_cast<double>(i) / sr));
            b = a; run(engine, a, b, pitch, mode);
            double expected = 440 * std::pow(2.0, pitch / 12.0);
            const double target = magnitude(a, expected, sr);
            require(target > 0.035, "shifted fundamental retained");
            require(target > magnitude(a, expected * 1.03, sr) * 2, "pitch accuracy within three percent");
            for (size_t i = 0; i < a.size(); ++i) { require(std::isfinite(a[i]) && std::abs(a[i]) <= 0.201f, "finite bounded audio"); require(a[i] == b[i], "shifted stereo coherence"); }
        }
        std::cout << "PASS: unity, reset, stereo, partitioning, and 18 pitch/mode combinations at " << sr << " Hz\n";
    }
    // Noise and rapid control changes stress grain wraps and mode switching.
    PitchEngine stress; stress.prepare(48000);
    std::mt19937 rng(7); std::uniform_real_distribution<float> noise(-0.9f, 0.9f);
    std::vector<float> left(256), right(256);
    for (int block = 0; block < 2000; ++block) {
        for (size_t i = 0; i < left.size(); ++i) left[i] = right[i] = noise(rng);
        float* ptrs[] = {left.data(), right.data()};
        stress.process(ptrs, block % 2 + 1, 256, block % 2 ? -24.0f : 24.0f, block % 3, 1, 0);
        for (float sample : left) require(std::isfinite(sample) && std::abs(sample) <= 0.901f, "rapid controls remain bounded");
    }
    std::cout << "PASS: noise, mono/stereo and rapid automation stress\n";
    PitchEngine engine; engine.prepare(48000); std::vector<float> a(480000, 0.1f), b = a;
    auto start = std::chrono::steady_clock::now(); run(engine, a, b, 7, 2, 64);
    std::cout << "10 seconds stereo DSP processed in " << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() << " seconds (cloud CPU; not a Mac benchmark)\n";
}
