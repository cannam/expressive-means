
/*
    Expressive Means Pitch Vibrato

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "PitchVibrato.h"

#include <vector>
#include <set>

using std::cerr;
using std::endl;
using std::vector;
using std::set;

static const CoreFeatures::Parameters defaultCoreParams;

PitchVibrato::PitchVibrato(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_stepSize(0),
    m_blockSize(0),
    m_haveStartTime(false),
    m_coreFeatures(inputSampleRate),
    m_coreParams(defaultCoreParams)
    /*,
    m_summaryOutput(-1),
    m_pitchvibratoTypeOutput(-1),
    m_pitchTrackOutput(-1),
    m_pitchvibratoIndexOutput(-1)
    */
{
}

PitchVibrato::~PitchVibrato()
{
}

string
PitchVibrato::getIdentifier() const
{
    return "pitch-vibrato";
}

string
PitchVibrato::getName() const
{
    return "Expressive Means: Pitch Vibrato";
}

string
PitchVibrato::getDescription() const
{
    return "";
}

string
PitchVibrato::getMaker() const
{
    return "Frithjof Vollmer and Chris Cannam";
}

int
PitchVibrato::getPluginVersion() const
{
    return 1;
}

string
PitchVibrato::getCopyright() const
{
    return "GPLv2";
}

PitchVibrato::InputDomain
PitchVibrato::getInputDomain() const
{
    return TimeDomain;
}

size_t
PitchVibrato::getPreferredBlockSize() const
{
    return m_coreFeatures.getPreferredBlockSize();
}

size_t 
PitchVibrato::getPreferredStepSize() const
{
    return m_coreFeatures.getPreferredStepSize();
}

size_t
PitchVibrato::getMinChannelCount() const
{
    return 1;
}

size_t
PitchVibrato::getMaxChannelCount() const
{
    return 1;
}

PitchVibrato::ParameterList
PitchVibrato::getParameterDescriptors() const
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
PitchVibrato::getParameter(string identifier) const
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
    } else if (identifier == "minimumOnsetInterval") {
        return m_coreParams.minimumOnsetInterval_ms;
    }
    
    return 0.f;
}

void
PitchVibrato::setParameter(string identifier, float value) 
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
    } else if (identifier == "minimumOnsetInterval") {
        m_coreParams.minimumOnsetInterval_ms = value;
    }
}

PitchVibrato::ProgramList
PitchVibrato::getPrograms() const
{
    ProgramList list;
    return list;
}

string
PitchVibrato::getCurrentProgram() const
{
    return ""; 
}

void
PitchVibrato::selectProgram(string)
{
}

PitchVibrato::OutputList
PitchVibrato::getOutputDescriptors() const
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
    
    d.identifier = "pitchvibratoType";
    d.name = "Pitch Vibrato Type";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 0;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.hasDuration = false;
    m_pitchvibratoTypeOutput = int(list.size());
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
    
    d.identifier = "pitchvibratoIndex";
    d.name = "Pitch Vibrato Index";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.hasDuration = false;
    m_pitchvibratoIndexOutput = int(list.size());
    list.push_back(d);
*/
    
    return list;
}

bool
PitchVibrato::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    if (channels < getMinChannelCount() || channels > getMaxChannelCount()) {
        cerr << "ERROR: PitchVibrato::initialise: unsupported channel count "
             << channels << endl;
        return false;
    }

    if (m_inputSampleRate < 8000.0) {
        cerr << "ERROR: PitchVibrato::initialise: sample rate ("
             << m_inputSampleRate << ") is too low, it must be at least 8kHz"
             << endl;
        return false;
    }
    
    if (m_inputSampleRate > 192000.0) {
        cerr << "ERROR: PitchVibrato::initialise: sample rate ("
             << m_inputSampleRate << ") is too high, maximum is 192kHz"
             << endl;
        return false;
    }
    
    if (stepSize > blockSize) {
        cerr << "ERROR: PitchVibrato::initialise: step size (" << stepSize
             << ") may not exceed block size (" << blockSize << ")" << endl;
        return false;
    }
/*!!!
    if (m_summaryOutput < 0) {
        (void)getOutputDescriptors(); // initialise output indices
    }
*/    
    m_stepSize = stepSize;
    m_blockSize = blockSize;
    m_haveStartTime = false;

    try {
        m_coreParams.stepSize = m_stepSize;
        m_coreParams.blockSize = m_blockSize;
        m_coreFeatures.initialise(m_coreParams);
    } catch (const std::logic_error &e) {
        cerr << "ERROR: PitchVibrato::initialise: Feature extractor initialisation failed: " << e.what() << endl;
        return false;
    }
    
    return true;
}

void
PitchVibrato::reset()
{
    m_haveStartTime = false;
    m_coreFeatures.reset();
}

PitchVibrato::FeatureSet
PitchVibrato::process(const float *const *inputBuffers, Vamp::RealTime timestamp)
{
    if (!m_haveStartTime) {
        m_startTime = timestamp;
        m_haveStartTime = true;
    }

    m_coreFeatures.process(inputBuffers[0], timestamp);
    return {};
}

PitchVibrato::FeatureSet
PitchVibrato::getRemainingFeatures()
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

