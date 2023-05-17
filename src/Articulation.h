
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

#include "CoreFeatures.h"

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
        Unclassifiable,
        Sonorous, Fricative, Plosive, Affricative
    };

    static std::string noiseTypeToString(NoiseType t) {
        switch (t) {
        case NoiseType::Unclassifiable: return "Unclassifiable";
        case NoiseType::Sonorous: return "Sonorous";
        case NoiseType::Fricative: return "Fricative";
        case NoiseType::Plosive: return "Plosive";
        case NoiseType::Affricative: return "Affricative";
        default: throw std::logic_error("unknown NoiseType");
        }
    }

    static std::string noiseTypeToCode(NoiseType t) {
        switch (t) {
        case NoiseType::Unclassifiable: return "?";
        case NoiseType::Sonorous: return "s";
        case NoiseType::Fricative: return "f";
        case NoiseType::Plosive: return "p";
        case NoiseType::Affricative: return "a";
        default: throw std::logic_error("unknown NoiseType");
        }
    }

    static double noiseTypeToFactor(NoiseType t) {
        switch (t) {
        case NoiseType::Unclassifiable: return 1.0;
        case NoiseType::Sonorous: return 1.0;
        case NoiseType::Fricative: return 2.0;
        case NoiseType::Plosive: return 3.0;
        case NoiseType::Affricative: return 5.0;
        default: throw std::logic_error("unknown NoiseType");
        }
    }

    struct NoiseRec {
        double total;
        NoiseType type;
        NoiseRec() : total(0.0), type(NoiseType::Unclassifiable) { }
    };
        
    static NoiseRec classifyOnsetNoise(const std::vector<std::vector<int>> &
                                       activeBinsAfterOnset,
                                       int binCount,
                                       double plosiveRatio,
                                       double fricativeRatio,
                                       bool forceSonorous);

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

    static std::string developmentToCode(LevelDevelopment d) {
        switch (d) {
        case LevelDevelopment::Unclassifiable:  return "?";
        case LevelDevelopment::Decreasing:      return ">";
        case LevelDevelopment::DeAndIncreasing: return ":";
        case LevelDevelopment::Constant:        return "=";
        case LevelDevelopment::InAndDecreasing: return ":";
        case LevelDevelopment::Increasing:      return "<";
        case LevelDevelopment::Other:           return ":";
        default: throw std::logic_error("unknown LevelDevelopment");
        }
    }

    static double developmentToFactor(LevelDevelopment d) {
        switch (d) {
        case LevelDevelopment::Unclassifiable:  return 1.0;
        case LevelDevelopment::Decreasing:      return 0.75;
        case LevelDevelopment::DeAndIncreasing: return 1.13;
        case LevelDevelopment::Constant:        return 1.0;
        case LevelDevelopment::InAndDecreasing: return 1.13;
        case LevelDevelopment::Increasing:      return 1.25;
        case LevelDevelopment::Other:           return 1.13;
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
    
    CoreFeatures m_coreFeatures;
    
    // Our parameters. Currently only those with simple single
    // floating-point values are provided. Multiple floating-point
    // values (e.g. 3.1, a_1.x impulse noise ratio boundaries) could
    // be added as multiple parameters; those with non-numeric values
    // (e.g. 3.2, a_2.x impulse type codes) can't.

    CoreFeatures::Parameters m_coreParams;
    float m_volumeDevelopmentThreshold_dB;      // 4.3, b_3
    float m_scalingFactor;                      // 6, s
    float m_impulseNoiseRatioPlosive_percent;
    float m_impulseNoiseRatioFricative_percent;
    float m_reverbDurationFactor;
    float m_overlapCompensationFactor;

    float m_glideThresholdPitch_cents;
    float m_glideThresholdHopMinimum_cents;
    float m_glideThresholdHopMaximum_cents;
    float m_glideThresholdDuration_ms;
    float m_glideThresholdProximity_ms;
    
    mutable int m_summaryOutput;
    mutable int m_noiseTypeOutput;
    mutable int m_volumeDevelopmentOutput;
    mutable int m_articulationTypeOutput;
    mutable int m_pitchTrackOutput;
    mutable int m_articulationIndexOutput;

    mutable int m_meanNoiseRatioOutput;
    mutable int m_meanDynamicsOutput;
    mutable int m_meanToneRatioOutput;
};

#endif
