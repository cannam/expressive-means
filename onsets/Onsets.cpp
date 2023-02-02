
/*
    Expressive Means Onsets

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Onsets.h"

#include <vector>
#include <set>

using std::cerr;
using std::endl;
using std::vector;
using std::set;

static const CoreFeatures::Parameters defaultCoreParams;

Onsets::Onsets(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_stepSize(0),
    m_blockSize(0),
    m_haveStartTime(false),
    m_coreFeatures(inputSampleRate),
    m_coreParams(defaultCoreParams),
    m_onsetOutput(-1),
    m_durationOutput(-1),
    m_pitchOnsetDfOutput(-1),
    m_transientOnsetDfOutput(-1)
{
}

Onsets::~Onsets()
{
}

string
Onsets::getIdentifier() const
{
    return "onsets";
}

string
Onsets::getName() const
{
    return "Expressive Means: Onsets";
}

string
Onsets::getDescription() const
{
    return "";
}

string
Onsets::getMaker() const
{
    return "Frithjof Vollmer and Chris Cannam";
}

int
Onsets::getPluginVersion() const
{
    return 1;
}

string
Onsets::getCopyright() const
{
    return "GPLv2";
}

Onsets::InputDomain
Onsets::getInputDomain() const
{
    return TimeDomain;
}

size_t
Onsets::getPreferredBlockSize() const
{
    return m_coreFeatures.getPreferredBlockSize();
}

size_t 
Onsets::getPreferredStepSize() const
{
    return m_coreFeatures.getPreferredStepSize();
}

size_t
Onsets::getMinChannelCount() const
{
    return 1;
}

size_t
Onsets::getMaxChannelCount() const
{
    return 1;
}

Onsets::ParameterList
Onsets::getParameterDescriptors() const
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
    
    d.identifier = "onsetSensitivityRawPowerThreshold";
    d.name = "Onset sensitivity: Raw power threshold";
    d.unit = "dB";
    d.minValue = 0.f;
    d.maxValue = 100.f;
    d.defaultValue = defaultCoreParams.onsetSensitivityRawPowerThreshold_dB;
    list.push_back(d);
    
    d.identifier = "minimumOnsetInterval";
    d.name = "Minimum onset interval";
    d.unit = "ms";
    d.minValue = 0.f;
    d.maxValue = 1000.f;
    d.defaultValue = defaultCoreParams.minimumOnsetInterval_ms;
    list.push_back(d);
    
    d.identifier = "sustainBeginThreshold";
    d.name = "Sustain phase begin threshold";
    d.unit = "ms";
    d.minValue = 0.f;
    d.maxValue = 1000.f;
    d.defaultValue = defaultCoreParams.sustainBeginThreshold_ms;
    list.push_back(d);
    
    d.identifier = "noteDurationThreshold";
    d.name = "Note duration level drop threshold";
    d.unit = "dB";
    d.minValue = 0.f;
    d.maxValue = 100.f;
    d.defaultValue = defaultCoreParams.noteDurationThreshold_dB;
    list.push_back(d);
    
    return list;
}

float
Onsets::getParameter(string identifier) const
{
    if (identifier == "pyin-threshdistr") {
        return m_coreParams.pyinThresholdDistribution;
    } else if (identifier == "pyin-lowampsuppression") {
        return m_coreParams.pyinLowAmpSuppressionThreshold;
    } else if (identifier == "pitchAverageWindow") {
        return m_coreParams.pitchAverageWindow_ms;
    } else if (identifier == "onsetSensitivityPitch") {
        return m_coreParams.onsetSensitivityPitch_cents;
    } else if (identifier == "onsetSensitivityNoise") {
        return m_coreParams.onsetSensitivityNoise_percent;
    } else if (identifier == "onsetSensitivityLevel") {
        return m_coreParams.onsetSensitivityLevel_dB;
    } else if (identifier == "onsetSensitivityNoiseTimeWindow") {
        return m_coreParams.onsetSensitivityNoiseTimeWindow_ms;
    } else if (identifier == "onsetSensitivityRawPowerThreshold") {
        return m_coreParams.onsetSensitivityRawPowerThreshold_dB;
    } else if (identifier == "minimumOnsetInterval") {
        return m_coreParams.minimumOnsetInterval_ms;
    } else if (identifier == "sustainBeginThreshold") {
        return m_coreParams.sustainBeginThreshold_ms;
    } else if (identifier == "noteDurationThreshold") {
        return m_coreParams.noteDurationThreshold_dB;
    }
    
    return 0.f;
}

void
Onsets::setParameter(string identifier, float value) 
{
    if (identifier == "pyin-threshdistr") {
        m_coreParams.pyinThresholdDistribution = value;
    } else if (identifier == "pyin-lowampsuppression") {
        m_coreParams.pyinLowAmpSuppressionThreshold = value;
    } else if (identifier == "pitchAverageWindow") {
        m_coreParams.pitchAverageWindow_ms = value;
    } else if (identifier == "onsetSensitivityPitch") {
        m_coreParams.onsetSensitivityPitch_cents = value;
    } else if (identifier == "onsetSensitivityNoise") {
        m_coreParams.onsetSensitivityNoise_percent = value;
    } else if (identifier == "onsetSensitivityLevel") {
        m_coreParams.onsetSensitivityLevel_dB = value;
    } else if (identifier == "onsetSensitivityNoiseTimeWindow") {
        m_coreParams.onsetSensitivityNoiseTimeWindow_ms = value;
    } else if (identifier == "onsetSensitivityRawPowerThreshold") {
        m_coreParams.onsetSensitivityRawPowerThreshold_dB = value;
    } else if (identifier == "minimumOnsetInterval") {
        m_coreParams.minimumOnsetInterval_ms = value;
    } else if (identifier == "sustainBeginThreshold") {
        m_coreParams.sustainBeginThreshold_ms = value;
    } else if (identifier == "noteDurationThreshold") {
        m_coreParams.noteDurationThreshold_dB = value;
    }
}

Onsets::ProgramList
Onsets::getPrograms() const
{
    ProgramList list;
    return list;
}

string
Onsets::getCurrentProgram() const
{
    return ""; 
}

void
Onsets::selectProgram(string)
{
}

Onsets::OutputList
Onsets::getOutputDescriptors() const
{
    OutputList list;
    OutputDescriptor d;

    d.identifier = "onsets";
    d.name = "Onsets";
    d.description = "Identified onset locations, labelled as either Pitch Change or Spectral Rise depending on how they were identified.";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 0;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = (m_inputSampleRate / m_stepSize);
    d.hasDuration = false;
    m_onsetOutput = int(list.size());
    list.push_back(d);

    d.identifier = "durations";
    d.name = "Durations";
    d.description = "Identified note onsets with estimated duration. Features have value 1 for notes identified via pitch change, 2 for spectral rise, and 3 for raw power rise. Offsets are determined using the \"Note duration level drop threshold\" parameter.";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = (m_inputSampleRate / m_stepSize);
    d.hasDuration = true;
    m_durationOutput = int(list.size());
    list.push_back(d);

    d.identifier = "pitchdf";
    d.name = "Pitch Onset Detection Function";
    d.description = "Function used to identify onsets by pitch change. Onsets are considered likely when the function is low rather than high, i.e. when it first falls below a threshold.";
    d.unit = "semitones";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = (m_inputSampleRate / m_stepSize);
    d.hasDuration = false;
    m_pitchOnsetDfOutput = int(list.size());
    list.push_back(d);

    d.identifier = "transientdf";
    d.name = "Transient Onset Detection Function";
    d.description = "Function used to identify onsets by spectral rise. Onsets are considered likely when the function exceeds a threshold.";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = (m_inputSampleRate / m_stepSize);
    d.hasDuration = false;
    m_transientOnsetDfOutput = int(list.size());
    list.push_back(d);
    
    return list;
}

bool
Onsets::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    if (channels < getMinChannelCount() || channels > getMaxChannelCount()) {
        cerr << "ERROR: Onsets::initialise: unsupported channel count "
             << channels << endl;
        return false;
    }

    if (m_inputSampleRate < 8000.0) {
        cerr << "ERROR: Onsets::initialise: sample rate ("
             << m_inputSampleRate << ") is too low, it must be at least 8kHz"
             << endl;
        return false;
    }
    
    if (m_inputSampleRate > 192000.0) {
        cerr << "ERROR: Onsets::initialise: sample rate ("
             << m_inputSampleRate << ") is too high, maximum is 192kHz"
             << endl;
        return false;
    }
    
    if (stepSize > blockSize) {
        cerr << "ERROR: Onsets::initialise: step size (" << stepSize
             << ") may not exceed block size (" << blockSize << ")" << endl;
        return false;
    }

    if (m_onsetOutput < 0) {
        (void)getOutputDescriptors(); // initialise output indices
    }
    
    m_stepSize = stepSize;
    m_blockSize = blockSize;
    m_haveStartTime = false;

    try {
        m_coreParams.stepSize = m_stepSize;
        m_coreParams.blockSize = m_blockSize;
        m_coreFeatures.initialise(m_coreParams);
    } catch (const std::logic_error &e) {
        cerr << "ERROR: Onsets::initialise: Feature extractor initialisation failed: " << e.what() << endl;
        return false;
    }
    
    return true;
}

void
Onsets::reset()
{
    m_haveStartTime = false;
    m_coreFeatures.reset();
}

Onsets::FeatureSet
Onsets::process(const float *const *inputBuffers, Vamp::RealTime timestamp)
{
    if (!m_haveStartTime) {
        m_startTime = timestamp;
        m_haveStartTime = true;
    }

    m_coreFeatures.process(inputBuffers[0], timestamp);
    return {};
}

Onsets::FeatureSet
Onsets::getRemainingFeatures()
{
    FeatureSet fs;

    m_coreFeatures.finish();

    auto timeForStep = [&](int i) {
        return m_startTime + Vamp::RealTime::frame2RealTime
            (i * m_stepSize, m_inputSampleRate);
    };
    
    auto pitchOnsetDf = m_coreFeatures.getPitchOnsetDF();
    for (int i = 0; i < int(pitchOnsetDf.size()); ++i) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = timeForStep(i);
        f.values.push_back(pitchOnsetDf[i]);
        fs[m_pitchOnsetDfOutput].push_back(f);
    }
    
    auto riseFractions = m_coreFeatures.getOnsetLevelRiseFractions();
    for (size_t i = 0; i < riseFractions.size(); ++i) {
        Feature f;
        f.hasTimestamp = true;
        int j = i + (m_blockSize / m_stepSize)/2;
        f.timestamp = timeForStep(j);
        f.values.push_back(riseFractions[i]);
        fs[m_transientOnsetDfOutput].push_back(f);
    }

    auto onsets = m_coreFeatures.getMergedOnsets();
    auto onsetOffsets = m_coreFeatures.getOnsetOffsets();

    for (auto pq : onsets) {
        int onset = pq.first;
        auto type = pq.second;
        int offset = onsetOffsets.at(onset);

        Feature f;
        f.hasTimestamp = true;
        f.timestamp = timeForStep(onset);
        f.hasDuration = false;
        switch (type) {
        case CoreFeatures::OnsetType::Pitch:
            f.label = "Pitch Change";
            break;
        case CoreFeatures::OnsetType::SpectralLevelRise:
            f.label = "Spectral Rise";
            break;
        case CoreFeatures::OnsetType::PowerRise:
            f.label = "Power Rise";
            break;
        }
        fs[m_onsetOutput].push_back(f);

        f.hasDuration = true;
        f.duration = timeForStep(offset) - f.timestamp;
        f.label = "";
            
        switch (type) {
        case CoreFeatures::OnsetType::Pitch:
            f.values.push_back(1);
            break;
        case CoreFeatures::OnsetType::SpectralLevelRise:
            f.values.push_back(2);
            break;
        case CoreFeatures::OnsetType::PowerRise:
            f.values.push_back(3);
            break;
        }
        
        fs[m_durationOutput].push_back(f);
    }
    
    return fs;
}

