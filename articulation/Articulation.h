
/*
    Expressive Means Articulation

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef EXPRESSIVE_MEANS_ARTICULATION_H
#define EXPRESSIVE_MEANS_ARTICULATION_H

#include <vamp-sdk/Plugin.h>

#include "common/CoreFeatures.h"

#define WITH_DEBUG_OUTPUTS 1

using std::string;

class Articulation : public Vamp::Plugin
{
public:
    Articulation(float inputSampleRate);
    virtual ~Articulation();

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
    
    // Our parameters. Currently only those with simple single
    // floating-point values are provided. Multiple floating-point
    // values (e.g. 3.1, a_1.x impulse noise ratio boundaries) could
    // be added as multiple parameters; those with non-numeric values
    // (e.g. 3.2, a_2.x impulse type codes) can't.
    
    float m_pitchAverageWindow_ms;              // 2.1, o_1
    float m_onsetSensitivityPitch_cents;        // 2.2, o_2
    float m_onsetSensitivityNoise_percent;      // 2.3, o_3
    float m_onsetSensitivityLevel_dB;           // 2.4, o_4
    float m_onsetSensitivityNoiseTimeWindow_ms; // 2.5, o_5
    float m_minimumOnsetInterval_ms;            // 2.6, o_6
    float m_sustainBeginThreshold_ms;           // 4.1, b_1
    float m_volumeDevelopmentThreshold_dB;      // 4.3, b_3
    float m_scalingFactor;                      // 6, s

    mutable int m_summaryOutput;
    mutable int m_articulationTypeOutput;
    mutable int m_pitchTrackOutput;
    mutable int m_articulationIndexOutput;

#ifdef WITH_DEBUG_OUTPUTS
    mutable int m_powerOutput;
    mutable int m_filteredPitchOutput;
    mutable int m_pitchOnsetDfOutput;
    mutable int m_transientOnsetDfOutput;
    mutable int m_onsetOutput;
#endif
};

#endif
