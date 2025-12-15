#pragma once
#include <JuceHeader.h>
#include <atomic>

class UltimateAdlibsAudioProcessor : public juce::AudioProcessor
{
public:
    UltimateAdlibsAudioProcessor();
    ~UltimateAdlibsAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS apvts;
    static APVTS::ParameterLayout createParameterLayout();

    // ===== Meters (linear RMS 0..~1) =====
    float getInputMeter()  const noexcept { return inMeter.load (std::memory_order_relaxed); }
    float getOutputMeter() const noexcept { return outMeter.load (std::memory_order_relaxed); }

private:
    juce::dsp::ProcessSpec spec {};
    float sr = 44100.0f;

    // Buffers
    juce::AudioBuffer<float> dryBuffer;
    juce::AudioBuffer<float> tempBuffer;

    // ===== Filters (JUCE 8 friendly) =====
    using IIRFilter = juce::dsp::IIR::Filter<float>;
    using IIRCoeffs = juce::dsp::IIR::Coefficients<float>;
    using IIRStereo = juce::dsp::ProcessorDuplicator<IIRFilter, IIRCoeffs>;

    IIRStereo hpf;
    IIRStereo lpf;

    // ===== Distortion =====
    juce::dsp::WaveShaper<float> waveshaper;

    // ===== Chorus =====
    juce::dsp::Chorus<float> chorus;

    // ===== Flanger =====
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> flangerDL { 8192 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> flangerDR { 8192 };
    float flangerPhase = 0.0f;

    // ===== Delay =====
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayL { 262144 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayR { 262144 };

    // ===== Reverb =====
    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters revParams;

    // ===== Meter state =====
    std::atomic<float> inMeter  { 0.0f };
    std::atomic<float> outMeter { 0.0f };
    float meterHold = 0.92f; // simple decay per block

    void updateDSP();

    static float dbToGain (float db) { return juce::Decibels::decibelsToGain (db); }
    static float clamp01 (float x)   { return juce::jlimit (0.0f, 1.0f, x); }

    void updateMeterAtomic (std::atomic<float>& dst, float newValue);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UltimateAdlibsAudioProcessor)
};
