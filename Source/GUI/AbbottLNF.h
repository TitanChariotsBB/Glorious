#include <BinaryData.h>
#include <JuceHeader.h>

class AbbottLNF : public juce::LookAndFeel_V4 {
public:
	AbbottLNF();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;

private:
    std::unique_ptr<juce::Drawable> knob = juce::Drawable::createFromImageData(BinaryData::knob_svg, BinaryData::knob_svgSize);
    std::unique_ptr<juce::Drawable> pointer = juce::Drawable::createFromImageData(BinaryData::pointer_svg, BinaryData::pointer_svgSize);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AbbottLNF)
};