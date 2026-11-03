#include "PluginProcessor.h"
#include "PluginEditor.h"
static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"pitch", 1}, "Transpose", juce::NormalisableRange<float>{-24, 24, 1}, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"mode", 1}, "Engine", juce::StringArray{"Tone", "Beat", "Hybrid"}, 2));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"wet", 1}, "Mix", juce::NormalisableRange<float>{0, 1, 0.01f}, 1));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"output", 1}, "Output dB", juce::NormalisableRange<float>{-24, 12, 0.1f}, 0));
    return layout;
}
FluteShiftProcessor::FluteShiftProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "FluteShift", makeParameters()) {
    pitch = parameters.getRawParameterValue("pitch"); mode = parameters.getRawParameterValue("mode");
    wet = parameters.getRawParameterValue("wet"); output = parameters.getRawParameterValue("output");
}
void FluteShiftProcessor::prepareToPlay(double rate, int) { engine.prepare(rate); setLatencySamples(engine.getLatency()); }
bool FluteShiftProcessor::isBusesLayoutSupported(const BusesLayout& layout) const {
    auto out = layout.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && out == layout.getMainInputChannelSet();
}
void FluteShiftProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals guard;
    engine.process(buffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples(), pitch->load(), static_cast<int>(mode->load()), wet->load(), output->load());
}
juce::AudioProcessorEditor* FluteShiftProcessor::createEditor() { return new FluteShiftEditor(*this); }
void FluteShiftProcessor::getStateInformation(juce::MemoryBlock& data) {
    if (auto xml = parameters.copyState().createXml()) copyXmlToBinary(*xml, data);
}
void FluteShiftProcessor::setStateInformation(const void* data, int bytes) {
    if (auto xml = getXmlFromBinary(data, bytes))
        if (xml->hasTagName(parameters.state.getType())) parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FluteShiftProcessor(); }
