
/*
    Expressive Means Articulation

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Articulation.h"

#include "../ext/qm-dsp/maths/MedianFilter.h"

#include <vector>
#include <set>

using std::cerr;
using std::endl;
using std::vector;
using std::set;

static const float default_pitchAverageWindow_ms = 250.f;
static const float default_onsetSensitivityPitch_cents = 20.f;
static const float default_onsetSensitivityNoise_percent = 10.f;
static const float default_onsetSensitivityLevel_dB = 20.f;
static const float default_onsetSensitivityNoiseTimeWindow_ms = 50.f;
static const float default_minimumOnsetInterval_ms = 200.f;
static const float default_sustainBeginThreshold_ms = 50.f;
static const float default_sustainEndThreshold_dBFS = -45.f;
static const float default_volumeDevelopmentThreshold_dB = 2.f;
static const float default_scalingFactor = 10.7f;

Articulation::Articulation(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_pyin(inputSampleRate),
    m_haveStartTime(false),
    m_pitchAverageWindow_ms(default_pitchAverageWindow_ms),
    m_onsetSensitivityPitch_cents(default_onsetSensitivityPitch_cents),
    m_onsetSensitivityNoise_percent(default_onsetSensitivityNoise_percent),
    m_onsetSensitivityLevel_dB(default_onsetSensitivityLevel_dB),
    m_onsetSensitivityNoiseTimeWindow_ms(default_onsetSensitivityNoiseTimeWindow_ms),
    m_minimumOnsetInterval_ms(default_minimumOnsetInterval_ms),
    m_sustainBeginThreshold_ms(default_sustainBeginThreshold_ms),
    m_sustainEndThreshold_dBFS(default_sustainEndThreshold_dBFS),
    m_volumeDevelopmentThreshold_dB(default_volumeDevelopmentThreshold_dB),
    m_scalingFactor(default_scalingFactor),
    m_blockSize(m_pyin.getPreferredBlockSize()),
    m_stepSize(m_pyin.getPreferredStepSize())
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
    return "Articulation";
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
    return m_pyin.getPreferredBlockSize();
}

size_t 
Articulation::getPreferredStepSize() const
{
    return m_pyin.getPreferredStepSize();
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

    ParameterList pyinParams = m_pyin.getParameterDescriptors();
    for (auto d: pyinParams) {
        // I think these ones are not relevant to us? Check this
        if (d.identifier == "fixedlag" ||
            d.identifier == "outputunvoiced" ||
            d.identifier == "onsetsensitivity" ||
            d.identifier == "prunethresh") {
            continue;
        }
        d.identifier = "pyin-" + d.identifier;
        d.name = "pYIN: " + d.name;
        list.push_back(d);
    }

    ParameterDescriptor d;

    d.description = "";
    d.isQuantized = false;

    d.identifier = "pitchAverageWindow";
    d.name = "Moving pitch average window";
    d.unit = "ms";
    d.minValue = 20.f;
    d.maxValue = 1000.f;
    d.defaultValue = default_pitchAverageWindow_ms;
    list.push_back(d);

    d.identifier = "onsetSensitivityPitch";
    d.name = "Onset sensitivity: Pitch";
    d.unit = "cents";
    d.minValue = 0.f;
    d.maxValue = 100.f;
    d.defaultValue = default_onsetSensitivityPitch_cents;
    list.push_back(d);
    
    d.identifier = "onsetSensitivityNoise";
    d.name = "Onset sensitivity: Noise";
    d.unit = "%";
    d.minValue = 0.f;
    d.maxValue = 100.f;
    d.defaultValue = default_onsetSensitivityNoise_percent;
    list.push_back(d);
    
    d.identifier = "onsetSensitivityLevel";
    d.name = "Onset sensitivity: Level";
    d.unit = "dB";
    d.minValue = 0.f;
    d.maxValue = 100.f;
    d.defaultValue = default_onsetSensitivityLevel_dB;
    list.push_back(d);
    
    d.identifier = "onsetSensitivityNoiseTimeWindow";
    d.name = "Onset sensitivity: Noise time window";
    d.unit = "ms";
    d.minValue = 20.f;
    d.maxValue = 500.f;
    d.defaultValue = default_onsetSensitivityNoiseTimeWindow_ms;
    list.push_back(d);
    
    d.identifier = "minimumOnsetInterval";
    d.name = "Minimum onset interval";
    d.unit = "ms";
    d.minValue = 0.f;
    d.maxValue = 1000.f;
    d.defaultValue = default_minimumOnsetInterval_ms;
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
    if (identifier.size() > 5 &&
        string(identifier.begin(), identifier.begin() + 5) == "pyin-") {
        string pyinParam(identifier.begin() + 5, identifier.end());
//        cerr << "<- " << pyinParam << endl;
        return m_pyin.getParameter(pyinParam);
    }
    
    if (identifier == "pitchAverageWindow") {
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
    if (identifier.size() > 5 &&
        string(identifier.begin(), identifier.begin() + 5) == "pyin-") {
        string pyinParam(identifier.begin() + 5, identifier.end());
//        cerr << pyinParam << " -> " << value << endl;
        m_pyin.setParameter(pyinParam, value);
        return;
    }

    if (identifier == "pitchAverageWindow") {
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
    OutputList pyinOutputs = m_pyin.getOutputDescriptors();
    m_pyinSmoothedPitchTrackOutput = -1;
    for (int i = 0; i < int(pyinOutputs.size()); ++i) {
        if (pyinOutputs[i].identifier == "smoothedpitchtrack") {
            m_pyinSmoothedPitchTrackOutput = i;
        }
    }
    if (m_pyinSmoothedPitchTrackOutput == -1) {
        cerr << "ERROR: Articulation::getOutputDescriptors: pYIN smoothed pitch track output not found" << endl;
        m_pyinSmoothedPitchTrackOutput = 0;
    }
    
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
    d.description = "";
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
    d.description = "";
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
    d.description = "";
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

    m_pyin.setParameter("outputunvoiced", 2); // As negative frequencies
    
    if (!m_pyin.initialise(channels, stepSize, blockSize)) {
        cerr << "ERROR: Articulation::initialise: pYIN initialise failed" << endl;
        return false;
    }
    
    m_stepSize = stepSize;
    m_blockSize = blockSize;

    reset();
    
    return true;
}

void
Articulation::reset()
{
    m_pyin.reset();

    m_power = Power();
    m_onsetLevelRise = SpectralLevelRise();
    m_noiseRatioLevelRise = SpectralLevelRise();
    m_haveStartTime = false;
    m_pitch.clear();

    m_power.initialise(m_blockSize, 18, -120.0);

    int onsetLevelRiseHistoryLength =
        msToSteps(m_onsetSensitivityNoiseTimeWindow_ms, false);
    if (onsetLevelRiseHistoryLength < 2) {
        onsetLevelRiseHistoryLength = 2;
    }
    
    m_onsetLevelRise.initialise
        (m_inputSampleRate, m_blockSize, 100.0, 4000.0,
         m_onsetSensitivityLevel_dB, onsetLevelRiseHistoryLength);

    m_noiseRatioLevelRise.initialise
        (m_inputSampleRate, m_blockSize, 100.0, 4000.0,
         20.0, ceil(0.05 * m_inputSampleRate / m_stepSize));
}

Articulation::FeatureSet
Articulation::process(const float *const *inputBuffers, Vamp::RealTime timestamp)
{
    if (!m_haveStartTime) {
        m_startTime = timestamp;
        m_haveStartTime = true;
    }
    
    FeatureSet fs;
    FeatureSet pyinFeatures = m_pyin.process(inputBuffers, timestamp);
    for (const auto &f: pyinFeatures[m_pyinSmoothedPitchTrackOutput]) {
        double hz = f.values[0];
        if (hz > 0.0) {
            fs[m_pitchTrackOutput].push_back(f);
            m_pitch.push_back(hzToPitch(hz));
        } else {
            m_pitch.push_back(0.0);
        }
    }
    
    m_power.process(inputBuffers[0]);
    m_onsetLevelRise.process(inputBuffers[0]);
    m_noiseRatioLevelRise.process(inputBuffers[0]);

    return fs;
}

Articulation::FeatureSet
Articulation::getRemainingFeatures()
{
    FeatureSet fs;

    FeatureSet pyinFeatures = m_pyin.getRemainingFeatures();
    for (const auto &f : pyinFeatures[m_pyinSmoothedPitchTrackOutput]) {
        double hz = f.values[0];
        if (hz > 0.0) {
            fs[m_pitchTrackOutput].push_back(f);
            m_pitch.push_back(hzToPitch(hz));
        } else {
            m_pitch.push_back(0.0);
        }
    }

    // "If the absolute difference of a pitch and its following moving
    // pitch average window falls below o_2" - calculate a moving mean
    // window over the pitch curve (which is in semitones, not Hz) and
    // compare each pitch to the mean within the window that follows
    // it: if they are close, record an onset
    
    int pitchFilterLength = msToSteps(m_pitchAverageWindow_ms, true);
    int halfLength = pitchFilterLength/2;
    MeanFilter pitchFilter(pitchFilterLength);
    int n = m_pitch.size();
    vector<double> filteredPitch(n, 0.0);
    pitchFilter.filter(m_pitch.data(), filteredPitch.data(), n);
    
    vector<double> pitchOnsetDf;
    for (int i = 0; i + halfLength < n; ++i) {
        pitchOnsetDf.push_back
            (fabsf(m_pitch[i] - filteredPitch[i + halfLength]));
    }

    set<int> pitchOnsets;
    int minimumOnsetSteps = msToSteps(m_minimumOnsetInterval_ms, false);
    int lastBelowThreshold = -minimumOnsetSteps;
    double threshold = m_onsetSensitivityPitch_cents / 100.0;

    for (int i = 0; i + halfLength < n; ++i) {
        // "absolute difference... falls below o_2":
        if (pitchOnsetDf[i] < threshold) {
            // "subsequent onsets require o_2 to be exceeded for at
            // least the duration of o_6 first":
            if (i > lastBelowThreshold + minimumOnsetSteps) {
                pitchOnsets.insert(i);
            }
            lastBelowThreshold = i;
        }
    }
    
    vector<double> smoothedPower = m_power.getSmoothedPower();
    vector<double> riseFractions = m_onsetLevelRise.getFractions();

    set<int> allOnsets = pitchOnsets;
    for (int i = 0; i < int(riseFractions.size()); ++i) {
        if (riseFractions[i] > m_onsetSensitivityNoise_percent / 100.0) {
            allOnsets.insert(i + (m_blockSize / m_stepSize)/2);
        }
    }

    vector<int> onsets;
    int prevP = -minimumOnsetSteps;
    for (auto p: allOnsets) {
        if (p < prevP + minimumOnsetSteps) {
            continue;
        }
        onsets.push_back(p);
        prevP = p;
    }
    
#ifdef WITH_DEBUG_OUTPUTS
    for (int i = 0; i < n; ++i) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_startTime + Vamp::RealTime::frame2RealTime
            (i * m_stepSize, m_inputSampleRate);
        f.values.push_back(filteredPitch[i]);
        fs[m_filteredPitchOutput].push_back(f);
    }
    
    for (int i = 0; i < int(pitchOnsetDf.size()); ++i) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_startTime + Vamp::RealTime::frame2RealTime
            (i * m_stepSize, m_inputSampleRate);
        f.values.push_back(pitchOnsetDf[i]);
        fs[m_pitchOnsetDfOutput].push_back(f);
    }
    
    for (size_t i = 0; i < smoothedPower.size(); ++i) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_startTime + Vamp::RealTime::frame2RealTime
            (i * m_stepSize, m_inputSampleRate);
        f.values.push_back(smoothedPower[i]);
        fs[m_powerOutput].push_back(f);
    }
    
    for (size_t i = 0; i < riseFractions.size(); ++i) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_startTime + Vamp::RealTime::frame2RealTime
            (i * m_stepSize, m_inputSampleRate);
        f.values.push_back(riseFractions[i]);
        fs[m_transientOnsetDfOutput].push_back(f);
    }

    for (auto p: onsets) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_startTime + Vamp::RealTime::frame2RealTime
            (p * m_stepSize, m_inputSampleRate);
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

