
/*
    Expressive Means Portamento

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef EXPRESSIVE_MEANS_PORTAMENTO_H
#define EXPRESSIVE_MEANS_PORTAMENTO_H

#include <vamp-sdk/Plugin.h>

#include "common/CoreFeatures.h"

#define WITH_DEBUG_OUTPUTS 1

using std::string;

class Portamento : public Vamp::Plugin
{
public:
    Portamento(float inputSampleRate);
    virtual ~Portamento();

    string getIdentifier() const;
    string getName() const;
    string getDescription() const;
    string getMaker() const;
    int getPluginVersion() const;
    string getCopyright() const;

    InputDomain getInputDomain() const;
    size_t getPreferredBlockSize() const;
    size_t getPreferredStepSize() const;
    size_t getMinChannelCount() const;
    size_t getMaxChannelCount() const;

    ParameterList getParameterDescriptors() const;
    float getParameter(string identifier) const;
    void setParameter(string identifier, float value);

    ProgramList getPrograms() const;
    string getCurrentProgram() const;
    void selectProgram(string name);

    OutputList getOutputDescriptors() const;

    bool initialise(size_t channels, size_t stepSize, size_t blockSize);
    void reset();

    FeatureSet process(const float *const *inputBuffers,
                       Vamp::RealTime timestamp);

    FeatureSet getRemainingFeatures();

protected:
    int m_stepSize;
    int m_blockSize;
    
    CoreFeatures m_coreFeatures;

    CoreFeatures::Parameters m_coreParams;
    float m_glideThresholdPitch_cents;  // 3.1, g_1
    float m_glideThresholdDuration_ms;  // 3.2, g_2
    float m_glideThresholdProximity_ms; // 3.3, g_3
    
    mutable int m_summaryOutput;
    mutable int m_portamentoTypeOutput;
    mutable int m_pitchTrackOutput;
    mutable int m_portamentoIndexOutput;

#ifdef WITH_DEBUG_OUTPUTS
    mutable int m_pitchDiffOutput1;
    mutable int m_pitchDiffOutput2;
    mutable int m_candidateHopsOutput;
    mutable int m_portamentoPointsOutput;
#endif
};

#endif
