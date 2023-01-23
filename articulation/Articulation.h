
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

    enum class NoiseType {
        Sonorous = 0,
        Fricative = 1,
        Plosive = 2,
        Affricative = 3
    };

    static std::string noiseTypeToString(NoiseType t) {
        switch (t) {
        case NoiseType::Sonorous: return "Sonorous";
        case NoiseType::Fricative: return "Fricative";
        case NoiseType::Plosive: return "Plosive";
        case NoiseType::Affricative: return "Affricative";
        default: throw std::logic_error("unknown NoiseType");
        }
    }
    
    enum class LevelDevelopment {
        Unclassifiable,
        Decreasing, DeAndIncreasing, Constant, InAndDecreasing, Increasing,
        Other
    };

    static std::string developmentToString(LevelDevelopment d) {
        switch (d) {
        case LevelDevelopment::Unclassifiable: return "Unclassifiable";
        case LevelDevelopment::Decreasing: return "Decreasing";
        case LevelDevelopment::DeAndIncreasing: return "De-and-Increasing";
        case LevelDevelopment::Constant: return "Constant";
        case LevelDevelopment::InAndDecreasing: return "In-And-Decreasing";
        case LevelDevelopment::Increasing: return "Increasing";
        case LevelDevelopment::Other: return "Other";
        default: throw std::logic_error("unknown LevelDevelopment");
        }
    }

    static LevelDevelopment classifyLevelDevelopment(double sustainBeginPower,
                                                     double sustainEndPower,
                                                     double maxBetweenPower,
                                                     double minBetweenPower,
                                                     double threshold);

protected:
    int m_stepSize;
    int m_blockSize;
    
    bool m_haveStartTime;
    Vamp::RealTime m_startTime;
    
    CoreFeatures m_coreFeatures;
    SpectralLevelRise m_noiseRatioLevelRise;

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
    float m_onsetSensitivityRawPowerThreshold_dB;
    float m_minimumOnsetInterval_ms;            // 2.6, o_6
    float m_sustainBeginThreshold_ms;           // 4.1, b_1
    float m_noteDurationThreshold_dB;
    float m_volumeDevelopmentThreshold_dB;      // 4.3, b_3
    float m_scalingFactor;                      // 6, s
    
    mutable int m_summaryOutput;
    mutable int m_noiseTypeOutput;
    mutable int m_volumeDevelopmentOutput;
    mutable int m_articulationTypeOutput;
    mutable int m_pitchTrackOutput;
    mutable int m_articulationIndexOutput;
    
#ifdef WITH_DEBUG_OUTPUTS
    mutable int m_rawPowerOutput;
    mutable int m_smoothedPowerOutput;
    mutable int m_filteredPitchOutput;
    mutable int m_pitchOnsetDfOutput;
    mutable int m_transientOnsetDfOutput;
    mutable int m_noiseRatioOutput;
    mutable int m_relativeDurationOutput;
    mutable int m_onsetOutput;
#endif
};

#endif
