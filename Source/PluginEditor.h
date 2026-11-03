#pragma once
#include "PluginProcessor.h"
class FluteShiftEditor : public juce::AudioProcessorEditor {
public:
    explicit FluteShiftEditor(FluteShiftProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void step(float semitones);
    FluteShiftProcessor& processor;
    juce::Slider transpose, mix, output;
    juce::ComboBox mode;
    juce::TextButton down{"- OCT"}, unity{"RESET"}, up{"+ OCT"};
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    SliderAttachment pitchAttachment, mixAttachment, outputAttachment;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment modeAttachment;
};
