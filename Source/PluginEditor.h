#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CLALookAndFeel.h"

// ===== Simple VU meter component (vertical bar) =====
class VUMeter  : public juce::Component, private juce::Timer
{
public:
    explicit VUMeter (std::function<float()> valueFnIn)
        : valueFn (std::move (valueFnIn))
    {
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();

        // background
        g.setColour (juce::Colour (0xFF101010));
        g.fillRoundedRectangle (r, 10.0f);

        g.setColour (juce::Colour (0xFF2A2A2A));
        g.drawRoundedRectangle (r, 10.0f, 1.5f);

        // value (linear RMS -> dB scale display)
        float v = juce::jlimit (0.0f, 2.0f, currentValue);
        float db = juce::Decibels::gainToDecibels (v, -60.0f); // [-60..+]
        // map [-60..0] dB to [0..1]
        float norm = juce::jmap (db, -60.0f, 0.0f, 0.0f, 1.0f);
        norm = juce::jlimit (0.0f, 1.0f, norm);

        auto inner = r.reduced (6.0f);
        auto filled = inner.withY (inner.getY() + inner.getHeight() * (1.0f - norm));
        filled.setHeight (inner.getHeight() * norm);

        // gradient-ish segments
        g.setColour (juce::Colour (0xFF3DFF7A).withAlpha (0.85f));
        g.fillRoundedRectangle (filled, 8.0f);

        // clip line at "red zone" near top
        auto redLineY = inner.getY() + inner.getHeight() * 0.12f;
        g.setColour (juce::Colour (0xFFFF3D3D).withAlpha (0.7f));
        g.drawLine (inner.getX(), redLineY, inner.getRight(), redLineY, 1.0f);
    }

private:
    void timerCallback() override
    {
        if (valueFn)
            currentValue = valueFn();
        repaint();
    }

    std::function<float()> valueFn;
    float currentValue = 0.0f;
};

class UltimateAdlibsAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    UltimateAdlibsAudioProcessorEditor (UltimateAdlibsAudioProcessor&);
    ~UltimateAdlibsAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    UltimateAdlibsAudioProcessor& audioProcessor;
    CLALookAndFeel lnf;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void makeKnob (juce::Slider& s);

    // Top bar
    juce::Label title;
    juce::ComboBox presetBox; // placeholder

    // VU meters (IN/OUT)
    VUMeter inVu  { [this]{ return audioProcessor.getInputMeter();  } };
    VUMeter outVu { [this]{ return audioProcessor.getOutputMeter(); } };
    juce::Label inLbl, outLbl;

    // Global
    juce::Slider inGain, globalMix, outGain;
    std::unique_ptr<SliderAttachment> inGainA, globalMixA, outGainA;

    // ON/OFF
    juce::ToggleButton filtOn, distOn, choOn, flaOn, dlyOn, revOn;
    std::unique_ptr<ButtonAttachment> filtOnA, distOnA, choOnA, flaOnA, dlyOnA, revOnA;

    // Filters
    juce::Slider hpf, lpf, filtMix;
    std::unique_ptr<SliderAttachment> hpfA, lpfA, filtMixA;

    // Dist
    juce::Slider distDrive, distMix;
    std::unique_ptr<SliderAttachment> distDriveA, distMixA;

    // Chorus
    juce::Slider choRate, choDepth, choMix;
    std::unique_ptr<SliderAttachment> choRateA, choDepthA, choMixA;

    // Flanger
    juce::Slider flaRate, flaDepth, flaFb, flaMix;
    std::unique_ptr<SliderAttachment> flaRateA, flaDepthA, flaFbA, flaMixA;

    // Delay
    juce::Slider dlyTime, dlyFb, dlyMix;
    std::unique_ptr<SliderAttachment> dlyTimeA, dlyFbA, dlyMixA;

    // Reverb
    juce::Slider revSize, revDamp, revMix;
    std::unique_ptr<SliderAttachment> revSizeA, revDampA, revMixA;

    struct Section { juce::String name; juce::Rectangle<int> area; };
    std::array<Section, 6> sections;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UltimateAdlibsAudioProcessorEditor)
};
