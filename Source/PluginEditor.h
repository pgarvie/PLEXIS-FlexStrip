#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

//==============================================================================
// Custom look: knobs in the spirit of a vintage console strip (original artwork, PLEXIS Labs)
class FlexLookAndFeel : public juce::LookAndFeel_V4
{
public:
    enum class Knob { Red, White, Black };

    FlexLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
};

//==============================================================================
// Small switch with an LED: LABEL  (o)  [ ]
class LedSwitch : public juce::Button
{
public:
    LedSwitch (const juce::String& label, juce::Colour ledOn);
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    juce::String text;
    juce::Colour ledColour;
};

//==============================================================================
// Large illuminated H-DRIVE switch: glows amber (little) to red (a lot)
class HDriveButton : public juce::Button
{
public:
    HDriveButton();
    std::function<float()> getDrive;
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
};

//==============================================================================
// Backlit analogue VU meter
class VuMeterComponent : public juce::Component
{
public:
    void setValueDb (double vuDb);
    void paint (juce::Graphics&) override;

private:
    double needle = 0.0; // 0..1.05
};

//==============================================================================
// 7-segment LED level meter (0, -6, -12, -18, -24, -30, -36 dBFS)
class LedMeterComponent : public juce::Component
{
public:
    void setPeakDb (double db);
    void paint (juce::Graphics&) override;

private:
    double peakDb = -120.0;
};

//==============================================================================
class PlexisFlexStripEditor : public juce::AudioProcessorEditor,
                              private juce::Timer
{
public:
    explicit PlexisFlexStripEditor (PlexisFlexStripProcessor&);
    ~PlexisFlexStripEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupKnob (juce::Slider&, FlexLookAndFeel::Knob kind, const juce::String& paramId);
    void drawPanel (juce::Graphics&, juce::Rectangle<int>, const juce::String& title);
    void drawKnobLabel (juce::Graphics&, const juce::Slider&, const juce::String&);

    PlexisFlexStripProcessor& processor;
    FlexLookAndFeel lnf;

    juce::Slider gainKnob, lpKnob, hpKnob, hdriveKnob, outputKnob, vuTrimKnob;
    LedSwitch micSwitch  { "MIC",  juce::Colour (0xff3fdc5a) },
              lineSwitch { "LINE", juce::Colour (0xff3fdc5a) },
              instSwitch { "INST", juce::Colour (0xff3fdc5a) },
              padSwitch  { "-20",  juce::Colour (0xff3fdc5a) },
              polSwitch  { juce::String::fromUTF8 ("\xC3\x98"), juce::Colour (0xff3fdc5a) },
              filtSwitch { "IN",   juce::Colour (0xff3fdc5a) };
    HDriveButton hdriveButton;
    VuMeterComponent vuLeft, vuRight;
    LedMeterComponent ledMeter;

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAtt> gainAtt, lpAtt, hpAtt, hdriveAtt, outputAtt, vuTrimAtt;
    std::unique_ptr<ButtonAtt> padAtt, polAtt, filtAtt, hdriveInAtt;
    std::unique_ptr<juce::ParameterAttachment> sourceAtt;

    void setSourceFromUi (int index);
    void updateSourceSwitches (int index);

    juce::Rectangle<int> panelInput, panelFilters, panelDrive, panelMonitors, panelMaster;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlexisFlexStripEditor)
};
