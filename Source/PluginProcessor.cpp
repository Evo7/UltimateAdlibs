#include "PluginProcessor.h"
#include "PluginEditor.h"

static constexpr float twoPi = 6.2831853071795864769f;

UltimateAdlibsAudioProcessor::UltimateAdlibsAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor (BusesProperties()
    #if ! JucePlugin_IsMidiEffect
     #if ! JucePlugin_IsSynth
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
     #endif
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
    #endif
        ),
#endif
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    waveshaper.functionToUse = [] (float x) { return std::tanh (x); };
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool UltimateAdlibsAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
   #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
   #else
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (in != out)
        return false;
   #endif

    return true;
   #endif
}
#endif

UltimateAdlibsAudioProcessor::APVTS::ParameterLayout
UltimateAdlibsAudioProcessor::createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto pct = NormalisableRange<float> (0.f, 100.f, 0.01f);
    auto db  = NormalisableRange<float> (-24.f, 24.f, 0.01f);
    auto hz  = NormalisableRange<float> (20.f, 20000.f, 1.f, 0.5f);

    p.push_back (std::make_unique<AudioParameterFloat> ("IN_GAIN",    "Input Gain",  db, 0.f));
    p.push_back (std::make_unique<AudioParameterFloat> ("OUT_GAIN",   "Output Gain", db, 0.f));
    p.push_back (std::make_unique<AudioParameterFloat> ("GLOBAL_MIX", "Global Mix",  pct, 100.f));

    p.push_back (std::make_unique<AudioParameterBool>  ("FILT_ON",  "Filters On", true));
    p.push_back (std::make_unique<AudioParameterFloat> ("HPF_HZ",   "HPF (Hz)", hz, 120.f));
    p.push_back (std::make_unique<AudioParameterFloat> ("LPF_HZ",   "LPF (Hz)", hz, 16000.f));
    p.push_back (std::make_unique<AudioParameterFloat> ("FILT_MIX", "Filters Mix", pct, 100.f));

    p.push_back (std::make_unique<AudioParameterBool>  ("DIST_ON",   "Dist On", true));
    p.push_back (std::make_unique<AudioParameterFloat> ("DIST_DRIVE","Drive (dB)", NormalisableRange<float>(0.f, 24.f, 0.01f), 6.f));
    p.push_back (std::make_unique<AudioParameterFloat> ("DIST_MIX",  "Dist Mix", pct, 30.f));

    p.push_back (std::make_unique<AudioParameterBool>  ("CHO_ON",   "Chorus On", true));
    p.push_back (std::make_unique<AudioParameterFloat> ("CHO_RATE", "Chorus Rate", NormalisableRange<float>(0.05f, 8.f, 0.001f, 0.5f), 0.8f));
    p.push_back (std::make_unique<AudioParameterFloat> ("CHO_DEPTH","Chorus Depth", NormalisableRange<float>(0.f, 1.f, 0.001f), 0.25f));
    p.push_back (std::make_unique<AudioParameterFloat> ("CHO_MIX",  "Chorus Mix", pct, 25.f));

    p.push_back (std::make_unique<AudioParameterBool>  ("FLA_ON",   "Flanger On", true));
    p.push_back (std::make_unique<AudioParameterFloat> ("FLA_RATE", "Flanger Rate", NormalisableRange<float>(0.05f, 5.f, 0.001f, 0.5f), 0.35f));
    p.push_back (std::make_unique<AudioParameterFloat> ("FLA_DEPTH","Flanger Depth", NormalisableRange<float>(0.f, 1.f, 0.001f), 0.6f));
    p.push_back (std::make_unique<AudioParameterFloat> ("FLA_FB",   "Flanger FB", NormalisableRange<float>(-0.95f, 0.95f, 0.001f), 0.2f));
    p.push_back (std::make_unique<AudioParameterFloat> ("FLA_MIX",  "Flanger Mix", pct, 20.f));

    p.push_back (std::make_unique<AudioParameterBool>  ("DLY_ON",   "Delay On", true));
    p.push_back (std::make_unique<AudioParameterFloat> ("DLY_TIME", "Delay Time (ms)", NormalisableRange<float>(1.f, 1200.f, 0.01f, 0.5f), 220.f));
    p.push_back (std::make_unique<AudioParameterFloat> ("DLY_FB",   "Delay Feedback", NormalisableRange<float>(0.f, 0.95f, 0.001f), 0.35f));
    p.push_back (std::make_unique<AudioParameterFloat> ("DLY_MIX",  "Delay Mix", pct, 22.f));

    p.push_back (std::make_unique<AudioParameterBool>  ("REV_ON",   "Reverb On", true));
    p.push_back (std::make_unique<AudioParameterFloat> ("REV_SIZE", "Room Size", NormalisableRange<float>(0.f, 1.f, 0.001f), 0.35f));
    p.push_back (std::make_unique<AudioParameterFloat> ("REV_DAMP", "Damping", NormalisableRange<float>(0.f, 1.f, 0.001f), 0.5f));
    p.push_back (std::make_unique<AudioParameterFloat> ("REV_MIX",  "Reverb Mix", pct, 18.f));

    return { p.begin(), p.end() };
}

void UltimateAdlibsAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = (float) sampleRate;

    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = (juce::uint32) juce::jmax (1, getTotalNumOutputChannels());

    dryBuffer.setSize (getTotalNumOutputChannels(), samplesPerBlock);
    tempBuffer.setSize (getTotalNumOutputChannels(), samplesPerBlock);

    hpf.reset(); lpf.reset();
    hpf.prepare (spec);
    lpf.prepare (spec);

    chorus.prepare (spec);
    chorus.reset();

    flangerDL.prepare (spec); flangerDR.prepare (spec);
    flangerDL.reset(); flangerDR.reset();
    flangerPhase = 0.0f;

    delayL.prepare (spec); delayR.prepare (spec);
    delayL.reset(); delayR.reset();

    reverb.reset();

    inMeter.store (0.0f);
    outMeter.store (0.0f);

    updateDSP();
}

void UltimateAdlibsAudioProcessor::updateDSP()
{
    const float hpfHz = apvts.getRawParameterValue ("HPF_HZ")->load();
    const float lpfHz = apvts.getRawParameterValue ("LPF_HZ")->load();

    *hpf.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, hpfHz);
    *lpf.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass  (sr, lpfHz);

    chorus.setRate (apvts.getRawParameterValue ("CHO_RATE")->load());
    chorus.setDepth (apvts.getRawParameterValue ("CHO_DEPTH")->load());
    chorus.setCentreDelay (7.0f);
    chorus.setFeedback (0.0f);
    chorus.setMix (1.0f);

    revParams.roomSize = apvts.getRawParameterValue ("REV_SIZE")->load();
    revParams.damping  = apvts.getRawParameterValue ("REV_DAMP")->load();
    revParams.width    = 1.0f;
    revParams.wetLevel = 1.0f;
    revParams.dryLevel = 0.0f;
    reverb.setParameters (revParams);
}

void UltimateAdlibsAudioProcessor::updateMeterAtomic (std::atomic<float>& dst, float newValue)
{
    newValue = juce::jmax (0.0f, newValue);
    const float prev = dst.load (std::memory_order_relaxed);
    const float decayed = prev * meterHold;
    const float out = (newValue > decayed) ? newValue : decayed;
    dst.store (out, std::memory_order_relaxed);
}

static float computeRmsStereo (const juce::AudioBuffer<float>& b, int numCh)
{
    const int n = b.getNumSamples();
    if (n <= 0 || numCh <= 0) return 0.0f;

    double sum = 0.0;
    int count = 0;

    for (int ch = 0; ch < numCh; ++ch)
    {
        const float* x = b.getReadPointer (ch);
        for (int i = 0; i < n; ++i)
        {
            const double v = (double) x[i];
            sum += v * v;
        }
        count += n;
    }

    if (count <= 0) return 0.0f;
    return (float) std::sqrt (sum / (double) count);
}

void UltimateAdlibsAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();

    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, numSamples);

    const int numCh = juce::jmin (2, numOut);

    // Dry copy before anything (also used for IN meter)
    dryBuffer.makeCopyOf (buffer, true);

    // IN meter (pre gain)
    updateMeterAtomic (inMeter, computeRmsStereo (dryBuffer, numCh));

    updateDSP();

    // Input gain
    buffer.applyGain (dbToGain (apvts.getRawParameterValue ("IN_GAIN")->load()));

    auto mixWetFromTemp = [&] (float mix01)
    {
        mix01 = clamp01 (mix01);
        if (mix01 <= 0.000001f) return;

        for (int ch = 0; ch < numCh; ++ch)
        {
            buffer.applyGain (ch, 0, numSamples, 1.0f - mix01);
            buffer.addFrom  (ch, 0, tempBuffer, ch, 0, numSamples, mix01);
        }
    };

    // 1) FILTERS
    {
        const bool on  = apvts.getRawParameterValue ("FILT_ON")->load() > 0.5f;
        const float mix = apvts.getRawParameterValue ("FILT_MIX")->load() / 100.0f;

        if (on && mix > 0.0001f)
        {
            tempBuffer.makeCopyOf (buffer, true);
            juce::dsp::AudioBlock<float> block (tempBuffer);
            auto ctx = juce::dsp::ProcessContextReplacing<float> (block);
            hpf.process (ctx);
            lpf.process (ctx);
            mixWetFromTemp (mix);
        }
    }

    // 2) DIST
    {
        const bool on  = apvts.getRawParameterValue ("DIST_ON")->load() > 0.5f;
        const float mix = apvts.getRawParameterValue ("DIST_MIX")->load() / 100.0f;
        const float driveDb = apvts.getRawParameterValue ("DIST_DRIVE")->load();
        const float drive = dbToGain (driveDb);

        if (on && mix > 0.0001f)
        {
            tempBuffer.makeCopyOf (buffer, true);
            tempBuffer.applyGain (drive);

            for (int ch = 0; ch < numCh; ++ch)
            {
                auto* x = tempBuffer.getWritePointer (ch);
                for (int i = 0; i < numSamples; ++i)
                    x[i] = waveshaper.processSample (x[i]);
            }

            mixWetFromTemp (mix);
        }
    }

    // 3) CHORUS
    {
        const bool on  = apvts.getRawParameterValue ("CHO_ON")->load() > 0.5f;
        const float mix = apvts.getRawParameterValue ("CHO_MIX")->load() / 100.0f;

        if (on && mix > 0.0001f)
        {
            tempBuffer.makeCopyOf (buffer, true);
            juce::dsp::AudioBlock<float> block (tempBuffer);
            chorus.process (juce::dsp::ProcessContextReplacing<float> (block));
            mixWetFromTemp (mix);
        }
    }

    // 4) FLANGER
    {
        const bool on  = apvts.getRawParameterValue ("FLA_ON")->load() > 0.5f;
        const float mix = apvts.getRawParameterValue ("FLA_MIX")->load() / 100.0f;
        const float rate  = apvts.getRawParameterValue ("FLA_RATE")->load();
        const float depth = apvts.getRawParameterValue ("FLA_DEPTH")->load();
        const float fb    = apvts.getRawParameterValue ("FLA_FB")->load();

        if (on && mix > 0.0001f && numCh >= 1)
        {
            tempBuffer.makeCopyOf (buffer, true);

            const float minDelayMs = 0.2f;
            const float maxDelayMs = 8.0f;
            const float phaseInc = (twoPi * rate) / sr;

            auto* l = tempBuffer.getWritePointer (0);
            auto* r = (numCh > 1) ? tempBuffer.getWritePointer (1) : nullptr;

            for (int i = 0; i < numSamples; ++i)
            {
                const float lfo = 0.5f * (1.0f + std::sin (flangerPhase));
                const float dMs = juce::jmap (lfo * depth, minDelayMs, maxDelayMs);
                const float dSamp = (dMs / 1000.0f) * sr;

                const float dl = flangerDL.popSample (0, dSamp);
                const float inL = l[i] + dl * fb;
                flangerDL.pushSample (0, inL);
                l[i] = l[i] + dl;

                if (r)
                {
                    const float dr = flangerDR.popSample (0, dSamp);
                    const float inR = r[i] + dr * fb;
                    flangerDR.pushSample (0, inR);
                    r[i] = r[i] + dr;
                }

                flangerPhase += phaseInc;
                if (flangerPhase >= twoPi) flangerPhase -= twoPi;
            }

            mixWetFromTemp (mix);
        }
    }

    // 5) DELAY
    {
        const bool on  = apvts.getRawParameterValue ("DLY_ON")->load() > 0.5f;
        const float mix = apvts.getRawParameterValue ("DLY_MIX")->load() / 100.0f;
        const float timeMs = apvts.getRawParameterValue ("DLY_TIME")->load();
        const float fb     = apvts.getRawParameterValue ("DLY_FB")->load();

        if (on && mix > 0.0001f && numCh >= 1)
        {
            tempBuffer.makeCopyOf (buffer, true);
            const float dSamp = (timeMs / 1000.0f) * sr;

            auto* l = tempBuffer.getWritePointer (0);
            auto* r = (numCh > 1) ? tempBuffer.getWritePointer (1) : nullptr;

            for (int i = 0; i < numSamples; ++i)
            {
                const float dl = delayL.popSample (0, dSamp);
                delayL.pushSample (0, l[i] + dl * fb);
                l[i] = dl;

                if (r)
                {
                    const float dr = delayR.popSample (0, dSamp);
                    delayR.pushSample (0, r[i] + dr * fb);
                    r[i] = dr;
                }
            }

            mixWetFromTemp (mix);
        }
    }

    // 6) REVERB
    {
        const bool on  = apvts.getRawParameterValue ("REV_ON")->load() > 0.5f;
        const float mix = apvts.getRawParameterValue ("REV_MIX")->load() / 100.0f;

        if (on && mix > 0.0001f)
        {
            tempBuffer.makeCopyOf (buffer, true);
            juce::dsp::AudioBlock<float> block (tempBuffer);
            reverb.process (juce::dsp::ProcessContextReplacing<float> (block));
            mixWetFromTemp (mix);
        }
    }

    // Global wet/dry
    const float globalMix = apvts.getRawParameterValue ("GLOBAL_MIX")->load() / 100.0f;
    if (globalMix < 0.9999f)
    {
        const float gm = clamp01 (globalMix);
        for (int ch = 0; ch < numCh; ++ch)
        {
            buffer.applyGain (ch, 0, numSamples, gm);
            buffer.addFrom  (ch, 0, dryBuffer, ch, 0, numSamples, 1.0f - gm);
        }
    }

    // Output gain
    buffer.applyGain (dbToGain (apvts.getRawParameterValue ("OUT_GAIN")->load()));

    // OUT meter (post gain)
    updateMeterAtomic (outMeter, computeRmsStereo (buffer, numCh));
}

juce::AudioProcessorEditor* UltimateAdlibsAudioProcessor::createEditor()
{
    return new UltimateAdlibsAudioProcessorEditor (*this);
}

void UltimateAdlibsAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void UltimateAdlibsAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new UltimateAdlibsAudioProcessor();
}
