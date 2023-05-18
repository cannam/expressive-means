
/*
    Expressive Means Onsets

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Onsets.h"

#include "version.h"

#include <vector>
#include <set>

using std::cerr;
using std::endl;
using std::vector;
using std::set;

Onsets::Onsets(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_stepSize(0),
    m_blockSize(0),
    m_coreFeatures(inputSampleRate),
    m_onsetOutput(-1),
    m_offsetOutput(-1),
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
    return "Expressive Means (advanced)";
}

string
Onsets::getDescription() const
{
    return "finds note onsets and durations in monophonic recordings based on changes in spectral content, power, and pitch (specified parameter settings)";
}

string
Onsets::getMaker() const
{
    return "Frithjof Vollmer and Chris Cannam";
}

int
Onsets::getPluginVersion() const
{
    return EXPRESSIVE_MEANS_PLUGIN_VERSION;
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
    m_coreParams.appendVampParameterDescriptors(list, true);
    return list;
}

float
Onsets::getParameter(string identifier) const
{
    float value = 0.f;
    if (m_coreParams.obtainVampParameter(identifier, value)) {
        return value;
    }
    return 0.f;
}

void
Onsets::setParameter(string identifier, float value) 
{
    (void)m_coreParams.acceptVampParameter(identifier, value);
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

    // In common
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = (m_inputSampleRate / m_stepSize);

    d.identifier = "onsets";
    d.name = "Onsets";
    d.description = "Identified onset locations, labelled as either Pitch Change, Spectral Rise, or Power Rise depending on how they were identified.";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 0;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_onsetOutput = int(list.size());
    list.push_back(d);

    d.identifier = "offsets";
    d.name = "Offsets";
    d.description = "Estimated offset locations, labelled as either Power Drop, Spectral Drop, or Following Onset Reached depending on how they were identified.";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 0;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_offsetOutput = int(list.size());
    list.push_back(d);

    d.identifier = "durations";
    d.name = "Durations";
    d.description = "Identified note onsets with estimated duration. Features have value 1 for notes identified via pitch change, 2 for spectral rise, and 3 for raw power rise. Offsets are determined using the \"Note duration level drop threshold\" parameter.";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.hasDuration = true;
    m_durationOutput = int(list.size());
    list.push_back(d);

    d.identifier = "pitchdf";
    d.name = "Pitch Onset Detection Function";
    d.description = "Function used to identify onsets by pitch change. Onsets are considered likely when the function is low rather than high, i.e. when it first falls below a threshold.";
    d.unit = "cents";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_pitchOnsetDfOutput = int(list.size());
    list.push_back(d);

    d.identifier = "transientdf";
    d.name = "Spectral Rise Onset Detection Function";
    d.description = "Function used to identify onsets by spectral rise. Onsets are considered likely when the function exceeds a threshold.";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_transientOnsetDfOutput = int(list.size());
    list.push_back(d);
    
    d.identifier = "power";
    d.name = "Power";
    d.description = "Power curve used to identify onsets and offsets by power level.";
    d.unit = "dB";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_rawPowerOutput = int(list.size());
    list.push_back(d);
    
    d.identifier = "spectraloffset";
    d.name = "Spectral Drop Offset Detection Function";
    d.description = "Function used to identify offsets by spectral drop.";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_spectralDropDfOutput = int(list.size());
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
    m_coreFeatures.reset();
}

Onsets::FeatureSet
Onsets::process(const float *const *inputBuffers, Vamp::RealTime timestamp)
{
    m_coreFeatures.process(inputBuffers[0], timestamp);
    return {};
}

Onsets::FeatureSet
Onsets::getRemainingFeatures()
{
    FeatureSet fs;

    m_coreFeatures.finish();

    auto pitchOnsetDf = m_coreFeatures.getPitchOnsetDF();
    auto pitchOnsetDfValidity = m_coreFeatures.getPitchOnsetDFValidity();
    for (int i = 0; i < int(pitchOnsetDf.size()); ++i) {
        if (pitchOnsetDfValidity[i]) {
            Feature f;
            f.hasTimestamp = true;
            f.timestamp = m_coreFeatures.timeForStep(i);
            f.values.push_back(pitchOnsetDf[i] * 100.0);
            fs[m_pitchOnsetDfOutput].push_back(f);
        }
    }
    
    auto riseFractions = m_coreFeatures.getOnsetLevelRiseFractions();
    for (size_t i = 0; i < riseFractions.size(); ++i) {
        Feature f;
        f.hasTimestamp = true;
        int j = i + (m_blockSize / m_stepSize)/2;
        f.timestamp = m_coreFeatures.timeForStep(j);
        f.values.push_back(riseFractions[i]);
        fs[m_transientOnsetDfOutput].push_back(f);
    }

    auto onsets = m_coreFeatures.getMergedOnsets();
    auto onsetOffsets = m_coreFeatures.getOnsetOffsets();

    for (auto pq : onsets) {
        
        int onset = pq.first;
        auto onsetType = pq.second;
        
        int offset = onsetOffsets.at(onset).first;

        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.timeForStep(onset);
        f.hasDuration = false;
        switch (onsetType) {
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
        f.duration = m_coreFeatures.timeForStep(offset) - f.timestamp;
        f.label = "";
            
        switch (onsetType) {
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

    for (auto pq : onsets) {
        
        int onset = pq.first;
        
        int offset = onsetOffsets.at(onset).first;
        auto offsetType = onsetOffsets.at(onset).second;

        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.timeForStep(offset);
        f.hasDuration = false;
        switch (offsetType) {
        case CoreFeatures::OffsetType::PowerDrop:
            f.label = "Power Drop";
            break;
        case CoreFeatures::OffsetType::SpectralLevelDrop:
            f.label = "Spectral Drop";
            break;
        case CoreFeatures::OffsetType::FollowingOnsetReached:
            f.label = "Following Onset Reached";
            break;
        }
        fs[m_offsetOutput].push_back(f);
    }

    auto rawPower = m_coreFeatures.getRawPower_dB();
        
    for (size_t i = 0; i < rawPower.size(); ++i) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.timeForStep(i);
        f.values.push_back(rawPower[i]);
        fs[m_rawPowerOutput].push_back(f);
    }

    auto spectralDropDf = m_coreFeatures.getOffsetDropDF();
        
    for (size_t i = 0; i < rawPower.size(); ++i) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.timeForStep(i);
        f.values.push_back(spectralDropDf[i]);
        fs[m_spectralDropDfOutput].push_back(f);
    }
    
    return fs;
}

