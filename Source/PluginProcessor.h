#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <array>
#include <vector>
#include "Dsp/FlexStripDsp.h"

namespace ParamIDs
{
    static const juce::String source    { "source" };
    static const juce::String gain      { "gain" };
    static const juce::String pad       { "pad" };
    static const juce::String polarity  { "polarity" };
    static const juce::String filtersIn { "filtersIn" };
    static const juce::String hpFreq    { "hpFreq" };
    static const juce::String lpFreq    { "lpFreq" };
    static const juce::String hdriveIn  { "hdriveIn" };
    static const juce::String hdrive    { "hdrive" };
    static const juce::String output    { "output" };
    static const juce::String vuTrim    { "vuTrim" };
}

class PlexisFlexStripProcessor : public juce::AudioProcessor
{
public:
    PlexisFlexStripProcessor();
    ~PlexisFlexStripProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "PLEXIS FlexStrip"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // Meter values read by the editor (written on the audio thread)
    std::atomic<float> vuDb[2] { { -60.0f }, { -60.0f } };
    std::atomic<float> ledPeakDb { -120.0f };
    std::atomic<int> numActiveChannels { 1 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    plexis::StripParams readParams() const;

    std::array<plexis::ChannelStrip, 2> strips;
    std::vector<double> scratch;

    std::atomic<float>* pSource = nullptr;
    std::atomic<float>* pGain = nullptr;
    std::atomic<float>* pPad = nullptr;
    std::atomic<float>* pPolarity = nullptr;
    std::atomic<float>* pFiltersIn = nullptr;
    std::atomic<float>* pHp = nullptr;
    std::atomic<float>* pLp = nullptr;
    std::atomic<float>* pHdriveIn = nullptr;
    std::atomic<float>* pHdrive = nullptr;
    std::atomic<float>* pOutput = nullptr;
    std::atomic<float>* pVuTrim = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlexisFlexStripProcessor)
};
