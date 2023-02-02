
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
            noteDurationThreshold_dB(6.f)
        {}
    };

    void initialise(Parameters parameters) {
        if (m_initialised) {
            throw std::logic_error("Features::initialise: Already initialised");
        }

        m_parameters = parameters;

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
        m_pyin.setParameter("threshdistr",
                            m_parameters.pyinThresholdDistribution);
        m_pyin.setParameter("lowampsuppression",
                            m_parameters.pyinLowAmpSuppressionThreshold);

        if (!m_pyin.initialise(1, m_parameters.stepSize, m_parameters.blockSize)) {
            throw std::logic_error("pYIN initialisation failed");
        }

        Power::Parameters powerParameters;
        powerParameters.blockSize = m_parameters.blockSize;
        m_power.initialise(powerParameters);

        SpectralLevelRise::Parameters levelRiseParameters;
        levelRiseParameters.sampleRate = m_sampleRate;
        levelRiseParameters.blockSize = m_parameters.blockSize;
        levelRiseParameters.dB = m_parameters.onsetSensitivityLevel_dB;
        levelRiseParameters.historyLength =
            msToSteps(m_parameters.onsetSensitivityNoiseTimeWindow_ms,
                      m_parameters.stepSize, false);
        if (levelRiseParameters.historyLength < 2) {
            levelRiseParameters.historyLength = 2;
        }
        m_onsetLevelRise.initialise(levelRiseParameters);

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
        m_mergedOnsets.clear();
        m_onsetOffsets.clear();
    }

    enum class OnsetType {
        Pitch,
        SpectralLevelRise,
        PowerRise
    };
    
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
    
        int pitchFilterLength = msToSteps(m_parameters.pitchAverageWindow_ms,
                                          m_parameters.stepSize, true);
        int halfLength = pitchFilterLength/2;
        MeanFilter pitchFilter(pitchFilterLength);
        int n = m_pitch.size();
        m_filteredPitch = vector<double>(n, 0.0);
        pitchFilter.filter(m_pitch.data(), m_filteredPitch.data(), n);
    
        for (int i = 0; i + halfLength < n; ++i) {
            m_pitchOnsetDf.push_back
                (fabsf(m_pitch[i] - m_filteredPitch[i + halfLength]));
        }

        int minimumOnsetSteps = msToSteps(m_parameters.minimumOnsetInterval_ms,
                                          m_parameters.stepSize, false);
        int lastBelowThreshold = -minimumOnsetSteps;
        double threshold = m_parameters.onsetSensitivityPitch_cents / 100.0;

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
        double upperThreshold = m_parameters.onsetSensitivityNoise_percent / 100.0;
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
                    m_levelRiseOnsets.insert(i + (m_parameters.blockSize /
                                                  m_parameters.stepSize)/2);
                    aboveThreshold = false;
                }
            }
        }

        m_rawPower = m_power.getRawPower();
        m_smoothedPower = m_power.getSmoothedPower();
        
        int rawPowerSteps = msToSteps(50.0, m_parameters.stepSize, false);
        bool onsetComing = false;
        double prevDerivative = 0.0;
        // Iterate through raw power, and when we see a rise above a
        // certain level within the following rawPowerSteps, make note
        // that we have an onset coming (onsetComing = true). But
        // don't actually record the onset (insert into
        // m_powerRiseOnsets) until we see the derivative of raw power
        // begin to fall again, otherwise the onset appears early.
        for (int i = 0; i + 1 < int(m_rawPower.size()); ++i) {
            double derivative = m_rawPower[i+1] - m_rawPower[i];
            if (onsetComing) {
                if (derivative < prevDerivative) {
                    // Like level rise, power is offset by half a block
                    m_powerRiseOnsets.insert(i + (m_parameters.blockSize /
                                                  m_parameters.stepSize)/2);
                    onsetComing = false;
                }
            } else if (i + rawPowerSteps < int(m_rawPower.size())) {
                for (int j = i; j <= i + rawPowerSteps; ++j) {
                    if (m_rawPower[j] < m_rawPower[i]) {
                        break;
                    }
                    if (m_rawPower[j] > m_rawPower[i] +
                        m_parameters.onsetSensitivityRawPowerThreshold_dB) {
                        onsetComing = true;
                        break;
                    }
                }
            }
            prevDerivative = derivative;
        }

        std::map<int, OnsetType> mergingOnsets;
        for (auto p : m_pitchOnsets) {
            mergingOnsets[p] = OnsetType::Pitch;
        }
        for (auto p : m_levelRiseOnsets) {
            mergingOnsets[p] = OnsetType::SpectralLevelRise;
        }
        for (auto p : m_powerRiseOnsets) {
            mergingOnsets[p] = OnsetType::PowerRise;
        }

        int prevP = -minimumOnsetSteps;
        OnsetType prevType = OnsetType::Pitch;
        
        for (auto pq : mergingOnsets) {
            int p = pq.first;
            auto type = pq.second;
            
            if (p < prevP + minimumOnsetSteps) {

                if (prevType == OnsetType::PowerRise &&
                    type != OnsetType::PowerRise) {
                    // "If a spectral rise onset follows [use minimum
                    // onset interval, i.e. 100 ms] after a raw power
                    // onset, return spectral rise onset only",
                    // i.e. erase the raw power onset we previously
                    // added. (But the motivating example for this
                    // actually has a pitch onset following the raw
                    // power one, not a spectral rise, so we test
                    // above for anything other than raw power)
                    m_mergedOnsets.erase(prevP);

                } else {
                    // An onset follows another one within the minimum
                    // onset interval and the above rule doesn't
                    // apply, so we don't insert this one, and also
                    // don't update prevP and prevType because we want
                    // it to have no effect on any following onsets
                    continue;
                }
            }

            m_mergedOnsets[p] = type;

            prevP = p;
            prevType = type;
        }

        n = m_rawPower.size();

        int sustainBeginSteps = msToSteps(m_parameters.sustainBeginThreshold_ms,
                                          m_parameters.stepSize, false);

        for (auto i = m_mergedOnsets.begin(); i != m_mergedOnsets.end(); ++i) {
            int p = i->first;
            int limit = n;
            auto j = i;
            if (++j != m_mergedOnsets.end()) {
                limit = j->first; // stop at the next onset
            }
            int s = p + sustainBeginSteps;
            if (s < n) {
                /*
                std::cerr << "power " << m_rawPower[s] << ", threshold "
                          << m_parameters.noteDurationThreshold_dB
                          << ", gives target power "
                          << m_rawPower[s] - m_parameters.noteDurationThreshold_dB
                          << std::endl;
                */
            } else {
                std::cerr << "sustain start index " << s
                          << " out of range at end" << std::endl;
            }
            int q = s;
            while (q < limit) {
                if (m_rawPower[q] <
                    m_rawPower[s] - m_parameters.noteDurationThreshold_dB) {
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

    std::vector<double>
    getOnsetLevelRiseFractionsFirstHalves() const {
        assertFinished();
        return m_onsetLevelRise.getFractions
            (SpectralLevelRise::FractionType::FirstHalf);
    }

    std::vector<double>
    getOnsetLevelRiseFractionsSecondHalves() const {
        assertFinished();
        return m_onsetLevelRise.getFractions
            (SpectralLevelRise::FractionType::SecondHalf);
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
    bool m_initialised;
    bool m_finished;
    Parameters m_parameters;

    PYinVamp m_pyin; // Not copyable by value, though, so we can't
                     // have default operator= etc
    Power m_power;
    SpectralLevelRise m_onsetLevelRise;

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
    std::map<int, OnsetType> m_mergedOnsets;
    std::map<int, int> m_onsetOffsets;

    void assertFinished() const {
        if (!m_finished) {
            throw std::logic_error("Features: feature retrieval attempted before finish() called");
        }
    }
};

#endif
