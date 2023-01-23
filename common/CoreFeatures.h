
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
        int stepSize;
        int blockSize;
        float pitchAverageWindow_ms;                // 2.1, o_1
        float onsetSensitivityPitch_cents;          // 2.2, o_2
        float onsetSensitivityNoise_percent;        // 2.3, o_3
        float onsetSensitivityLevel_dB;             // 2.4, o_4
        float onsetSensitivityNoiseTimeWindow_ms;   // 2.5, o_5
        float onsetSensitivityRawPowerThreshold_dB;
        float minimumOnsetInterval_ms;              // 2.6, o_6
        float sustainBeginThreshold_ms;
        float noteDurationThreshold_dB;             // 2.7, o_7
        Parameters() :
            stepSize(256),
            blockSize(2048),
            pitchAverageWindow_ms(150.f),
            onsetSensitivityPitch_cents(15.f),
            onsetSensitivityNoise_percent(24.f),
            onsetSensitivityLevel_dB(8.f),
            onsetSensitivityNoiseTimeWindow_ms(100.f),
            onsetSensitivityRawPowerThreshold_dB(6.f),
            minimumOnsetInterval_ms(100.f),
            sustainBeginThreshold_ms(60.f),
            noteDurationThreshold_dB(6.f)
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

        m_stepSize = parameters.stepSize;
        m_blockSize = parameters.blockSize;
        m_pitchAverageWindow_ms = parameters.pitchAverageWindow_ms;
        m_onsetSensitivityPitch_cents = parameters.onsetSensitivityPitch_cents;
        m_onsetSensitivityNoise_percent = parameters.onsetSensitivityNoise_percent;
        m_onsetSensitivityLevel_dB = parameters.onsetSensitivityLevel_dB;
        m_onsetSensitivityNoiseTimeWindow_ms = parameters.onsetSensitivityNoiseTimeWindow_ms;
        m_onsetSensitivityRawPowerThreshold_dB = parameters.onsetSensitivityRawPowerThreshold_dB;
        m_minimumOnsetInterval_ms = parameters.minimumOnsetInterval_ms;
        m_sustainBeginThreshold_ms = parameters.sustainBeginThreshold_ms;
        m_noteDurationThreshold_dB = parameters.noteDurationThreshold_dB;
        
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

        m_pyinPitchHz.clear();
        m_pyinTimestamps.clear();
        m_pitch.clear();
        m_filteredPitch.clear();
        m_pitchOnsetDf.clear();
        m_rawPower.clear();
        m_smoothedPower.clear();
        m_pitchOnsets.clear();
        m_levelRiseOnsets.clear();
        m_powerRiseOnsets.clear();
        m_allOnsets.clear();
        m_mergedOnsets.clear();
        m_onsetOffsets.clear();
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

        int minimumOnsetSteps = msToSteps
            (m_minimumOnsetInterval_ms, m_stepSize, false);
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

        m_rawPower = m_power.getRawPower();
        int rawPowerSteps = msToSteps(50.0, m_stepSize, false);
        bool onsetComing = false;
        double prevDerivative = 0.0;
        // Iterate through raw power, and when we see a rise above a
        // certain level within the following rawPowerSteps, make note
        // that we have an onset coming (onsetComing = true). But
        // don't actually record the onset (insert into
        // m_powerRiseOnsets) until we see the derivative of raw power
        // begin to fall again, otherwise the onset appears early.
        for (int i = 0; i + 1 < m_rawPower.size(); ++i) {
            double derivative = m_rawPower[i+1] - m_rawPower[i];
            if (onsetComing) {
                if (derivative < prevDerivative) {
                    // Like level rise, power is offset by half a block
                    m_powerRiseOnsets.insert(i + (m_blockSize / m_stepSize)/2);
                    onsetComing = false;
                }
            } else if (i + rawPowerSteps < m_rawPower.size()) {
                for (int j = i; j <= i + rawPowerSteps; ++j) {
                    if (m_rawPower[j] < m_rawPower[i]) {
                        break;
                    }
                    if (m_rawPower[j] > m_rawPower[i] +
                        m_onsetSensitivityRawPowerThreshold_dB) {
                        onsetComing = true;
                        break;
                    }
                }
            }
            prevDerivative = derivative;
        }
        
        m_allOnsets = m_pitchOnsets;
        m_allOnsets.insert(m_levelRiseOnsets.begin(), m_levelRiseOnsets.end());
        m_allOnsets.insert(m_powerRiseOnsets.begin(), m_powerRiseOnsets.end());

        int prevP = -minimumOnsetSteps;
        for (auto p: m_allOnsets) {
            if (p < prevP + minimumOnsetSteps) {
                continue;
            }
            m_mergedOnsets.insert(p);
            prevP = p;
        }

        m_smoothedPower = m_power.getSmoothedPower();
        n = m_smoothedPower.size();

        int sustainBeginSteps = msToSteps
            (m_sustainBeginThreshold_ms, m_stepSize, false);

        for (auto i = m_mergedOnsets.begin(); i != m_mergedOnsets.end(); ++i) {
            int p = *i;
            int limit = n;
            auto j = i;
            if (++j != m_mergedOnsets.end()) {
                limit = *j; // stop at the next onset
            }
            int s = p + sustainBeginSteps;
            if (s < n) {
                std::cerr << "power " << m_smoothedPower[s] << ", threshold "
                          << m_noteDurationThreshold_dB
                          << ", gives target power "
                          << m_smoothedPower[s] - m_noteDurationThreshold_dB
                          << std::endl;
            } else {
                std::cerr << "sustain start index " << s
                          << " out of range at end" << std::endl;
            }
            int q = s;
            while (q < limit) {
                if (m_smoothedPower[q] <
                    m_smoothedPower[s] - m_noteDurationThreshold_dB) {
                    break;
                }
                ++q;
            }
            if (q > limit) {
                m_onsetOffsets[p] = limit;
            } else {
                m_onsetOffsets[p] = q;
            }
        }
        
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

    std::set<int>
    getMergedOnsets() const {
        assertFinished();
        return m_mergedOnsets;
    }

    std::map<int, int>
    getOnsetOffsets() const {
        assertFinished();
        return m_onsetOffsets;
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

    float m_pitchAverageWindow_ms;              // 2.1, o_1
    float m_onsetSensitivityPitch_cents;        // 2.2, o_2
    float m_onsetSensitivityNoise_percent;      // 2.3, o_3
    float m_onsetSensitivityLevel_dB;           // 2.4, o_4
    float m_onsetSensitivityNoiseTimeWindow_ms; // 2.5, o_5
    float m_onsetSensitivityRawPowerThreshold_dB;
    float m_minimumOnsetInterval_ms;            // 2.6, o_6
    float m_sustainBeginThreshold_ms;
    float m_noteDurationThreshold_dB;           // 2.7, o_7
    
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
    std::set<int> m_powerRiseOnsets;
    std::set<int> m_allOnsets;
    std::set<int> m_mergedOnsets;
    std::map<int, int> m_onsetOffsets;

    void assertFinished() const {
        if (!m_finished) {
            throw std::logic_error("Features: feature retrieval attempted before finish() called");
        }
    }
};

#endif
