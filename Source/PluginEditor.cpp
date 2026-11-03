#include "PluginEditor.h"
FluteShiftEditor::FluteShiftEditor(FluteShiftProcessor& p)
    : AudioProcessorEditor(p), processor(p), pitchAttachment(p.parameters, "pitch", transpose),
      mixAttachment(p.parameters, "wet", mix), outputAttachment(p.parameters, "output", output),
      modeAttachment(p.parameters, "mode", mode) {
    for (auto* slider : {&transpose, &mix, &output}) {
        slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 28);
        slider->setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff61e6be));
        addAndMakeVisible(slider);
    }
    transpose.setName("Transpose in semitones"); mix.setName("Dry wet mix"); output.setName("Output gain in decibels"); mode.setName("Pitch engine mode");
    transpose.setTextValueSuffix(" st"); output.setTextValueSuffix(" dB");
    mode.addItemList({"Tone / sustained flute", "Beat / short grains", "Hybrid / protect attacks"}, 1);
    mode.setSelectedId(static_cast<int>(p.parameters.getRawParameterValue("mode")->load()) + 1, juce::dontSendNotification);
    addAndMakeVisible(mode);
    for (auto* button : {&down, &unity, &up}) addAndMakeVisible(button);
    down.onClick = [this] { step(-12); }; up.onClick = [this] { step(12); };
    unity.onClick = [this] { auto* parameter = processor.parameters.getParameter("pitch"); parameter->beginChangeGesture(); parameter->setValueNotifyingHost(parameter->convertTo0to1(0)); parameter->endChangeGesture(); };
    setSize(580, 420);
}
void FluteShiftEditor::step(float semitones) {
    auto* p = processor.parameters.getParameter("pitch");
    const float value = processor.parameters.getRawParameterValue("pitch")->load();
    p->beginChangeGesture(); p->setValueNotifyingHost(p->convertTo0to1(juce::jlimit(-24.0f, 24.0f, value + semitones))); p->endChangeGesture();
}
void FluteShiftEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff101a23));
    g.setColour(juce::Colour(0xff61e6be)); g.setFont(juce::FontOptions(30.0f)); g.drawText("FLUTE / SHIFT", 26, 20, 530, 42, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xffa4b7c4)); g.setFont(juce::FontOptions(14.0f));
    g.drawText("LIVE PITCH   /   FLUTE + BEATBOX", 28, 62, 500, 24, juce::Justification::centredLeft);
    g.drawText("TRANSPOSE", 28, 108, 260, 24, juce::Justification::centred);
    g.drawText("MIX", 326, 108, 100, 24, juce::Justification::centred);
    g.drawText("OUTPUT", 450, 108, 100, 24, juce::Justification::centred);
    g.drawText("16 ms engine buffer / host and interface add latency", 28, 378, 530, 22, juce::Justification::centredLeft);
}
void FluteShiftEditor::resized() {
    transpose.setBounds(28, 132, 260, 194); mix.setBounds(320, 148, 110, 138); output.setBounds(444, 148, 110, 138);
    down.setBounds(30, 332, 80, 32); unity.setBounds(118, 332, 80, 32); up.setBounds(206, 332, 80, 32);
    mode.setBounds(318, 310, 238, 40);
}
