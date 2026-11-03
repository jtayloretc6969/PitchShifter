#include "PluginProcessor.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
static void check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
int main() {
    juce::ScopedJuceInitialiser_GUI initialise;
    FluteShiftProcessor processor;
    processor.prepareToPlay(48000, 128);
    check(processor.getLatencySamples() == 768, "reported latency");
    juce::AudioBuffer<float> buffer(2, 128);
    juce::MidiBuffer midi;
    for (int block = 0; block < 10; ++block) {
        buffer.clear();
        if (block == 0) { buffer.setSample(0, 0, 0.5f); buffer.setSample(1, 0, 0.5f); }
        processor.processBlock(buffer, midi);
        for (int channel = 0; channel < 2; ++channel)
            for (int i = 0; i < 128; ++i)
                check(std::abs(buffer.getSample(channel, i) - (block == 6 && i == 0 ? 0.5f : 0.0f)) < 1e-6f, "unity impulse through processor");
    }
    auto* pitch = processor.parameters.getParameter("pitch");
    pitch->setValueNotifyingHost(pitch->convertTo0to1(7));
    auto* mode = processor.parameters.getParameter("mode");
    mode->setValueNotifyingHost(mode->convertTo0to1(1));
    auto* mix = processor.parameters.getParameter("wet");
    mix->setValueNotifyingHost(mix->convertTo0to1(0.35f));
    juce::MemoryBlock state; processor.getStateInformation(state);
    FluteShiftProcessor restored;
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    check(restored.parameters.getRawParameterValue("pitch")->load() == 7, "transpose recall");
    check(restored.parameters.getRawParameterValue("mode")->load() == 1, "mode recall");
    check(std::abs(restored.parameters.getRawParameterValue("wet")->load() - 0.35f) < 1e-5f, "mix recall");
    std::cout << "PASS: processor audio, reported latency, and session state recall\n";
}
