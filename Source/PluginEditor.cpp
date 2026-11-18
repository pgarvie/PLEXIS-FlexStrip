#include "PluginEditor.h"

using namespace juce;

namespace
{
    const Colour kBg        { 0xff17181a };
    const Colour kPanel     { 0xff202225 };
    const Colour kPanelEdge { 0xff3a3d42 };
    const Colour kText      { 0xffd9dbde };
    const Colour kTextDim   { 0xff8d9198 };
    const Colour kAccent    { 0xffe8332c };
    const Colour kAmber     { 0xffffa726 };

    Font makeFont (float size, bool bold = false)
    {
        return Font (FontOptions (size, bold ? Font::bold : Font::plain));
    }
}

//==============================================================================
FlexLookAndFeel::FlexLookAndFeel()
{
    setColour (Slider::textBoxTextColourId, kTextDim);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
}

Label* FlexLookAndFeel::createSliderTextBox (Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (makeFont (11.0f));
    l->setJustificationType (Justification::centred);
    return l;
}

void FlexLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, Slider& slider)
{
    const auto kind = (Knob) (int) slider.getProperties().getWithDefault ("kind", 1);
    const auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const auto c = bounds.getCentre();
    const float r = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 7.0f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    // tick marks
    g.setColour (kTextDim.withAlpha (0.8f));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
        const float s = std::sin (a), co = std::cos (a);
        g.drawLine (c.x + s * (r + 3.0f), c.y - co * (r + 3.0f),
                    c.x + s * (r + 6.0f), c.y - co * (r + 6.0f), i == 0 || i == 10 ? 1.6f : 1.0f);
    }

    // shadow
    g.setColour (Colours::black.withAlpha (0.45f));
    g.fillEllipse (Rectangle<float> (2.0f * r, 2.0f * r).withCentre (c.translated (1.5f, 2.5f)));

    Colour top, bottom, pointer;
    switch (kind)
    {
        case FlexLookAndFeel::Knob::Red:   top = Colour (0xffff5a4d); bottom = Colour (0xffa8140f); pointer = Colours::white; break;
        case FlexLookAndFeel::Knob::Black: top = Colour (0xff3a3c40); bottom = Colour (0xff0c0c0d); pointer = Colours::white; break;
        default:                           top = Colour (0xfff4f4f4); bottom = Colour (0xffa9abae); pointer = Colour (0xff222222); break;
    }

    const auto body = Rectangle<float> (2.0f * r, 2.0f * r).withCentre (c);
    g.setGradientFill (ColourGradient (top, c.x - r * 0.4f, c.y - r * 0.6f, bottom, c.x + r * 0.5f, c.y + r * 0.8f, false));
    g.fillEllipse (body);
    g.setColour (Colours::black.withAlpha (0.5f));
    g.drawEllipse (body, 1.0f);

    // knurled inner cap
    const auto cap = body.reduced (r * 0.22f);
    g.setGradientFill (ColourGradient (top.brighter (0.15f), c.x, c.y - r * 0.5f, bottom.brighter (0.05f), c.x, c.y + r * 0.6f, false));
    g.fillEllipse (cap);

    // pointer
    const float s = std::sin (angle), co = std::cos (angle);
    g.setColour (pointer);
    g.drawLine (c.x + s * r * 0.25f, c.y - co * r * 0.25f, c.x + s * r * 0.88f, c.y - co * r * 0.88f, 2.4f);
}

//==============================================================================
LedSwitch::LedSwitch (const String& label, Colour ledOn)
    : Button (label), text (label), ledColour (ledOn)
{
    setClickingTogglesState (true);
}

void LedSwitch::paintButton (Graphics& g, bool highlighted, bool down)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (kText);
    g.setFont (makeFont (11.5f, true));
    g.drawText (text, b.removeFromLeft (46.0f), Justification::centredRight);

    b.removeFromLeft (8.0f);
    const auto led = b.removeFromLeft (10.0f).withSizeKeepingCentre (8.0f, 8.0f);
    const bool on = getToggleState();
    if (on)
    {
        g.setColour (ledColour.withAlpha (0.25f));
        g.fillEllipse (led.expanded (3.0f));
    }
    g.setColour (on ? ledColour : ledColour.darker (0.85f).withMultipliedSaturation (0.6f));
    g.fillEllipse (led);

    b.removeFromLeft (8.0f);
    const auto sw = b.removeFromLeft (20.0f).withSizeKeepingCentre (18.0f, 16.0f);
    g.setColour (down ? Colour (0xff4b4e53) : (highlighted ? Colour (0xff6a6e74) : Colour (0xff5a5d63)));
    g.fillRoundedRectangle (sw, 2.5f);
    g.setColour (Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (sw, 2.5f, 1.0f);
}

//==============================================================================
HDriveButton::HDriveButton()
    : Button ("H-DRIVE")
{
    setClickingTogglesState (true);
}

void HDriveButton::paintButton (Graphics& g, bool highlighted, bool down)
{
    auto b = getLocalBounds().toFloat().reduced (3.0f);
    const bool on = getToggleState();
    const float amount = getDrive ? jlimit (0.0f, 1.0f, getDrive()) : 0.0f;

    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (b.translated (0.0f, 2.0f), 6.0f);

    g.setColour (Colour (0xff0c0c0d));
    g.fillRoundedRectangle (b, 6.0f);
    g.setColour (kPanelEdge);
    g.drawRoundedRectangle (b, 6.0f, 1.2f);

    const auto face = b.reduced (5.0f);
    const Colour lit = kAmber.interpolatedWith (Colour (0xffff2a1f), amount);
    if (on)
    {
        g.setColour (lit.withAlpha (0.22f));
        g.fillRoundedRectangle (b.expanded (4.0f), 9.0f);
        g.setGradientFill (ColourGradient (lit.brighter (0.35f), face.getCentreX(), face.getY(),
                                           lit.darker (0.35f), face.getCentreX(), face.getBottom(), false));
    }
    else
    {
        g.setColour (Colour (0xff2a2b2e));
    }
    g.fillRoundedRectangle (face, 4.0f);

    // original PLEXIS mark: two crossing bars
    Path mark;
    const auto m = face.reduced (face.getWidth() * 0.22f);
    mark.startNewSubPath (m.getX(), m.getY());           mark.lineTo (m.getRight(), m.getBottom());
    mark.startNewSubPath (m.getRight(), m.getY());       mark.lineTo (m.getX(), m.getBottom());
    g.setColour (on ? Colours::black.withAlpha (0.75f) : Colour (0xff55585d));
    g.strokePath (mark, PathStrokeType (face.getWidth() * 0.13f, PathStrokeType::mitered, PathStrokeType::butt));

    if (down || highlighted)
    {
        g.setColour (Colours::white.withAlpha (down ? 0.10f : 0.05f));
        g.fillRoundedRectangle (face, 4.0f);
    }
}

//==============================================================================
void VuMeterComponent::setValueDb (double vuDb)
{
    const double target = plexis::VuMeter::needlePosition (vuDb);
    if (std::abs (target - needle) > 0.0005)
    {
        needle = target;
        repaint();
    }
}

void VuMeterComponent::paint (Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // bezel
    g.setColour (Colour (0xff0b0b0c));
    g.fillRoundedRectangle (bounds, 6.0f);
    auto face = bounds.reduced (4.0f);

    // backlit face
    g.setGradientFill (ColourGradient (Colour (0xfffff2c8), face.getCentreX(), face.getY() + face.getHeight() * 0.35f,
                                       Colour (0xffd6b25c), face.getX(), face.getBottom(), true));
    g.fillRoundedRectangle (face, 4.0f);

    Graphics::ScopedSaveState state (g);
    g.reduceClipRegion (face.toNearestInt());

    const float w = face.getWidth(), h = face.getHeight();
    const float spanDeg = 50.0f;
    // radius limited by height and by width so the +3 end of the scale is never clipped
    const float R = jmin (h * 0.80f, (w * 0.5f - 15.0f) / std::sin (degreesToRadians (spanDeg)));
    const Point<float> pivot (face.getCentreX(), face.getY() + h * 0.14f + R);

    auto angleFor = [&] (double vu)
    {
        const double p = std::min (1.0, plexis::VuMeter::needlePosition (vu) / 1.0);
        return degreesToRadians (-spanDeg + (float) p * 2.0f * spanDeg);
    };
    auto pt = [&] (float rad, float ang) { return Point<float> (pivot.x + std::sin (ang) * rad, pivot.y - std::cos (ang) * rad); };

    // scale arc: black up to 0 VU, red from 0 VU to +3
    const float a0 = angleFor (-20.0), a1 = angleFor (0.0), a2 = angleFor (3.0);
    Path blackArc, redArc;
    blackArc.addCentredArc (pivot.x, pivot.y, R, R, 0.0f, a0, a1, true);
    redArc.addCentredArc (pivot.x, pivot.y, R, R, 0.0f, a1, a2, true);
    g.setColour (Colour (0xff1b1b1b));
    g.strokePath (blackArc, PathStrokeType (1.4f));
    g.setColour (Colour (0xffc4281f));
    g.strokePath (redArc, PathStrokeType (4.0f));

    // ticks and labels
    struct Tick { double vu; const char* label; };
    const Tick major[] { { -20, "20" }, { -10, "10" }, { -7, "7" }, { -5, "5" }, { -3, "3" }, { -2, "2" },
                         { -1, "1" }, { 0, "0" }, { 1, "1" }, { 2, "2" }, { 3, "3" } };
    g.setFont (makeFont (jmin (h * 0.085f, 11.5f), true));
    for (auto& t : major)
    {
        const float a = angleFor (t.vu);
        g.setColour (t.vu >= 0 ? Colour (0xffc4281f) : Colour (0xff1b1b1b));
        g.drawLine (Line<float> (pt (R - 7.0f, a), pt (R + 2.0f, a)), 1.4f);
        const auto lp = pt (R - 17.0f, a);
        g.drawText (t.label, Rectangle<float> (16.0f, 12.0f).withCentre (lp), Justification::centred);
    }
    g.setColour (Colour (0xff1b1b1b));
    for (double vu : { -15.0, -9.0, -8.0, -6.0, -4.0 })
    {
        const float a = angleFor (vu);
        g.drawLine (Line<float> (pt (R - 4.0f, a), pt (R + 1.0f, a)), 1.0f);
    }

    g.setFont (makeFont (h * 0.2f, true));
    g.setColour (Colour (0xff1b1b1b));
    g.drawText ("VU", Rectangle<float> (face.getCentreX() - 20.0f, pivot.y - R * 0.50f, 40.0f, h * 0.2f), Justification::centred);
    g.setColour (Colour (0xffc4281f));
    g.setFont (makeFont (h * 0.1f, true));
    g.drawText ("-", Rectangle<float> (face.getX() + 6.0f, face.getY() + 4.0f, 10.0f, 12.0f), Justification::centred);
    g.drawText ("+", Rectangle<float> (face.getRight() - 16.0f, face.getY() + 4.0f, 10.0f, 12.0f), Justification::centred);
    juce::ignoreUnused (w);

    // needle
    const float na = degreesToRadians (-spanDeg + (float) jlimit (0.0, 1.0, needle) * 2.0f * spanDeg);
    g.setColour (Colours::black.withAlpha (0.25f));
    g.drawLine (Line<float> (pt (0.0f, na).translated (2.0f, 1.5f), pt (R + 6.0f, na).translated (2.0f, 1.5f)), 2.0f);
    g.setColour (Colour (0xff111111));
    g.drawLine (Line<float> (pt (0.0f, na), pt (R + 6.0f, na)), 1.5f);

    // pivot cover
    g.setColour (Colour (0xff0b0b0c));
    g.fillRect (Rectangle<float> (face.getX(), pivot.y - 2.0f, w, face.getBottom() - pivot.y + 3.0f));
    g.setColour (Colour (0xff2b2b2e));
    g.fillEllipse (Rectangle<float> (14.0f, 14.0f).withCentre (pivot));
}

//==============================================================================
void LedMeterComponent::setPeakDb (double db)
{
    const int before = plexis::PeakMeter::litSegments (peakDb);
    peakDb = db;
    if (before != plexis::PeakMeter::litSegments (peakDb))
        repaint();
}

void LedMeterComponent::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const int lit = plexis::PeakMeter::litSegments (peakDb);
    const char* labels[] { "0", "6", "12", "18", "24", "30", "36" };
    const float segH = b.getHeight() / 8.0f;

    g.setFont (makeFont (9.5f));
    g.setColour (kTextDim);
    g.drawText ("dBFS", Rectangle<float> (b.getX(), b.getY(), b.getWidth(), segH), Justification::centredLeft);

    for (int i = 0; i < 7; ++i)
    {
        // LED i (0 = top) is lit when level >= threshold i; LEDs light from the bottom up
        const bool on = i >= 7 - lit;
        Colour c = i == 0 ? Colour (0xffff3b30) : (i < 3 ? Colour (0xffffd23a) : Colour (0xff3fdc5a));
        const auto row = Rectangle<float> (b.getX(), b.getY() + segH * (float) (i + 1), b.getWidth(), segH);
        const auto led = Rectangle<float> (8.0f, 8.0f).withCentre ({ row.getX() + 7.0f, row.getCentreY() });
        if (on)
        {
            g.setColour (c.withAlpha (0.25f));
            g.fillEllipse (led.expanded (3.0f));
        }
        g.setColour (on ? c : c.darker (0.8f).withMultipliedSaturation (0.7f));
        g.fillEllipse (led);
        g.setColour (kText);
        g.setFont (makeFont (10.0f));
        g.drawText (labels[i], row.withTrimmedLeft (18.0f), Justification::centredLeft);
    }
}

//==============================================================================
PlexisFlexStripEditor::PlexisFlexStripEditor (PlexisFlexStripProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lnf);
    setSize (840, 510);

    setupKnob (gainKnob,    FlexLookAndFeel::Knob::Red,   ParamIDs::gain);
    setupKnob (lpKnob,      FlexLookAndFeel::Knob::White, ParamIDs::lpFreq);
    setupKnob (hpKnob,      FlexLookAndFeel::Knob::White, ParamIDs::hpFreq);
    setupKnob (hdriveKnob,  FlexLookAndFeel::Knob::Black, ParamIDs::hdrive);
    setupKnob (outputKnob,  FlexLookAndFeel::Knob::Red,   ParamIDs::output);
    setupKnob (vuTrimKnob,  FlexLookAndFeel::Knob::White, ParamIDs::vuTrim);

    gainAtt   = std::make_unique<SliderAtt> (p.apvts, ParamIDs::gain,   gainKnob);
    lpAtt     = std::make_unique<SliderAtt> (p.apvts, ParamIDs::lpFreq, lpKnob);
    hpAtt     = std::make_unique<SliderAtt> (p.apvts, ParamIDs::hpFreq, hpKnob);
    hdriveAtt = std::make_unique<SliderAtt> (p.apvts, ParamIDs::hdrive, hdriveKnob);
    outputAtt = std::make_unique<SliderAtt> (p.apvts, ParamIDs::output, outputKnob);
    vuTrimAtt = std::make_unique<SliderAtt> (p.apvts, ParamIDs::vuTrim, vuTrimKnob);

    // readouts
    gainKnob.textFromValueFunction = [this] (double v)
    {
        const int src = (int) processor.apvts.getRawParameterValue (ParamIDs::source)->load();
        return String (v / 10.0 * plexis::StripParams::gainRangeDb (src), 1) + " dB";
    };
    gainKnob.updateText();
    lpKnob.textFromValueFunction = hpKnob.textFromValueFunction = [] (double v)
    {
        return v >= 1000.0 ? String (v / 1000.0, 2) + " kHz" : String (roundToInt (v)) + " Hz";
    };
    lpKnob.updateText(); hpKnob.updateText();
    hdriveKnob.textFromValueFunction = [] (double v) { return String (roundToInt (v * 100.0)) + " %"; };
    hdriveKnob.updateText();
    outputKnob.textFromValueFunction = vuTrimKnob.textFromValueFunction = [] (double v)
    {
        return (v > 0 ? "+" : "") + String (v, 1) + " dB";
    };
    outputKnob.updateText(); vuTrimKnob.updateText();

    // switches
    for (auto* s : { &padSwitch, &polSwitch, &filtSwitch, &micSwitch, &lineSwitch, &instSwitch })
        addAndMakeVisible (*s);

    padAtt      = std::make_unique<ButtonAtt> (p.apvts, ParamIDs::pad,       padSwitch);
    polAtt      = std::make_unique<ButtonAtt> (p.apvts, ParamIDs::polarity,  polSwitch);
    filtAtt     = std::make_unique<ButtonAtt> (p.apvts, ParamIDs::filtersIn, filtSwitch);

    // source: three radio-style switches bound to one choice parameter
    micSwitch.onClick  = [this] { if (micSwitch.getToggleState())  setSourceFromUi (0); else updateSourceSwitches ((int) processor.apvts.getRawParameterValue (ParamIDs::source)->load()); };
    lineSwitch.onClick = [this] { if (lineSwitch.getToggleState()) setSourceFromUi (1); else updateSourceSwitches ((int) processor.apvts.getRawParameterValue (ParamIDs::source)->load()); };
    instSwitch.onClick = [this] { if (instSwitch.getToggleState()) setSourceFromUi (2); else updateSourceSwitches ((int) processor.apvts.getRawParameterValue (ParamIDs::source)->load()); };

    auto* sourceParam = p.apvts.getParameter (ParamIDs::source);
    sourceAtt = std::make_unique<ParameterAttachment> (*sourceParam, [this] (float v)
    {
        updateSourceSwitches ((int) std::lround (v));
        gainKnob.updateText();
        repaint();
    }, nullptr);
    sourceAtt->sendInitialUpdate();

    // H-DRIVE
    hdriveButton.getDrive = [this] { return (float) hdriveKnob.getValue(); };
    addAndMakeVisible (hdriveButton);
    hdriveInAtt = std::make_unique<ButtonAtt> (p.apvts, ParamIDs::hdriveIn, hdriveButton);
    hdriveKnob.onValueChange = [this] { hdriveButton.repaint(); };

    addAndMakeVisible (vuLeft);
    addAndMakeVisible (vuRight);
    addAndMakeVisible (ledMeter);

    startTimerHz (30);
}

PlexisFlexStripEditor::~PlexisFlexStripEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void PlexisFlexStripEditor::setupKnob (Slider& s, FlexLookAndFeel::Knob kind, const String&)
{
    s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    s.setRotaryParameters (degreesToRadians (-135.0f), degreesToRadians (135.0f), true);
    s.setTextBoxStyle (Slider::TextBoxBelow, true, 80, 16);
    s.getProperties().set ("kind", (int) kind);
    s.setColour (Slider::textBoxTextColourId, kTextDim);
    s.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    s.setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    s.setDoubleClickReturnValue (true, s.getDoubleClickReturnValue());
    s.setMouseDragSensitivity (180);
    addAndMakeVisible (s);
}

void PlexisFlexStripEditor::setSourceFromUi (int index)
{
    if (auto* param = processor.apvts.getParameter (ParamIDs::source))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 ((float) index));
        param->endChangeGesture();
    }
}

void PlexisFlexStripEditor::updateSourceSwitches (int index)
{
    micSwitch.setToggleState  (index == 0, dontSendNotification);
    lineSwitch.setToggleState (index == 1, dontSendNotification);
    instSwitch.setToggleState (index == 2, dontSendNotification);
}

void PlexisFlexStripEditor::timerCallback()
{
    vuLeft.setValueDb  (processor.vuDb[0].load());
    vuRight.setValueDb (processor.vuDb[1].load());
    ledMeter.setPeakDb (processor.ledPeakDb.load());
}

void PlexisFlexStripEditor::drawPanel (Graphics& g, Rectangle<int> r, const String& title)
{
    const auto rf = r.toFloat();
    g.setColour (kPanel);
    g.fillRoundedRectangle (rf, 8.0f);
    g.setColour (kPanelEdge);
    g.drawRoundedRectangle (rf.reduced (0.5f), 8.0f, 1.2f);
    g.setColour (kText);
    g.setFont (makeFont (12.0f, true));
    g.drawText (title, r.withHeight (22).withTrimmedLeft (14), Justification::centredLeft);
}

void PlexisFlexStripEditor::drawKnobLabel (Graphics& g, const Slider& s, const String& text)
{
    g.setColour (kText);
    g.setFont (makeFont (11.5f, true));
    g.drawText (text, s.getBounds().withHeight (16).translated (0, -14), Justification::centred);
}

void PlexisFlexStripEditor::paint (Graphics& g)
{
    g.fillAll (kBg);

    // header
    g.setColour (Colour (0xff101113));
    g.fillRect (0, 0, getWidth(), 56);
    g.setColour (kAccent);
    g.fillRect (0, 55, getWidth(), 2);

    // original PLEXIS mark
    Path mark;
    mark.startNewSubPath (24.0f, 16.0f); mark.lineTo (46.0f, 40.0f);
    mark.startNewSubPath (46.0f, 16.0f); mark.lineTo (24.0f, 40.0f);
    g.setColour (kAccent);
    g.strokePath (mark, PathStrokeType (7.0f, PathStrokeType::mitered, PathStrokeType::butt));

    g.setColour (kText);
    g.setFont (makeFont (26.0f, true));
    g.drawText ("PLEXIS", 58, 8, 130, 30, Justification::centredLeft);
    g.setColour (kTextDim);
    g.setFont (makeFont (13.0f));
    g.drawText ("LABS", 58, 33, 60, 16, Justification::centredLeft);

    g.setColour (kText);
    g.setFont (makeFont (20.0f, true));
    g.drawText ("FlexStrip", getWidth() - 260, 8, 240, 26, Justification::centredRight);
    g.setColour (kTextDim);
    g.setFont (makeFont (11.0f));
    g.drawText ("single channel strip with H-DRIVE", getWidth() - 360, 32, 340, 16, Justification::centredRight);

    drawPanel (g, panelInput,    "INPUT");
    drawPanel (g, panelFilters,  "FILTERS");
    drawPanel (g, panelDrive,    "H-DRIVE");
    drawPanel (g, panelMonitors, "MONITORS");
    drawPanel (g, panelMaster,   "MASTER");

    drawKnobLabel (g, gainKnob, "GAIN");
    drawKnobLabel (g, lpKnob, "LP");
    drawKnobLabel (g, hpKnob, "HP");
    drawKnobLabel (g, hdriveKnob, "DRIVE");
    drawKnobLabel (g, outputKnob, "OUTPUT");
    drawKnobLabel (g, vuTrimKnob, "VU TRIM");

    g.setColour (kText);
    g.setFont (makeFont (12.0f, true));
    g.drawText ("LEFT",  vuLeft.getBounds().withY (vuLeft.getBottom() + 4).withHeight (16), Justification::centred);
    g.drawText ("RIGHT", vuRight.getBounds().withY (vuRight.getBottom() + 4).withHeight (16), Justification::centred);
    g.setColour (kTextDim);
    g.setFont (makeFont (10.0f));
    g.drawText ("0 VU = -18 dBFS", panelMonitors.withTrimmedTop (panelMonitors.getHeight() - 22).withTrimmedRight (14), Justification::centredRight);
    g.drawText ("LEVEL", ledMeter.getBounds().withY (ledMeter.getBottom() + 2).withHeight (14).withWidth (70), Justification::centredLeft);
}

void PlexisFlexStripEditor::resized()
{
    panelInput    = { 20, 70, 410, 160 };
    panelFilters  = { 20, 240, 410, 120 };
    panelDrive    = { 20, 370, 410, 130 };
    panelMonitors = { 446, 70, 374, 226 };
    panelMaster   = { 446, 308, 374, 192 };

    // input: switches on the left, gain knob on the right
    const int sx = panelInput.getX() + 22, sy = panelInput.getY() + 30;
    int row = 0;
    for (auto* s : { &micSwitch, &lineSwitch, &instSwitch, &padSwitch, &polSwitch })
        s->setBounds (sx, sy + 25 * row++, 110, 22);
    gainKnob.setBounds (panelInput.getX() + 230, panelInput.getY() + 36, 110, 112 + 6);

    // filters
    filtSwitch.setBounds (panelFilters.getX() + 22, panelFilters.getY() + 52, 110, 22);
    lpKnob.setBounds (panelFilters.getX() + 170, panelFilters.getY() + 24, 100, 94);
    hpKnob.setBounds (panelFilters.getX() + 290, panelFilters.getY() + 24, 100, 94);

    // drive
    ledMeter.setBounds (panelDrive.getX() + 20, panelDrive.getY() + 26, 70, 88);
    hdriveKnob.setBounds (panelDrive.getX() + 140, panelDrive.getY() + 28, 100, 94);
    hdriveButton.setBounds (panelDrive.getX() + 290, panelDrive.getY() + 36, 74, 74);

    // monitors
    const int vuW = 172, vuH = 130;
    vuLeft.setBounds  (panelMonitors.getX() + 10, panelMonitors.getY() + 38, vuW, vuH);
    vuRight.setBounds (panelMonitors.getRight() - 10 - vuW, panelMonitors.getY() + 38, vuW, vuH);

    // master
    outputKnob.setBounds (panelMaster.getX() + 60, panelMaster.getY() + 56, 100, 94);
    vuTrimKnob.setBounds (panelMaster.getX() + 214, panelMaster.getY() + 56, 100, 94);
}
