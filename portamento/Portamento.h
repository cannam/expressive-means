
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
    
    bool m_haveStartTime;
    Vamp::RealTime m_startTime;
    
    CoreFeatures m_coreFeatures;

    float m_pyinThresholdDistribution;
    float m_pyinLowAmpSuppression;
    
    float m_pitchAverageWindow_ms;
    float m_onsetSensitivityPitch_cents;
    float m_onsetSensitivityNoise_percent;
    float m_onsetSensitivityLevel_dB;
    float m_onsetSensitivityNoiseTimeWindow_ms;
    float m_minimumOnsetInterval_ms;

/*!!!
    mutable int m_summaryOutput;
    mutable int m_portamentoTypeOutput;
    mutable int m_pitchTrackOutput;
    mutable int m_portamentoIndexOutput;
*/

};

#endif
