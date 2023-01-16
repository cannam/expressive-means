
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
#include <memory>

/** Extractor for features (pitch, onsets etc) that Expressive Means
 *  plugins have in common.
 */
class CoreFeatures
{
public:
    CoreFeatures(double sampleRate) :
        m_sampleRate(sampleRate),
        m_initialised(false),
        m_finished(false),
        m_pyin(sampleRate)
    { }

    size_t getPreferredBlockSize() const {
        return m_pyin.getPreferredBlockSize();
    }

    size_t getPreferredStepSize() const {
        return m_pyin.getPreferredStepSize();
    }
    
    struct PYinParameters {
        float thresholdDistribution;
        float lowAmplitudeSuppressionThreshold;
        PYinParameters() :
            thresholdDistribution(2.f),
            lowAmplitudeSuppressionThreshold(0.1f)
        {}
    };
    
    struct Parameters {
        PYinParameters pyinParameters;
        Power::Parameters powerParameters;
        SpectralLevelRise::Parameters onsetLevelRiseParameters;
        SpectralLevelRise::Parameters noiseRatioLevelRiseParameters;
        int stepSize;
        int blockSize;
        float pitchAverageWindow_ms;                // 2.1, o_1
        float onsetSensitivityPitch_cents;          // 2.2, o_2
        float onsetSensitivityNoise_percent;        // 2.3, o_3
        float onsetSensitivityLevel_dB;             // 2.4, o_4
        float onsetSensitivityNoiseTimeWindow_ms;   // 2.5, o_5
        float minimumOnsetInterval_ms;              // 2.6, o_6
        Parameters() :
            stepSize(256),
            blockSize(2048),
            pitchAverageWindow_ms(150.f),
            onsetSensitivityPitch_cents(15.f),
            onsetSensitivityNoise_percent(40.f),
            onsetSensitivityLevel_dB(8.f),
            onsetSensitivityNoiseTimeWindow_ms(100.f),
            minimumOnsetInterval_ms(100.f)
        {}
    };

    void initialise(Parameters parameters) {
        if (m_initialised) {
            throw std::logic_error("Features::initialise: Already initialised");
        }

        auto pyinOutputs = m_pyin.getOutputDescriptors();
        m_pyinSmoothedPitchTrackOutput = -1;
        for (int i = 0; i < int(pyinOutputs.size()); ++i) {
            if (pyinOutputs[i].identifier == "smoothedpitchtrack") {
                m_pyinSmoothedPitchTrackOutput = i;
            }
        }
        if (m_pyinSmoothedPitchTrackOutput < 0) {
            throw std::logic_error("pYIN smoothed pitch track output not found");
        }
        
        m_pyin.setParameter("outputunvoiced", 2); // As negative frequencies
        m_pyin.setParameter
            ("threshdistr", parameters.pyinParameters.thresholdDistribution);
        m_pyin.setParameter
            ("lowampsuppression",
             parameters.pyinParameters.lowAmplitudeSuppressionThreshold);
        if (!m_pyin.initialise(1, parameters.stepSize, parameters.blockSize)) {
            throw std::logic_error("pYIN initialisation failed");
        }
        
        m_power.initialise(parameters.powerParameters);
        m_onsetLevelRise.initialise(parameters.onsetLevelRiseParameters);
        m_noiseRatioLevelRise.initialise
            (parameters.noiseRatioLevelRiseParameters);

        m_stepSize = parameters.stepSize;
        m_blockSize = parameters.blockSize;
        m_pitchAverageWindow_ms = parameters.pitchAverageWindow_ms;
        m_onsetSensitivityPitch_cents = parameters.onsetSensitivityPitch_cents;
        m_onsetSensitivityNoise_percent = parameters.onsetSensitivityNoise_percent;
        m_onsetSensitivityLevel_dB = parameters.onsetSensitivityLevel_dB;
        m_onsetSensitivityNoiseTimeWindow_ms = parameters.onsetSensitivityNoiseTimeWindow_ms;
        m_minimumOnsetInterval_ms = parameters.minimumOnsetInterval_ms;
        
        m_initialised = true;
    };

    void reset() {
        if (!m_initialised) {
            throw std::logic_error("Features::reset: Never initialised");
        }
        m_finished = false;

        m_pyin.reset();
        m_power.reset();
        m_onsetLevelRise.reset();
        m_noiseRatioLevelRise.reset();

        m_pyinPitchHz.clear();
        m_pyinTimestamps.clear();
        m_pitch.clear();
        m_filteredPitch.clear();
        m_pitchOnsetDf.clear();
        m_rawPower.clear();
        m_smoothedPower.clear();
        m_pitchOnsets.clear();
        m_levelRiseOnsets.clear();
        m_allOnsets.clear();
        m_mergedOnsets.clear();
    }
    
    void
    process(const float *input, Vamp::RealTime timestamp) {
        if (!m_initialised) {
            throw std::logic_error("Features::process: Not initialised");
        }
        if (m_finished) {
            throw std::logic_error("Features::process: Already finished");
        }
        const float *const *iptr = &input;
        auto pyinFeatures = m_pyin.process(iptr, timestamp);
        for (const auto &f: pyinFeatures[m_pyinSmoothedPitchTrackOutput]) {
            m_pyinPitchHz.push_back(f.values[0]);
            m_pyinTimestamps.push_back(f.timestamp);
        }

        m_power.process(input);
        m_onsetLevelRise.process(input);
        m_noiseRatioLevelRise.process(input);
    }

    void
    finish() {
        if (m_finished) {
            throw std::logic_error("Features::finish: Already finished");
        }
        auto pyinFeatures = m_pyin.getRemainingFeatures();
        for (const auto &f: pyinFeatures[m_pyinSmoothedPitchTrackOutput]) {
            m_pyinPitchHz.push_back(f.values[0]);
            m_pyinTimestamps.push_back(f.timestamp);
        }

        double prevHz = 0.0;
        for (auto hz : m_pyinPitchHz) {
            if (hz > 0.0) {
                m_pitch.push_back(hzToPitch(hz));
                prevHz = hz;
            } else if (prevHz > 0.0) {
                m_pitch.push_back(hzToPitch(prevHz));
            } else {
                m_pitch.push_back(0.0);
            }
        }

        // "If the absolute difference of a pitch and its following
        // moving pitch average window falls below o_2" - calculate a
        // moving mean window over the pitch curve (which is in
        // semitones, not Hz) and compare each pitch to the mean
        // within the window that follows it: if they are close,
        // record an onset
    
        int pitchFilterLength = msToSteps(m_pitchAverageWindow_ms, m_stepSize, true);
        int halfLength = pitchFilterLength/2;
        MeanFilter pitchFilter(pitchFilterLength);
        int n = m_pitch.size();
        m_filteredPitch = vector<double>(n, 0.0);
        pitchFilter.filter(m_pitch.data(), m_filteredPitch.data(), n);
    
        for (int i = 0; i + halfLength < n; ++i) {
            m_pitchOnsetDf.push_back
                (fabsf(m_pitch[i] - m_filteredPitch[i + halfLength]));
        }

        int minimumOnsetSteps = msToSteps(m_minimumOnsetInterval_ms, m_stepSize, false);
        int lastBelowThreshold = -minimumOnsetSteps;
        double threshold = m_onsetSensitivityPitch_cents / 100.0;

        for (int i = 0; i + halfLength < n; ++i) {
            // "absolute difference... falls below o_2":
            if (m_pitchOnsetDf[i] < threshold) {
                // "subsequent onsets require o_2 to be exceeded for at
                // least the duration of o_6 first":
                if (i > lastBelowThreshold + minimumOnsetSteps) {
                    m_pitchOnsets.insert(i);
                }
                lastBelowThreshold = i;
            }
        }
    
        std::vector<double> riseFractions = m_onsetLevelRise.getFractions();
        double upperThreshold = m_onsetSensitivityNoise_percent / 100.0;
        double lowerThreshold = upperThreshold / 2.0;
        bool aboveThreshold = false;
        for (int i = 0; i < int(riseFractions.size()); ++i) {
            // Watch for the level to rise above threshold, then wait
            // for it to fall again and identify that moment as the
            // onset.
            //            
            // NB SpectralLevelRise only starts recording values once
            // its history buffer (say length n) is full. So the value
            // at i indicates the fraction of bins that saw a
            // significant rise during the n steps starting at input
            // step i. Step i is calculated from time-domain samples
            // between sample i*stepSize and i*stepSize + blockSize,
            // so it reflects activity around the centre of the window
            // at i*stepSize + blockSize/2.
            //
            if (riseFractions[i] > upperThreshold) {
                aboveThreshold = true;
            } else if (riseFractions[i] < lowerThreshold) {
                if (aboveThreshold) {
                    m_levelRiseOnsets.insert(i + (m_blockSize / m_stepSize)/2);
                    aboveThreshold = false;
                }
            }
        }
        
        m_allOnsets = m_pitchOnsets;
        m_allOnsets.insert(m_levelRiseOnsets.begin(), m_levelRiseOnsets.end());

        int prevP = -minimumOnsetSteps;
        for (auto p: m_allOnsets) {
            if (p < prevP + minimumOnsetSteps) {
                continue;
            }
            m_mergedOnsets.insert(p);
            prevP = p;
        }

        m_rawPower = m_power.getRawPower();
        m_smoothedPower = m_power.getSmoothedPower();
        
        m_finished = true;
    }

    std::vector<double>
    getPYinPitch_Hz() const {
        assertFinished();
        return m_pyinPitchHz;
    }

    std::vector<Vamp::RealTime>
    getPYinTimestamps() const {
        assertFinished();
        return m_pyinTimestamps;
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

    std::vector<double>
    getNoiseRatioLevelRiseFractions() const {
        assertFinished();
        return m_noiseRatioLevelRise.getFractions();
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
    getMergedOnsets() const {
        assertFinished();
        return m_mergedOnsets;
    }

    PYinVamp::ParameterList
    getPYinParameterDescriptors() const {
        return m_pyin.getParameterDescriptors();
    }
    
    int msToSteps(float ms, int stepSize, bool odd) const {
        int n = ceil((ms / 1000.0) * m_sampleRate / stepSize);
        if (odd && (n % 2 == 0)) ++n;
        return n;
    }

    double hzToPitch(double hz) const {
        double p = 12.0 * (log(hz / 220.0) / log(2.0)) + 57.0;
        return p;
    }

    CoreFeatures(const CoreFeatures &) =delete;
    CoreFeatures &operator=(const CoreFeatures &) =delete;
    
private:
    double m_sampleRate;
    int m_stepSize;
    int m_blockSize;
    bool m_initialised;
    bool m_finished;
    
    PYinVamp m_pyin; // Not copyable by value, though, so we can't
                     // have default operator= etc
    Power m_power;
    SpectralLevelRise m_onsetLevelRise;
    SpectralLevelRise m_noiseRatioLevelRise;

    float m_pitchAverageWindow_ms;              // 2.1, o_1
    float m_onsetSensitivityPitch_cents;        // 2.2, o_2
    float m_onsetSensitivityNoise_percent;      // 2.3, o_3
    float m_onsetSensitivityLevel_dB;           // 2.4, o_4
    float m_onsetSensitivityNoiseTimeWindow_ms; // 2.5, o_5
    float m_minimumOnsetInterval_ms;            // 2.6, o_6
    
    int m_pyinSmoothedPitchTrackOutput;
    std::vector<double> m_pyinPitchHz;
    std::vector<Vamp::RealTime> m_pyinTimestamps;
    std::vector<double> m_pitch;
    std::vector<double> m_filteredPitch;
    std::vector<double> m_pitchOnsetDf;
    std::vector<double> m_rawPower;
    std::vector<double> m_smoothedPower;
    std::set<int> m_pitchOnsets;
    std::set<int> m_levelRiseOnsets;
    std::set<int> m_allOnsets;
    std::set<int> m_mergedOnsets;

    void assertFinished() const {
        if (!m_finished) {
            throw std::logic_error("Features: feature retrieval attempted before finish() called");
        }
    }
};

#endif
