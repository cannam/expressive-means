
/*
    Expressive Means

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef EXPRESSIVE_MEANS_CORE_FEATURES_H
#define EXPRESSIVE_MEANS_CORE_FEATURES_H

#include "Power.h"
#include "SpectralLevelRise.h"

#include "../ext/pyin/PYinVamp.h"

#include <vector>
#include <set>
#include <map>
#include <memory>

/** Extractor for features (pitch, onsets etc) that Expressive Means
 *  plugins have in common.
 */
class CoreFeatures
{
public:
    CoreFeatures(double sampleRate);

    size_t getPreferredBlockSize() const {
        return m_pyin.getPreferredBlockSize();
    }

    size_t getPreferredStepSize() const {
        return m_pyin.getPreferredStepSize();
    }

    struct Parameters {
        int stepSize;
        int blockSize;
        float pyinThresholdDistribution;
        float pyinLowAmpSuppressionThreshold;
        float pitchAverageWindow_ms;                // 2.1, o_1
        float onsetSensitivityPitch_cents;          // 2.2, o_2
        float onsetSensitivityNoise_percent;        // 2.3, o_3
        float onsetSensitivityLevel_dB;             // 2.4, o_4
        float onsetSensitivityNoiseTimeWindow_ms;   // 2.5, o_5
        float onsetSensitivityRawPowerThreshold_dB;
        float minimumOnsetInterval_ms;              // 2.6, o_6
        float sustainBeginThreshold_ms;
        float noteDurationThreshold_dB;             // 2.7, o_7
        float spectralDropFloor_dB;
        float spectralFrequencyMin_Hz;
        float spectralFrequencyMax_Hz;

        Parameters() :
            stepSize(256),
            blockSize(2048),
            pyinThresholdDistribution(2.f),
            pyinLowAmpSuppressionThreshold(0.1f),
            pitchAverageWindow_ms(150.f),
            onsetSensitivityPitch_cents(15.f),
            onsetSensitivityNoise_percent(24.f),
            onsetSensitivityLevel_dB(8.f),
            onsetSensitivityNoiseTimeWindow_ms(100.f),
            onsetSensitivityRawPowerThreshold_dB(6.f),
            minimumOnsetInterval_ms(100.f),
            sustainBeginThreshold_ms(60.f),
            noteDurationThreshold_dB(12.f),
            spectralDropFloor_dB(-70.f),
            spectralFrequencyMin_Hz(100.f),
            spectralFrequencyMax_Hz(4000.f)
        {}

        static void appendVampParameterDescriptors(Vamp::Plugin::ParameterList &);

        bool acceptVampParameter(std::string identifier, float value);
        bool obtainVampParameter(std::string identifier, float &value) const;
    };

    enum class OnsetType {
        Pitch,
        SpectralLevelRise,
        PowerRise
    };

    enum class OffsetType {
        PowerDrop,
        SpectralLevelDrop,
        FollowingOnsetReached
    };

    void initialise(Parameters parameters);
    void reset();
    void process(const float *input, Vamp::RealTime timestamp);
    void finish();

    std::vector<double>
    getPYinPitch_Hz() const {
        assertFinished();
        return m_pyinPitchHz;
    }
    
    std::vector<double>
    getPitch_semis() const {
        assertFinished();
        return m_pitch;
    }

    std::vector<double>
    getFilteredPitch_semis() const {
        assertFinished();
        return m_filteredPitch;
    }

    std::vector<double>
    getPitchOnsetDF() const {
        assertFinished();
        return m_pitchOnsetDf;
    }

    std::vector<double>
    getRawPower_dB() const {
        assertFinished();
        return m_rawPower;
    }
    
    std::vector<double>
    getSmoothedPower_dB() const {
        assertFinished();
        return m_smoothedPower;
    }

    std::vector<double>
    getOnsetLevelRiseFractions() const {
        assertFinished();
        return m_onsetLevelRise.getFractions();
    }

    int
    getOnsetBinCount() const {
        assertFinished();
        return m_onsetLevelRise.getBinCount();
    }
    
    std::vector<int>
    getOnsetBinsAboveFloorAt(int step) const {
        assertFinished();
        return m_onsetLevelRise.getBinsAboveFloorAt(step);
    }

    std::vector<double>
    getOffsetDropDF() const {
        assertFinished();
        return m_offsetDropDf;
    }
    
    std::set<int>
    getPitchOnsets() const {
        assertFinished();
        return m_pitchOnsets;
    }

    std::set<int>
    getLevelRiseOnsets() const {
        assertFinished();
        return m_levelRiseOnsets;
    }

    std::set<int>
    getPowerRiseOnsets() const {
        assertFinished();
        return m_powerRiseOnsets;
    }

    std::map<int, OnsetType>
    getMergedOnsets() const {
        assertFinished();
        return m_mergedOnsets;
    }

    typedef std::map<int, std::pair<int, OffsetType>> OnsetOffsetMap;

    OnsetOffsetMap
    getOnsetOffsets() const {
        assertFinished();
        return m_onsetOffsets;
    }

    Vamp::RealTime getStartTime() const {
        return m_startTime;
    }
    
    Vamp::RealTime timeForStep(int step) const {
        // See notes about timing alignment in finish() in the .cpp file
        int halfBlock = (m_parameters.blockSize / m_parameters.stepSize) / 2;
        return m_startTime + Vamp::RealTime::frame2RealTime
            ((step + halfBlock) * m_parameters.stepSize, m_sampleRate);
    }
    
    int msToSteps(float ms, int stepSize, bool odd) const {
        int n = ceil((ms / 1000.0) * m_sampleRate / stepSize);
        if (odd && (n % 2 == 0)) ++n;
        return n;
    }

    static double hzToPitch(double hz) {
        double p = 12.0 * (log(hz / 220.0) / log(2.0)) + 57.0;
        return p;
    }

    CoreFeatures(const CoreFeatures &) =delete;
    CoreFeatures &operator=(const CoreFeatures &) =delete;
    
private:
    double m_sampleRate;
    bool m_initialised;
    bool m_finished;
    Parameters m_parameters;
    
    bool m_haveStartTime;
    Vamp::RealTime m_startTime;

    PYinVamp m_pyin; // Not copyable by value, though, so we can't
                     // have default operator= etc
    Power m_power;
    SpectralLevelRise m_onsetLevelRise;

    int m_pyinSmoothedPitchTrackOutput;
    std::vector<double> m_pyinPitchHz;
    std::vector<double> m_pitch;
    std::vector<double> m_filteredPitch;
    std::vector<double> m_pitchOnsetDf;
    std::vector<double> m_rawPower;
    std::vector<double> m_smoothedPower;
    std::vector<double> m_offsetDropDf;
    std::set<int> m_pitchOnsets;
    std::set<int> m_levelRiseOnsets;
    std::set<int> m_powerRiseOnsets;
    std::map<int, OnsetType> m_mergedOnsets;
    std::map<int, std::pair<int, OffsetType>> m_onsetOffsets;

    void assertFinished() const {
        if (!m_finished) {
            throw std::logic_error("Features: feature retrieval attempted before finish() called");
        }
    }
};

#endif
