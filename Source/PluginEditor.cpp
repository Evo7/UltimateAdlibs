#include "PluginEditor.h"

UltimateAdlibsAudioProcessorEditor::UltimateAdlibsAudioProcessorEditor (UltimateAdlibsAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&lnf);

    auto& vts = audioProcessor.apvts;

    auto bindS = [&] (juce::Slider& s, const char* id, std::unique_ptr<SliderAttachment>& a)
    {
        makeKnob (s);
        addAndMakeVisible (s);
        a = std::make_unique<SliderAttachment> (vts, id, s);
    };

    auto bindB = [&] (juce::ToggleButton& b, const char* id, const juce::String& label, std::unique_ptr<ButtonAttachment>& a)
    {
        addAndMakeVisible (b);
        b.setButtonText (label);
        a = std::make_unique<ButtonAttachment> (vts, id, b);
    };

    // Title / preset
    title.setText ("ULTIMATE ADLIBS", juce::dontSendNotification);
    title.setJustificationType (juce::Justification::centredLeft);
    title.setColour (juce::Label::textColourId, juce::Colour (0xFFFFF0C2));
    title.setFont (juce::Font (22.0f, juce::Font::bold));
    addAndMakeVisible (title);

    presetBox.addItem ("Default", 1);
    presetBox.addItem ("Wide Throw", 2);
    presetBox.addItem ("Tight Double", 3);
    presetBox.setSelectedId (1);
    addAndMakeVisible (presetBox);

    // VU meters
    addAndMakeVisible (inVu);
    addAndMakeVisible (outVu);

    inLbl.setText ("IN", juce::dontSendNotification);
    outLbl.setText ("OUT", juce::dontSendNotification);
    for (auto* l : { &inLbl, &outLbl })
    {
        l->setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.7f));
        l->setFont (juce::Font (12.0f, juce::Font::bold));
        l->setJustificationType (juce::Justification::centred);
        addAndMakeVisible (*l);
    }

    // Global
    bindS (inGain, "IN_GAIN", inGainA);
    bindS (globalMix, "GLOBAL_MIX", globalMixA);
    bindS (outGain, "OUT_GAIN", outGainA);

    // Filters
    bindB (filtOn, "FILT_ON", "Filters", filtOnA);
    bindS (hpf, "HPF_HZ", hpfA);
    bindS (lpf, "LPF_HZ", lpfA);
    bindS (filtMix, "FILT_MIX", filtMixA);

    // Dist
    bindB (distOn, "DIST_ON", "Dist", distOnA);
    bindS (distDrive, "DIST_DRIVE", distDriveA);
    bindS (distMix, "DIST_MIX", distMixA);

    // Chorus
    bindB (choOn, "CHO_ON", "Chorus", choOnA);
    bindS (choRate, "CHO_RATE", choRateA);
    bindS (choDepth, "CHO_DEPTH", choDepthA);
    bindS (choMix, "CHO_MIX", choMixA);

    // Flanger
    bindB (flaOn, "FLA_ON", "Flanger", flaOnA);
    bindS (flaRate, "FLA_RATE", flaRateA);
    bindS (flaDepth, "FLA_DEPTH", flaDepthA);
    bindS (flaFb, "FLA_FB", flaFbA);
    bindS (flaMix, "FLA_MIX", flaMixA);

    // Delay
    bindB (dlyOn, "DLY_ON", "Delay", dlyOnA);
    bindS (dlyTime, "DLY_TIME", dlyTimeA);
    bindS (dlyFb, "DLY_FB", dlyFbA);
    bindS (dlyMix, "DLY_MIX", dlyMixA);

    // Reverb
    bindB (revOn, "REV_ON", "Reverb", revOnA);
    bindS (revSize, "REV_SIZE", revSizeA);
    bindS (revDamp, "REV_DAMP", revDampA);
    bindS (revMix, "REV_MIX", revMixA);

    setSize (1080, 580);
}

UltimateAdlibsAudioProcessorEditor::~UltimateAdlibsAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void UltimateAdlibsAudioProcessorEditor::makeKnob (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 86, 18);
}

void UltimateAdlibsAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xFF0B0B0B));

    // Top bar background
    auto top = getLocalBounds().removeFromTop (66).toFloat().reduced (10, 10);
    g.setColour (juce::Colour (0xFF121212));
    g.fillRoundedRectangle (top, 16.0f);

    // Cards
    g.setColour (juce::Colour (0xFF141414));
    for (auto& s : sections)
        g.fillRoundedRectangle (s.area.toFloat(), 18.0f);

    g.setColour (juce::Colour (0xFF242424));
    for (auto& s : sections)
        g.drawRoundedRectangle (s.area.toFloat(), 18.0f, 1.5f);

    // Titles
    g.setFont (juce::Font (13.0f, juce::Font::bold));
    g.setColour (juce::Colours::white.withAlpha (0.7f));
    for (auto& s : sections)
    {
        auto t = s.area.reduced (14).removeFromTop (18);
        g.drawText (s.name, t, juce::Justification::centredLeft);
    }
}

void UltimateAdlibsAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (10);

    // Top bar
    auto top = r.removeFromTop (66).reduced (10, 10);

    title.setBounds (top.removeFromLeft (280));
    presetBox.setBounds (top.removeFromLeft (240).reduced (0, 12));

    // meters area
    auto meters = top.removeFromLeft (160);
    auto mW = 44;

    auto inArea = meters.removeFromLeft (mW);
    meters.removeFromLeft (12);
    auto outArea = meters.removeFromLeft (mW);

    inLbl.setBounds  (inArea.removeFromTop (16));
    inVu.setBounds   (inArea);
    outLbl.setBounds (outArea.removeFromTop (16));
    outVu.setBounds  (outArea);

    // Global knobs on right
    auto global = top;
    const int cellW = global.getWidth() / 3;
    inGain.setBounds    (global.removeFromLeft (cellW));
    globalMix.setBounds (global.removeFromLeft (cellW));
    outGain.setBounds   (global);

    r.removeFromTop (10);

    // Grid 2 rows x 3 cols
    auto grid = r;
    auto rowH = (grid.getHeight() - 10) / 2;
    auto row1 = grid.removeFromTop (rowH);
    grid.removeFromTop (10);
    auto row2 = grid;

    auto colW = (row1.getWidth() - 20) / 3;
    auto c11 = row1.removeFromLeft (colW); row1.removeFromLeft (10);
    auto c12 = row1.removeFromLeft (colW); row1.removeFromLeft (10);
    auto c13 = row1;

    auto c21 = row2.removeFromLeft (colW); row2.removeFromLeft (10);
    auto c22 = row2.removeFromLeft (colW); row2.removeFromLeft (10);
    auto c23 = row2;

    sections = {{
        { "FILTER",  c11 },
        { "DIST",    c12 },
        { "CHORUS",  c13 },
        { "FLANGER", c21 },
        { "DELAY",   c22 },
        { "REVERB",  c23 },
    }};

    auto place = [] (juce::Rectangle<int> area, juce::ToggleButton& on, std::initializer_list<juce::Component*> knobs)
    {
        area = area.reduced (12);
        on.setBounds (area.removeFromTop (22));
        area.removeFromTop (8);

        const int n = (int)knobs.size();
        const int w = area.getWidth() / juce::jmax (1, n);

        for (auto* k : knobs)
            k->setBounds (area.removeFromLeft (w).reduced (6));
    };

    place (c11, filtOn, { &hpf, &lpf, &filtMix });
    place (c12, distOn, { &distDrive, &distMix });
    place (c13, choOn,  { &choRate, &choDepth, &choMix });

    place (c21, flaOn,  { &flaRate, &flaDepth, &flaFb, &flaMix });
    place (c22, dlyOn,  { &dlyTime, &dlyFb, &dlyMix });
    place (c23, revOn,  { &revSize, &revDamp, &revMix });
}
