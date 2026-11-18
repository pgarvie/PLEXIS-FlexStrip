#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

AudioProcessorValueTreeState::ParameterLayout PlexisFlexStripProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::source, 1 }, "Source",
                                                              StringArray { "Mic", "Line", "Inst" }, 0));

    params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::gain, 1 }, "Gain",
                                                             NormalisableRange<float> (0.0f, 10.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::pad, 1 }, "Pad -20", false));
    params.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::polarity, 1 }, "Polarity", false));
    params.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::filtersIn, 1 }, "Filters In", false));

    NormalisableRange<float> hpRange (25.0f, 3150.0f, 0.0f, 0.0f);
    hpRange.setSkewForCentre (280.0f);
    params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::hpFreq, 1 }, "HP Freq", hpRange, 25.0f,
                                                             AudioParameterFloatAttributes().withLabel ("Hz")));

    NormalisableRange<float> lpRange (160.0f, 20000.0f, 0.0f, 0.0f);
    lpRange.setSkewForCentre (1800.0f);
    params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::lpFreq, 1 }, "LP Freq", lpRange, 20000.0f,
                                                             AudioParameterFloatAttributes().withLabel ("Hz")));

    params.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::hdriveIn, 1 }, "H-Drive In", false));
    params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::hdrive, 1 }, "H-Drive",
                                                             NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));

    params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::output, 1 }, "Output",
                                                             NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f,
                                                             AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::vuTrim, 1 }, "VU Trim",
                                                             NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
                                                             AudioParameterFloatAttributes().withLabel ("dB")));

    return { params.begin(), params.end() };
}

PlexisFlexStripProcessor::PlexisFlexStripProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  AudioChannelSet::stereo(), true)
                          .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    pSource    = apvts.getRawParameterValue (ParamIDs::source);
    pGain      = apvts.getRawParameterValue (ParamIDs::gain);
    pPad       = apvts.getRawParameterValue (ParamIDs::pad);
    pPolarity  = apvts.getRawParameterValue (ParamIDs::polarity);
    pFiltersIn = apvts.getRawParameterValue (ParamIDs::filtersIn);
    pHp        = apvts.getRawParameterValue (ParamIDs::hpFreq);
    pLp        = apvts.getRawParameterValue (ParamIDs::lpFreq);
    pHdriveIn  = apvts.getRawParameterValue (ParamIDs::hdriveIn);
    pHdrive    = apvts.getRawParameterValue (ParamIDs::hdrive);
    pOutput    = apvts.getRawParameterValue (ParamIDs::output);
    pVuTrim    = apvts.getRawParameterValue (ParamIDs::vuTrim);
}

bool PlexisFlexStripProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return in == AudioChannelSet::mono() || in == AudioChannelSet::stereo();
}

plexis::StripParams PlexisFlexStripProcessor::readParams() const
{
    plexis::StripParams p;
    p.source    = (int) pSource->load();
    p.gainKnob  = pGain->load();
    p.pad       = pPad->load() > 0.5f;
    p.polarity  = pPolarity->load() > 0.5f;
    p.filtersIn = pFiltersIn->load() > 0.5f;
    p.hpHz      = pHp->load();
    p.lpHz      = pLp->load();
    p.hdriveIn  = pHdriveIn->load() > 0.5f;
    p.hdrive    = pHdrive->load();
    p.outputDb  = pOutput->load();
    p.vuTrimDb  = pVuTrim->load();
    return p;
}

void PlexisFlexStripProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    scratch.assign ((size_t) std::max (samplesPerBlock, 1) + 8, 0.0);
    const auto p = readParams();
    for (auto& s : strips)
    {
        s.setParams (p, true);
        s.prepare (sampleRate);
        s.setParams (p, true);
    }
}

void PlexisFlexStripProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;

    const int numCh = std::min (buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();
    if ((int) scratch.size() < n) scratch.resize ((size_t) n); // only if the host exceeds the announced block size

    const auto p = readParams();
    for (int ch = 0; ch < numCh; ++ch)
    {
        strips[(size_t) ch].setParams (p);
        strips[(size_t) ch].process (buffer.getWritePointer (ch), n, scratch.data());
    }

    numActiveChannels.store (numCh);
    vuDb[0].store ((float) strips[0].getVuDb());
    vuDb[1].store (numCh > 1 ? (float) strips[1].getVuDb() : (float) strips[0].getVuDb());

    float peak = (float) strips[0].getLedPeakDb();
    if (numCh > 1) peak = std::max (peak, (float) strips[1].getLedPeakDb());
    ledPeakDb.store (peak);
}

AudioProcessorEditor* PlexisFlexStripProcessor::createEditor()
{
    return new PlexisFlexStripEditor (*this);
}

void PlexisFlexStripProcessor::getStateInformation (MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PlexisFlexStripProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PlexisFlexStripProcessor();
}
