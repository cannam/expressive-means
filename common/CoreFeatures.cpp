
/*
    Expressive Means

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "CoreFeatures.h"

static const CoreFeatures::Parameters defaultCoreParams;

void
CoreFeatures::Parameters::appendVampParameterDescriptors(Vamp::Plugin::ParameterList &list)
{
    PYinVamp tempPYin(48000.f);
    auto pyinParams = tempPYin.getParameterDescriptors();
    for (auto d: pyinParams) {
        if (d.identifier == "threshdistr" ||
            d.identifier == "lowampsuppression") {
            d.identifier = "pyin-" + d.identifier;
            d.name = "pYIN: " + d.name;
            list.push_back(d);
        }
    }
    
    Vamp::Plugin::ParameterDescriptor d;

    d.description = "";
    d.isQuantized = false;
    
    d.identifier = "pitchAverageWindow";
    d.name = "Moving pitch average window";
    d.unit = "ms";
    d.minValue = 20.f;
    d.maxValue = 1000.f;
    d.defaultValue = defaultCoreParams.pitchAverageWindow_ms;
    list.push_back(d);

    d.identifier = "onsetSensitivityPitch";
    d.name = "Onset sensitivity: Pitch";
    d.unit = "cents";
    d.minValue = 0.f;
    d.maxValue = 100.f;
    d.defaultValue = defaultCoreParams.onsetSensitivityPitch_cents;
    list.push_back(d);
    
    d.identifier = "onsetSensitivityNoise";
    d.name = "Onset sensitivity: Noise";
    d.unit = "%";
    d.minValue = 0.f;
    d.maxValue = 100.f;
    d.defaultValue = defaultCoreParams.onsetSensitivityNoise_percent;
    list.push_back(d);
    
    d.identifier = "onsetSensitivityLevel";
    d.name = "Onset sensitivity: Level";
    d.unit = "dB";
    d.minValue = 0.f;
    d.maxValue = 100.f;
    d.defaultValue = defaultCoreParams.onsetSensitivityLevel_dB;
    list.push_back(d);
    
    d.identifier = "onsetSensitivityNoiseTimeWindow";
    d.name = "Onset sensitivity: Noise time window";
    d.unit = "ms";
    d.minValue = 20.f;
    d.maxValue = 500.f;
    d.defaultValue = defaultCoreParams.onsetSensitivityNoiseTimeWindow_ms;
    list.push_back(d);
    
    d.identifier = "onsetSensitivityRawPowerThreshold";
    d.name = "Onset sensitivity: Raw power threshold";
    d.unit = "dB";
    d.minValue = 0.f;
    d.maxValue = 100.f;
    d.defaultValue = defaultCoreParams.onsetSensitivityRawPowerThreshold_dB;
    list.push_back(d);
    
    d.identifier = "minimumOnsetInterval";
    d.name = "Minimum onset interval";
    d.unit = "ms";
    d.minValue = 0.f;
    d.maxValue = 1000.f;
    d.defaultValue = defaultCoreParams.minimumOnsetInterval_ms;
    list.push_back(d);
    
    d.identifier = "sustainBeginThreshold";
    d.name = "Sustain phase begin threshold";
    d.unit = "ms";
    d.minValue = 0.f;
    d.maxValue = 1000.f;
    d.defaultValue = defaultCoreParams.sustainBeginThreshold_ms;
    list.push_back(d);
    
    d.identifier = "noteDurationThreshold";
    d.name = "Note duration level drop threshold";
    d.unit = "dB";
    d.minValue = 0.f;
    d.maxValue = 100.f;
    d.defaultValue = defaultCoreParams.noteDurationThreshold_dB;
    list.push_back(d);
}

bool
CoreFeatures::Parameters::obtainVampParameter(std::string identifier, float &value) const
{
    if (identifier == "pyin-threshdistr") {
        value = pyinThresholdDistribution;
    } else if (identifier == "pyin-lowampsuppression") {
        value = pyinLowAmpSuppressionThreshold;
    } else if (identifier == "pitchAverageWindow") {
        value = pitchAverageWindow_ms;
    } else if (identifier == "onsetSensitivityPitch") {
        value = onsetSensitivityPitch_cents;
    } else if (identifier == "onsetSensitivityNoise") {
        value = onsetSensitivityNoise_percent;
    } else if (identifier == "onsetSensitivityLevel") {
        value = onsetSensitivityLevel_dB;
    } else if (identifier == "onsetSensitivityNoiseTimeWindow") {
        value = onsetSensitivityNoiseTimeWindow_ms;
    } else if (identifier == "onsetSensitivityRawPowerThreshold") {
        value = onsetSensitivityRawPowerThreshold_dB;
    } else if (identifier == "minimumOnsetInterval") {
        value = minimumOnsetInterval_ms;
    } else if (identifier == "sustainBeginThreshold") {
        value = sustainBeginThreshold_ms;
    } else if (identifier == "noteDurationThreshold") {
        value = noteDurationThreshold_dB;
    } else {
        return false;
    }
    return true;
}

bool
CoreFeatures::Parameters::acceptVampParameter(std::string identifier, float value)
{
    if (identifier == "pyin-threshdistr") {
        pyinThresholdDistribution = value;
    } else if (identifier == "pyin-lowampsuppression") {
        pyinLowAmpSuppressionThreshold = value;
    } else if (identifier == "pitchAverageWindow") {
        pitchAverageWindow_ms = value;
    } else if (identifier == "onsetSensitivityPitch") {
        onsetSensitivityPitch_cents = value;
    } else if (identifier == "onsetSensitivityNoise") {
        onsetSensitivityNoise_percent = value;
    } else if (identifier == "onsetSensitivityLevel") {
        onsetSensitivityLevel_dB = value;
    } else if (identifier == "onsetSensitivityNoiseTimeWindow") {
        onsetSensitivityNoiseTimeWindow_ms = value;
    } else if (identifier == "onsetSensitivityRawPowerThreshold") {
        onsetSensitivityRawPowerThreshold_dB = value;
    } else if (identifier == "minimumOnsetInterval") {
        minimumOnsetInterval_ms = value;
    } else if (identifier == "sustainBeginThreshold") {
        sustainBeginThreshold_ms = value;
    } else if (identifier == "noteDurationThreshold") {
        noteDurationThreshold_dB = value;
    } else {
        return false;
    }
    return true;
}

CoreFeatures::CoreFeatures(double sampleRate) :
    m_sampleRate(sampleRate),
    m_initialised(false),
    m_finished(false),
    m_pyin(sampleRate)
{ }

void
CoreFeatures::initialise(Parameters parameters) {
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
    levelRiseParameters.rise_dB = m_parameters.onsetSensitivityLevel_dB;
    levelRiseParameters.historyLength =
        msToSteps(m_parameters.onsetSensitivityNoiseTimeWindow_ms,
                  m_parameters.stepSize, false);
    if (levelRiseParameters.historyLength < 2) {
        levelRiseParameters.historyLength = 2;
    }
    m_onsetLevelRise.initialise(levelRiseParameters);

    m_initialised = true;
};

void
CoreFeatures::reset()
{
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

void
CoreFeatures::process(const float *input, Vamp::RealTime timestamp) {
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
CoreFeatures::finish()
{
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

