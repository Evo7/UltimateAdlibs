#pragma once
#include <JuceHeader.h>

class CLALookAndFeel : public juce::LookAndFeel_V4
{
public:
    CLALookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (0.9f));
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xFF141414));

        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF141414));
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xFF2A2A2A));
        setColour (juce::ComboBox::textColourId, juce::Colours::white.withAlpha (0.9f));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                           float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced (6.0f);
        auto r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        auto centre = bounds.getCentre();
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Base
        g.setColour (juce::Colour (0xFF1B1B1B));
        g.fillEllipse (bounds);

        // Rim
        g.setColour (juce::Colour (0xFF2A2A2A));
        g.drawEllipse (bounds, 2.0f);

        // Value arc (amber)
        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y, r - 3.0f, r - 3.0f, 0.0f, rotaryStartAngle, angle, true);
        g.setColour (juce::Colour (0xFFFFD36A));
        g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Pointer
        juce::Path p;
        p.addRoundedRectangle (-2.0f, -r + 8.0f, 4.0f, r * 0.55f, 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.fillPath (p, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));

        // Centre cap
        g.setColour (juce::Colour (0xFF0E0E0E));
        g.fillEllipse (bounds.withSizeKeepingCentre (10.0f, 10.0f));
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                           bool, bool) override
    {
        auto r = b.getLocalBounds().toFloat();
        auto led = r.removeFromLeft (26.0f).reduced (6.0f);

        const bool on = b.getToggleState();
        auto ledCol = on ? juce::Colour (0xFF3DFF7A) : juce::Colour (0xFFFF3D3D);

        // Glow
        g.setColour (ledCol.withAlpha (0.25f));
        g.fillEllipse (led.expanded (4.0f));

        // LED
        g.setColour (ledCol);
        g.fillEllipse (led);

        // Label
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.setFont (juce::Font (14.0f, juce::Font::bold));
        g.drawText (b.getButtonText(), r.toNearestInt(), juce::Justification::centredLeft);
    }
};