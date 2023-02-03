
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

Portamento::Portamento(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_stepSize(0),
    m_blockSize(0),
    m_coreFeatures(inputSampleRate),
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

    m_coreParams.appendVampParameterDescriptors(list);
    
    ParameterDescriptor d;

    d.description = "";
    d.isQuantized = false;
    
    
    return list;
}

float
Portamento::getParameter(string identifier) const
{
    float value = 0.f;
    if (m_coreParams.obtainVampParameter(identifier, value)) {
        return value;
    }
    
    
    return 0.f;
}

void
Portamento::setParameter(string identifier, float value) 
{
    if (m_coreParams.acceptVampParameter(identifier, value)) {
        return;
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

    // Common to all
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = (m_inputSampleRate / m_stepSize);
    
    d.identifier = "summary";
    d.name = "Summary";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
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
    d.hasDuration = false;
    m_portamentoIndexOutput = int(list.size());
    list.push_back(d);
    
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

    try {
        m_coreParams.stepSize = m_stepSize;
        m_coreParams.blockSize = m_blockSize;
        m_coreFeatures.initialise(m_coreParams);
    } catch (const std::logic_error &e) {
        cerr << "ERROR: Portamento::initialise: Feature extractor initialisation failed: " << e.what() << endl;
        return false;
    }
    
    return true;
}

void
Portamento::reset()
{
    m_coreFeatures.reset();
}

Portamento::FeatureSet
Portamento::process(const float *const *inputBuffers, Vamp::RealTime timestamp)
{
    m_coreFeatures.process(inputBuffers[0], timestamp);
    return {};
}

Portamento::FeatureSet
Portamento::getRemainingFeatures()
{
    FeatureSet fs;

    m_coreFeatures.finish();

    auto pyinPitch = m_coreFeatures.getPYinPitch_Hz();

    for (int i = 0; i < int(pyinPitch.size()); ++i) {
        if (pyinPitch[i] <= 0) continue;
        Feature f;
        f.hasTimestamp = true;
        f.values.push_back(pyinPitch[i]);
        fs[m_pitchTrackOutput].push_back(f);
    }

    
    return fs;
}

