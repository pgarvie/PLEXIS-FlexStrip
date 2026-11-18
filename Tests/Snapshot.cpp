// Headless check: instantiates the processor, runs audio through it, and renders the editor to PNG.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <cstdio>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String outDir = argc > 1 ? argv[1] : ".";

    PlexisFlexStripProcessor proc;
    proc.setPlayConfigDetails (2, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);

    auto set = [&] (const juce::String& id, float v)
    {
        auto* p = proc.apvts.getParameter (id);
        p->setValueNotifyingHost (p->convertTo0to1 (v));
    };
    set (ParamIDs::gain, 4.0f);
    set (ParamIDs::filtersIn, 1.0f);
    set (ParamIDs::hpFreq, 120.0f);
    set (ParamIDs::lpFreq, 9000.0f);
    set (ParamIDs::hdriveIn, 1.0f);
    set (ParamIDs::hdrive, 0.65f);
    set (ParamIDs::source, 1.0f);

    // 1 s of a -30 dBFS 220 Hz sine through the plugin
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    double ph = 0; bool ok = true; float maxAbs = 0;
    for (int b = 0; b < 94; ++b)
    {
        for (int i = 0; i < 512; ++i, ph += 2.0 * 3.14159265358979 * 220.0 / 48000.0)
            for (int ch = 0; ch < 2; ++ch) buf.setSample (ch, i, 0.0316f * (float) std::sin (ph));
        proc.processBlock (buf, midi);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i) { const float v = buf.getSample (ch, i); if (! std::isfinite (v)) ok = false; maxAbs = std::max (maxAbs, std::abs (v)); }
    }
    std::printf ("processBlock finite: %s, peak out %.4f, VU L/R %.2f / %.2f, LED peak %.2f dBFS\n",
                 ok ? "yes" : "NO", maxAbs, proc.vuDb[0].load(), proc.vuDb[1].load(), proc.ledPeakDb.load());

    // state save / restore
    juce::MemoryBlock state; proc.getStateInformation (state);
    PlexisFlexStripProcessor proc2; proc2.setStateInformation (state.getData(), (int) state.getSize());
    std::printf ("state restore: gain %.2f (expected 4.00), source %d (expected 1)\n",
                 proc2.apvts.getRawParameterValue (ParamIDs::gain)->load(), (int) proc2.apvts.getRawParameterValue (ParamIDs::source)->load());

    // editor snapshot
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.5f);
    juce::File f (outDir + "/editor.png");
    juce::FileOutputStream os (f); os.setPosition (0); os.truncate();
    juce::PNGImageFormat().writeImageToStream (img, os);
    std::printf ("snapshot: %dx%d -> %s\n", img.getWidth(), img.getHeight(), f.getFullPathName().toRawUTF8());
    ed.reset();

    // component check with known values: VU at -20 / -7 / 0 / +3, LED meter at -50 / -14.5 / -3 / 0 dBFS
    {
        juce::Image sheet (juce::Image::ARGB, 1000, 330, true);
        juce::Graphics g (sheet);
        g.fillAll (juce::Colour (0xff202225));
        const double vus[] { -20.0, -7.0, 0.0, 3.0 };
        for (int i = 0; i < 4; ++i)
        {
            VuMeterComponent vu; vu.setSize (230, 160); vu.setValueDb (vus[i]);
            auto im = vu.createComponentSnapshot (vu.getLocalBounds(), true, 1.0f);
            g.drawImageAt (im, 10 + i * 245, 10);
        }
        const double leds[] { -50.0, -14.5, -3.0, 0.0 };
        for (int i = 0; i < 4; ++i)
        {
            LedMeterComponent lm; lm.setSize (70, 120); lm.setPeakDb (leds[i]);
            auto im = lm.createComponentSnapshot (lm.getLocalBounds(), true, 1.0f);
            g.drawImageAt (im, 40 + i * 245, 190);
        }
        juce::File f2 (outDir + "/components.png");
        juce::FileOutputStream os2 (f2); os2.setPosition (0); os2.truncate();
        juce::PNGImageFormat().writeImageToStream (sheet, os2);
    }
    return ok ? 0 : 1;
}
