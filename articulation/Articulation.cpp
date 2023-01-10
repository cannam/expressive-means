
/*
    Expressive Means Articulation

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Articulation.h"

using std::cerr;
using std::endl;

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
    // Return something helpful here!
    return "";
}

string
Articulation::getMaker() const
{
    // Your name here
    return "";
}

int
Articulation::getPluginVersion() const
{
    // Increment this each time you release a version that behaves
    // differently from the previous one
    return 1;
}

string
Articulation::getCopyright() const
{
    // This function is not ideally named.  It does not necessarily
    // need to say who made the plugin -- getMaker does that -- but it
    // should indicate the terms under which it is distributed.  For
    // example, "Copyright (year). All Rights Reserved", or "GPL"
    return "";
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
        cerr << "<- " << pyinParam << endl;
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
        cerr << pyinParam << " -> " << value << endl;
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
Articulation::selectProgram(string name)
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
    
    d.identifier = "power";
    d.name = "Power";
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
    
    d.identifier = "transient";
    d.name = "Transient Detection Function";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = (m_inputSampleRate / m_stepSize);
    d.hasDuration = false;
    m_transientDfOutput = int(list.size());
    list.push_back(d);

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

    if (stepSize > blockSize) {
        cerr << "ERROR: Articulation::initialise: step size may not exceed block size" << endl;
        return false;
    }
    
    if (!m_pyin.initialise(channels, stepSize, blockSize)) {
        cerr << "ERROR: Articulation::initialise: pYIN initialise failed" << endl;
        return false;
    }

    m_power.initialise(blockSize, 18, -120.0);

    //!!! actually want two of these, one parameterised
    m_levelRise.initialise(m_inputSampleRate, blockSize, 100.0, 4000.0, 20.0,
                           ceil(0.05 * m_inputSampleRate / m_stepSize));
    
    m_stepSize = stepSize;
    m_blockSize = blockSize;
    
    return true;
}

void
Articulation::reset()
{
    m_pyin.reset();
    m_power = Power();
    m_levelRise = SpectralLevelRise();
    m_haveStartTime = false;
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
    fs[m_pitchTrackOutput] = pyinFeatures[m_pyinSmoothedPitchTrackOutput];
    
    m_power.process(inputBuffers[0]);
    m_levelRise.process(inputBuffers[0]);

    return fs;
}

Articulation::FeatureSet
Articulation::getRemainingFeatures()
{
    FeatureSet fs;

    FeatureSet pyinFeatures = m_pyin.getRemainingFeatures();
    fs[m_pitchTrackOutput] = pyinFeatures[m_pyinSmoothedPitchTrackOutput];

    auto smoothedPower = m_power.getSmoothedPower();
    for (size_t i = 0; i < smoothedPower.size(); ++i) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_startTime + Vamp::RealTime::frame2RealTime
            (i * m_stepSize, m_inputSampleRate);
        f.values.push_back(smoothedPower[i]);
        fs[m_powerOutput].push_back(f);
    }
    
    auto fractions = m_levelRise.getFractions();
    for (size_t i = 0; i < fractions.size(); ++i) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_startTime + Vamp::RealTime::frame2RealTime
            (i * m_stepSize, m_inputSampleRate);
        f.values.push_back(fractions[i]);
        fs[m_transientDfOutput].push_back(f);
    }
    
    return fs;
}

