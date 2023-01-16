
/*
    Expressive Means Articulation

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Articulation.h"

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

Articulation::Articulation(float inputSampleRate) :
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
    m_articulationTypeOutput(-1),
    m_pitchTrackOutput(-1),
    m_articulationIndexOutput(-1)
{
}

Articulation::~Articulation()
{
}

string
Articulation::getIdentifier() const
{
    return "articulation";
}

string
Articulation::getName() const
{
    return "Expressive Means: Articulation";
}

string
Articulation::getDescription() const
{
    return "";
}

string
Articulation::getMaker() const
{
    return "Frithjof Vollmer and Chris Cannam";
}

int
Articulation::getPluginVersion() const
{
    return 1;
}

string
Articulation::getCopyright() const
{
    return "GPLv2";
}

Articulation::InputDomain
Articulation::getInputDomain() const
{
    return TimeDomain;
}

size_t
Articulation::getPreferredBlockSize() const
{
    return m_coreFeatures.getPreferredBlockSize();
}

size_t 
Articulation::getPreferredStepSize() const
{
    return m_coreFeatures.getPreferredStepSize();
}

size_t
Articulation::getMinChannelCount() const
{
    return 1;
}

size_t
Articulation::getMaxChannelCount() const
{
    return 1;
}

Articulation::ParameterList
Articulation::getParameterDescriptors() const
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
    
    d.identifier = "sustainBeginThreshold";
    d.name = "Sustain phase begin threshold";
    d.unit = "ms";
    d.minValue = 0.f;
    d.maxValue = 1000.f;
    d.defaultValue = default_sustainBeginThreshold_ms;
    list.push_back(d);
    
    d.identifier = "sustainEndThreshold";
    d.name = "Sustain phase end threshold";
    d.unit = "dBFS";
    d.minValue = -80.f;
    d.maxValue = 0.f;
    d.defaultValue = default_sustainEndThreshold_dBFS;
    list.push_back(d);
    
    d.identifier = "volumeDevelopmentThreshold";
    d.name = "Volume development threshold";
    d.unit = "dB";
    d.minValue = 0.f;
    d.maxValue = 10.f;
    d.defaultValue = default_volumeDevelopmentThreshold_dB;
    list.push_back(d);
    
    d.identifier = "scalingFactor";
    d.name = "Scaling factor";
    d.unit = "";
    d.minValue = 1.f;
    d.maxValue = 30.f;
    d.defaultValue = default_scalingFactor;
    list.push_back(d);
    
    return list;
}

float
Articulation::getParameter(string identifier) const
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
    } else if (identifier == "sustainBeginThreshold") {
        return m_sustainBeginThreshold_ms;
    } else if (identifier == "sustainEndThreshold") {
        return m_sustainEndThreshold_dBFS;
    } else if (identifier == "volumeDevelopmentThreshold") {
        return m_volumeDevelopmentThreshold_dB;
    } else if (identifier == "scalingFactor") {
        return m_scalingFactor;
    }
    
    return 0.f;
}

void
Articulation::setParameter(string identifier, float value) 
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
    } else if (identifier == "sustainBeginThreshold") {
        m_sustainBeginThreshold_ms = value;
    } else if (identifier == "sustainEndThreshold") {
        m_sustainEndThreshold_dBFS = value;
    } else if (identifier == "volumeDevelopmentThreshold") {
        m_volumeDevelopmentThreshold_dB = value;
    } else if (identifier == "scalingFactor") {
        m_scalingFactor = value;
    }
}

Articulation::ProgramList
Articulation::getPrograms() const
{
    ProgramList list;
    return list;
}

string
Articulation::getCurrentProgram() const
{
    return ""; 
}

void
Articulation::selectProgram(string)
{
}

Articulation::OutputList
Articulation::getOutputDescriptors() const
{
    OutputList list;
    OutputDescriptor d;
    
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
    
    d.identifier = "articulationType";
    d.name = "Articulation Type";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 0;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.hasDuration = false;
    m_articulationTypeOutput = int(list.size());
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
    
    d.identifier = "articulationIndex";
    d.name = "Articulation Index";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.hasDuration = false;
    m_articulationIndexOutput = int(list.size());
    list.push_back(d);

#ifdef WITH_DEBUG_OUTPUTS
    d.identifier = "power";
    d.name = "[Debug] Power";
    d.description = "Smoothed power curve.";
    d.unit = "dB";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = (m_inputSampleRate / m_stepSize);
    d.hasDuration = false;
    m_powerOutput = int(list.size());
    list.push_back(d);
    
    d.identifier = "filteredPitch";
    d.name = "[Debug] Filtered Pitch";
    d.description = "Re-filtered pitch track.";
    d.unit = "MIDI units";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = (m_inputSampleRate / m_stepSize);
    d.hasDuration = false;
    m_filteredPitchOutput = int(list.size());
    list.push_back(d);

    d.identifier = "pitchdf";
    d.name = "[Debug] Pitch Onset Detection Function";
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
    d.name = "[Debug] Transient Onset Detection Function";
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

    d.identifier = "onsets";
    d.name = "[Debug] Onsets Labelled by Cause";
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
#endif
    
    return list;
}

bool
Articulation::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    if (channels < getMinChannelCount() || channels > getMaxChannelCount()) {
        cerr << "ERROR: Articulation::initialise: unsupported channel count "
             << channels << endl;
        return false;
    }

    if (m_inputSampleRate < 8000.0) {
        cerr << "ERROR: Articulation::initialise: sample rate ("
             << m_inputSampleRate << ") is too low, it must be at least 8kHz"
             << endl;
        return false;
    }
    
    if (m_inputSampleRate > 192000.0) {
        cerr << "ERROR: Articulation::initialise: sample rate ("
             << m_inputSampleRate << ") is too high, maximum is 192kHz"
             << endl;
        return false;
    }
    
    if (stepSize > blockSize) {
        cerr << "ERROR: Articulation::initialise: step size (" << stepSize
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
        cerr << "ERROR: Articulation::initialise: Feature extractor initialisation failed: " << e.what() << endl;
        return false;
    }
    
    return true;
}

void
Articulation::reset()
{
    m_haveStartTime = false;
    m_coreFeatures.reset();
}

Articulation::FeatureSet
Articulation::process(const float *const *inputBuffers, Vamp::RealTime timestamp)
{
    if (!m_haveStartTime) {
        m_startTime = timestamp;
        m_haveStartTime = true;
    }

    m_coreFeatures.process(inputBuffers[0], timestamp);
    return {};
}

Articulation::FeatureSet
Articulation::getRemainingFeatures()
{
    FeatureSet fs;

    m_coreFeatures.finish();

    auto pyinPitch = m_coreFeatures.getPYinPitch_Hz();
    auto pyinTimestamps = m_coreFeatures.getPYinTimestamps();

    for (int i = 0; i < int(pyinPitch.size()); ++i) {
        if (pyinPitch[i] <= 0) continue;
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = pyinTimestamps[i];
        f.values.push_back(pyinPitch[i]);
        fs[m_pitchTrackOutput].push_back(f);
    }
    
#ifdef WITH_DEBUG_OUTPUTS
    auto timeForStep = [&](int i) {
        return m_startTime + Vamp::RealTime::frame2RealTime
            (i * m_stepSize, m_inputSampleRate);
    };
    
    auto filteredPitch = m_coreFeatures.getFilteredPitch_semis();
    for (int i = 0; i < int(filteredPitch.size()); ++i) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = timeForStep(i);
        f.values.push_back(filteredPitch[i]);
        fs[m_filteredPitchOutput].push_back(f);
    }
    
    auto pitchOnsetDf = m_coreFeatures.getPitchOnsetDF();
    for (int i = 0; i < int(pitchOnsetDf.size()); ++i) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = timeForStep(i);
        f.values.push_back(pitchOnsetDf[i]);
        fs[m_pitchOnsetDfOutput].push_back(f);
    }
    
    auto smoothedPower = m_coreFeatures.getSmoothedPower_dB();
    for (size_t i = 0; i < smoothedPower.size(); ++i) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = timeForStep(i);
        f.values.push_back(smoothedPower[i]);
        fs[m_powerOutput].push_back(f);
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
    auto pitchOnsets = m_coreFeatures.getPitchOnsets();
    for (auto p: onsets) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = timeForStep(p);
        if (pitchOnsets.find(p) != pitchOnsets.end()) {
            f.label = "Pitch Change";
        } else {
            f.label = "Spectral Rise";
        }
        fs[m_onsetOutput].push_back(f);
    }
#endif
    
    return fs;
}

