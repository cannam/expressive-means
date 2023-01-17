
/*
    Expressive Means Portamento

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Portamento.h"

#include <vector>
#include <set>

using std::cerr;
using std::endl;
using std::vector;
using std::set;

static const CoreFeatures::Parameters defaultCoreParams;

static const float default_sustainBeginThreshold_ms = 50.f;
static const float default_sustainEndThreshold_dBFS = -45.f;
static const float default_volumeDevelopmentThreshold_dB = 2.f;
static const float default_scalingFactor = 10.7f;

Portamento::Portamento(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_stepSize(0),
    m_blockSize(0),
    m_haveStartTime(false),
    m_coreFeatures(inputSampleRate),
    m_pyinThresholdDistribution(defaultCoreParams.pyinParameters.thresholdDistribution),
    m_pyinLowAmpSuppression(defaultCoreParams.pyinParameters.lowAmplitudeSuppressionThreshold),
    m_pitchAverageWindow_ms(defaultCoreParams.pitchAverageWindow_ms),
    m_onsetSensitivityPitch_cents(defaultCoreParams.onsetSensitivityPitch_cents),
    m_onsetSensitivityNoise_percent(defaultCoreParams.onsetSensitivityNoise_percent),
    m_onsetSensitivityLevel_dB(defaultCoreParams.onsetSensitivityLevel_dB),
    m_onsetSensitivityNoiseTimeWindow_ms(defaultCoreParams.onsetSensitivityNoiseTimeWindow_ms),
    m_minimumOnsetInterval_ms(defaultCoreParams.minimumOnsetInterval_ms),
    m_sustainBeginThreshold_ms(default_sustainBeginThreshold_ms),
    m_sustainEndThreshold_dBFS(default_sustainEndThreshold_dBFS),
    m_volumeDevelopmentThreshold_dB(default_volumeDevelopmentThreshold_dB),
    m_scalingFactor(default_scalingFactor),
    m_summaryOutput(-1),
    m_portamentoTypeOutput(-1),
    m_pitchTrackOutput(-1),
    m_portamentoIndexOutput(-1)
{
}

Portamento::~Portamento()
{
}

string
Portamento::getIdentifier() const
{
    return "portamento";
}

string
Portamento::getName() const
{
    return "Expressive Means: Portamento";
}

string
Portamento::getDescription() const
{
    return "";
}

string
Portamento::getMaker() const
{
    return "Frithjof Vollmer and Chris Cannam";
}

int
Portamento::getPluginVersion() const
{
    return 1;
}

string
Portamento::getCopyright() const
{
    return "GPLv2";
}

Portamento::InputDomain
Portamento::getInputDomain() const
{
    return TimeDomain;
}

size_t
Portamento::getPreferredBlockSize() const
{
    return m_coreFeatures.getPreferredBlockSize();
}

size_t 
Portamento::getPreferredStepSize() const
{
    return m_coreFeatures.getPreferredStepSize();
}

size_t
Portamento::getMinChannelCount() const
{
    return 1;
}

size_t
Portamento::getMaxChannelCount() const
{
    return 1;
}

Portamento::ParameterList
Portamento::getParameterDescriptors() const
{
    ParameterList list;

    ParameterList pyinParams = m_coreFeatures.getPYinParameterDescriptors();
    for (auto d: pyinParams) {
        if (d.identifier == "threshdistr" ||
            d.identifier == "lowampsuppression") {
            d.identifier = "pyin-" + d.identifier;
            d.name = "pYIN: " + d.name;
            list.push_back(d);
        }
    }
    
    ParameterDescriptor d;

    d.description = "";
    d.isQuantized = false;
    
    d.identifier = "pitchAverageWindow";
    d.name = "Moving pitch average window";
    d.unit = "ms";
    d.minValue = 20.f;
    d.maxValue = 1000.f;
    d.defaultValue = defaultCoreParams.pitchAverageWindow_ms;
    list.push_back(d);

    d.identifier = "onsetSensitivityPitch";
    d.name = "Onset sensitivity: Pitch";
    d.unit = "cents";
    d.minValue = 0.f;
    d.maxValue = 100.f;
    d.defaultValue = defaultCoreParams.onsetSensitivityPitch_cents;
    list.push_back(d);
    
    d.identifier = "onsetSensitivityNoise";
    d.name = "Onset sensitivity: Noise";
    d.unit = "%";
    d.minValue = 0.f;
    d.maxValue = 100.f;
    d.defaultValue = defaultCoreParams.onsetSensitivityNoise_percent;
    list.push_back(d);
    
    d.identifier = "onsetSensitivityLevel";
    d.name = "Onset sensitivity: Level";
    d.unit = "dB";
    d.minValue = 0.f;
    d.maxValue = 100.f;
    d.defaultValue = defaultCoreParams.onsetSensitivityLevel_dB;
    list.push_back(d);
    
    d.identifier = "onsetSensitivityNoiseTimeWindow";
    d.name = "Onset sensitivity: Noise time window";
    d.unit = "ms";
    d.minValue = 20.f;
    d.maxValue = 500.f;
    d.defaultValue = defaultCoreParams.onsetSensitivityNoiseTimeWindow_ms;
    list.push_back(d);
    
    d.identifier = "minimumOnsetInterval";
    d.name = "Minimum onset interval";
    d.unit = "ms";
    d.minValue = 0.f;
    d.maxValue = 1000.f;
    d.defaultValue = defaultCoreParams.minimumOnsetInterval_ms;
    list.push_back(d);
    
    return list;
}

float
Portamento::getParameter(string identifier) const
{
    if (identifier == "pyin-threshdistr") {
        return m_pyinThresholdDistribution;
    } else if (identifier == "pyin-lowampsuppression") {
        return m_pyinLowAmpSuppression;
    } else if (identifier == "pitchAverageWindow") {
        return m_pitchAverageWindow_ms;
    } else if (identifier == "onsetSensitivityPitch") {
        return m_onsetSensitivityPitch_cents;
    } else if (identifier == "onsetSensitivityNoise") {
        return m_onsetSensitivityNoise_percent;
    } else if (identifier == "onsetSensitivityLevel") {
        return m_onsetSensitivityLevel_dB;
    } else if (identifier == "onsetSensitivityNoiseTimeWindow") {
        return m_onsetSensitivityNoiseTimeWindow_ms;
    } else if (identifier == "minimumOnsetInterval") {
        return m_minimumOnsetInterval_ms;
    }
    
    return 0.f;
}

void
Portamento::setParameter(string identifier, float value) 
{
    if (identifier == "pyin-threshdistr") {
        m_pyinThresholdDistribution = value;
    } else if (identifier == "pyin-lowampsuppression") {
        m_pyinLowAmpSuppression = value;
    } else if (identifier == "pitchAverageWindow") {
        m_pitchAverageWindow_ms = value;
    } else if (identifier == "onsetSensitivityPitch") {
        m_onsetSensitivityPitch_cents = value;
    } else if (identifier == "onsetSensitivityNoise") {
        m_onsetSensitivityNoise_percent = value;
    } else if (identifier == "onsetSensitivityLevel") {
        m_onsetSensitivityLevel_dB = value;
    } else if (identifier == "onsetSensitivityNoiseTimeWindow") {
        m_onsetSensitivityNoiseTimeWindow_ms = value;
    } else if (identifier == "minimumOnsetInterval") {
        m_minimumOnsetInterval_ms = value;
    }
}

Portamento::ProgramList
Portamento::getPrograms() const
{
    ProgramList list;
    return list;
}

string
Portamento::getCurrentProgram() const
{
    return ""; 
}

void
Portamento::selectProgram(string)
{
}

Portamento::OutputList
Portamento::getOutputDescriptors() const
{
    OutputList list;
    OutputDescriptor d;
/*!!!    
    d.identifier = "summary";
    d.name = "Summary";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.hasDuration = false;
    m_summaryOutput = int(list.size());
    list.push_back(d);
    
    d.identifier = "portamentoType";
    d.name = "Portamento Type";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 0;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.hasDuration = false;
    m_portamentoTypeOutput = int(list.size());
    list.push_back(d);
    
    d.identifier = "pitchTrack";
    d.name = "Pitch Track";
    d.description = "The smoothed pitch track computed by pYIN.";
    d.unit = "Hz";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = (m_inputSampleRate / m_stepSize);
    d.hasDuration = false;
    m_pitchTrackOutput = int(list.size());
    list.push_back(d);
    
    d.identifier = "portamentoIndex";
    d.name = "Portamento Index";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.hasDuration = false;
    m_portamentoIndexOutput = int(list.size());
    list.push_back(d);
*/
    
    return list;
}

bool
Portamento::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    if (channels < getMinChannelCount() || channels > getMaxChannelCount()) {
        cerr << "ERROR: Portamento::initialise: unsupported channel count "
             << channels << endl;
        return false;
    }

    if (m_inputSampleRate < 8000.0) {
        cerr << "ERROR: Portamento::initialise: sample rate ("
             << m_inputSampleRate << ") is too low, it must be at least 8kHz"
             << endl;
        return false;
    }
    
    if (m_inputSampleRate > 192000.0) {
        cerr << "ERROR: Portamento::initialise: sample rate ("
             << m_inputSampleRate << ") is too high, maximum is 192kHz"
             << endl;
        return false;
    }
    
    if (stepSize > blockSize) {
        cerr << "ERROR: Portamento::initialise: step size (" << stepSize
             << ") may not exceed block size (" << blockSize << ")" << endl;
        return false;
    }

    if (m_summaryOutput < 0) {
        (void)getOutputDescriptors(); // initialise output indices
    }
    
    m_stepSize = stepSize;
    m_blockSize = blockSize;
    m_haveStartTime = false;

    try {
        CoreFeatures::PYinParameters pyinParams;
        pyinParams.thresholdDistribution = m_pyinThresholdDistribution;
        pyinParams.lowAmplitudeSuppressionThreshold = m_pyinLowAmpSuppression;
    
        Power::Parameters powerParams;
        powerParams.blockSize = m_blockSize;

        int onsetLevelRiseHistoryLength =
            m_coreFeatures.msToSteps(m_onsetSensitivityNoiseTimeWindow_ms,
                                         m_stepSize, false);
        if (onsetLevelRiseHistoryLength < 2) {
            onsetLevelRiseHistoryLength = 2;
        }

        SpectralLevelRise::Parameters onsetLevelRiseParameters;
        onsetLevelRiseParameters.sampleRate = m_inputSampleRate;
        onsetLevelRiseParameters.blockSize = m_blockSize;
        onsetLevelRiseParameters.dB = m_onsetSensitivityLevel_dB;
        onsetLevelRiseParameters.historyLength = onsetLevelRiseHistoryLength;

        SpectralLevelRise::Parameters noiseRatioLevelRiseParameters;
        noiseRatioLevelRiseParameters.sampleRate = m_inputSampleRate;
        noiseRatioLevelRiseParameters.blockSize = m_blockSize;
        noiseRatioLevelRiseParameters.dB = 20.0;
        noiseRatioLevelRiseParameters.historyLength =
            ceil(0.05 * m_inputSampleRate / m_stepSize);

        CoreFeatures::Parameters fParams;
        fParams.pyinParameters = pyinParams;
        fParams.powerParameters = powerParams;
        fParams.onsetLevelRiseParameters = onsetLevelRiseParameters;
        fParams.noiseRatioLevelRiseParameters = noiseRatioLevelRiseParameters;
        fParams.stepSize = m_stepSize;
        fParams.blockSize = m_blockSize;
        fParams.pitchAverageWindow_ms = m_pitchAverageWindow_ms;
        fParams.onsetSensitivityPitch_cents = m_onsetSensitivityPitch_cents;
        fParams.onsetSensitivityNoise_percent = m_onsetSensitivityNoise_percent;
        fParams.onsetSensitivityLevel_dB = m_onsetSensitivityLevel_dB;
        fParams.onsetSensitivityNoiseTimeWindow_ms = m_onsetSensitivityNoiseTimeWindow_ms;
        fParams.minimumOnsetInterval_ms = m_minimumOnsetInterval_ms;

        m_coreFeatures.initialise(fParams);
    
    } catch (const std::logic_error &e) {
        cerr << "ERROR: Portamento::initialise: Feature extractor initialisation failed: " << e.what() << endl;
        return false;
    }
    
    return true;
}

void
Portamento::reset()
{
    m_haveStartTime = false;
    m_coreFeatures.reset();
}

Portamento::FeatureSet
Portamento::process(const float *const *inputBuffers, Vamp::RealTime timestamp)
{
    if (!m_haveStartTime) {
        m_startTime = timestamp;
        m_haveStartTime = true;
    }

    m_coreFeatures.process(inputBuffers[0], timestamp);
    return {};
}

Portamento::FeatureSet
Portamento::getRemainingFeatures()
{
    FeatureSet fs;

    m_coreFeatures.finish();

    auto pyinPitch = m_coreFeatures.getPYinPitch_Hz();
    auto pyinTimestamps = m_coreFeatures.getPYinTimestamps();
/*!!!
    for (int i = 0; i < int(pyinPitch.size()); ++i) {
        if (pyinPitch[i] <= 0) continue;
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = pyinTimestamps[i];
        f.values.push_back(pyinPitch[i]);
        fs[m_pitchTrackOutput].push_back(f);
    }
*/
    
    return fs;
}

