#include "AbbottLNF.h"

AbbottLNF::AbbottLNF() {}

void AbbottLNF::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    int diameter = (width > height) ? height : width;
    if (diameter < 16)
        return;

    juce::Point<float> centre(x + std::floor(width * 0.5f + 0.5f), y + std::floor(height * 0.5f + 0.5f));
    diameter -= (diameter % 2 == 1) ? 9 : 8;
    float radius = (float)diameter * 0.5f;
    x = int(centre.x - radius);
    y = int(centre.y - radius);

    const auto bounds = juce::Rectangle<int>(x, y, diameter, diameter).toFloat();

    auto b = pointer->getBounds().toFloat();
    pointer->setTransform(juce::AffineTransform::rotation(juce::MathConstants<float>::twoPi * ((sliderPos - 0.5f) * 300.0f / 360.0f),
        b.getCentreX(),
        b.getCentreY()));

    const auto alpha = slider.isEnabled() ? 1.0f : 0.4f;

    auto knobBounds = (bounds * 0.75f).withCentre(centre);
    knob->drawWithin(g, knobBounds, juce::RectanglePlacement::stretchToFit, alpha);
    pointer->drawWithin(g, knobBounds, juce::RectanglePlacement::stretchToFit, alpha);

    const auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    constexpr float arcFactor = 0.9f;

    juce::Path valueArc;
    valueArc.addPieSegment(bounds, rotaryStartAngle, rotaryEndAngle, arcFactor);
    g.setColour(juce::Colour(0xff595c6b).withAlpha(alpha));
    g.fillPath(valueArc);
    valueArc.clear();

    valueArc.addPieSegment(bounds, rotaryStartAngle, toAngle, arcFactor);
    g.setColour(juce::Colour(0xfffde6af).withAlpha(alpha));
    g.fillPath(valueArc);
}