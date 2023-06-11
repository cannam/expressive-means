
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

using std::string;
using std::logic_error;
using std::vector;
using std::map;
using std::set;
using std::cerr;
using std::endl;

//#define DEBUG_CORE_FEATURES 1

void
CoreFeatures::Parameters::appendVampParameterDescriptors(Vamp::Plugin::ParameterList &list,
                                                         bool includeOffsetParameters)
{
    Vamp::Plugin::ParameterDescriptor d;
    
    d.identifier = "normaliseAudio";
    d.name = "Normalise audio";
    d.unit = "";
    d.description = "Normalise the audio signal to peak 1.0 before further processing. Requires that signal be short enough to fit in memory.";
    d.minValue = 0.f;
    d.maxValue = 1.f;
    d.isQuantized = true;
    d.quantizeStep = 1.f;
    d.defaultValue = defaultCoreParams.normalise;
    list.push_back(d);

    PYinVamp tempPYin(48000.f);
    auto pyinParams = tempPYin.getParameterDescriptors();
    for (auto pd: pyinParams) {
        if (pd.identifier == "threshdistr" ||
            pd.identifier == "lowampsuppression") {
            pd.identifier = "pyin-" + pd.identifier;
            pd.name = "pYIN: " + pd.name;
            list.push_back(pd);
        }
    }

    d.description = "";
    d.isQuantized = false;
    d.quantizeStep = 0.f;
    
    d.identifier = "spectralFrequencyMin";
    d.name = "Spectral detection range minimum frequency";
    d.unit = "Hz";
    d.minValue = 0.f;
    d.maxValue = 20000.f;
    d.defaultValue = defaultCoreParams.spectralFrequencyMin_Hz;
    list.push_back(d);
    
    d.identifier = "spectralFrequencyMax";
    d.name = "Spectral detection range maximum frequency";
    d.unit = "Hz";
    d.minValue = 0.f;
    d.maxValue = 20000.f;
    d.defaultValue = defaultCoreParams.spectralFrequencyMax_Hz;
    list.push_back(d);
    
    d.identifier = "minimumOnsetInterval";
    d.name = "Minimum onset interval";
    d.unit = "ms";
    d.minValue = 0.f;
    d.maxValue = 1000.f;
    d.defaultValue = defaultCoreParams.minimumOnsetInterval_ms;
    list.push_back(d);
    
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
    d.maxValue = 500.f;
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
    d.name = "Onset sensitivity: Power rise threshold";
    d.unit = "dB";
    d.minValue = 0.f;
    d.maxValue = 100.f;
    d.defaultValue = defaultCoreParams.onsetSensitivityRawPowerThreshold_dB;
    list.push_back(d);

    if (includeOffsetParameters) {
    
        d.identifier = "sustainBeginThreshold";
        d.name = "Sustain phase begin threshold";
        d.unit = "ms";
        d.minValue = 0.f;
        d.maxValue = 1000.f;
        d.defaultValue = defaultCoreParams.sustainBeginThreshold_ms;
        list.push_back(d);
    
        d.identifier = "noteDurationThreshold";
        d.name = "Offset sensitivity: Power drop threshold";
        d.unit = "dB";
        d.minValue = 0.f;
        d.maxValue = 100.f;
        d.defaultValue = defaultCoreParams.noteDurationThreshold_dB;
        list.push_back(d);
    
        d.identifier = "spectralDropFloor";
        d.name = "Offset sensitivity: Spectral drop floor level";
        d.unit = "dB";
        d.minValue = -120.f;
        d.maxValue = 0.f;
        d.defaultValue = defaultCoreParams.spectralDropFloor_dB;
        list.push_back(d);
    }
}

bool
CoreFeatures::Parameters::obtainVampParameter(string identifier, float &value) const
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
    } else if (identifier == "spectralDropFloor") {
        value = spectralDropFloor_dB;
    } else if (identifier == "spectralFrequencyMin") {
        value = spectralFrequencyMin_Hz;
    } else if (identifier == "spectralFrequencyMax") {
        value = spectralFrequencyMax_Hz;
    } else if (identifier == "normaliseAudio") {
        value = (normalise ? 1.f : 0.f);
    } else {
        return false;
    }
    return true;
}

bool
CoreFeatures::Parameters::acceptVampParameter(string identifier, float value)
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
    } else if (identifier == "spectralDropFloor") {
        spectralDropFloor_dB = value;
    } else if (identifier == "spectralFrequencyMin") {
        spectralFrequencyMin_Hz = value;
    } else if (identifier == "spectralFrequencyMax") {
        spectralFrequencyMax_Hz = value;
    } else if (identifier == "normaliseAudio") {
        normalise = (value > 0.5f);
    } else {
        return false;
    }
    return true;
}

CoreFeatures::CoreFeatures(double sampleRate) :
    m_sampleRate(sampleRate),
    m_initialised(false),
    m_finished(false),
    m_haveStartTime(false),
    m_pyin(sampleRate),
    m_normalisationGain(1.f)
{ }

void
CoreFeatures::initialise(Parameters parameters) {
    if (m_initialised) {
        throw logic_error("CoreFeatures::initialise: Already initialised");
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
        throw logic_error("pYIN smoothed pitch track output not found");
    }
        
    m_pyin.setParameter("outputunvoiced", 2.f); // As negative frequencies
    m_pyin.setParameter("precisetime", 1.f); // Match timing with other features
                                             // (see notes in finish() below)
    
    m_pyin.setParameter("threshdistr",
                        m_parameters.pyinThresholdDistribution);
    m_pyin.setParameter("lowampsuppression",
                        m_parameters.pyinLowAmpSuppressionThreshold);

    if (!m_pyin.initialise(1, m_parameters.stepSize, m_parameters.blockSize)) {
        throw logic_error("pYIN initialisation failed");
    }

    Power::Parameters powerParameters;
    powerParameters.blockSize = m_parameters.blockSize;
    m_power.initialise(powerParameters);

    SpectralLevelRise::Parameters levelRiseParameters;
    levelRiseParameters.sampleRate = m_sampleRate;
    levelRiseParameters.blockSize = m_parameters.blockSize;
    levelRiseParameters.rise_dB = m_parameters.onsetSensitivityLevel_dB;
    levelRiseParameters.floor_dB = m_parameters.spectralDropFloor_dB;
    levelRiseParameters.frequencyMin_Hz = m_parameters.spectralFrequencyMin_Hz;
    levelRiseParameters.frequencyMax_Hz = m_parameters.spectralFrequencyMax_Hz;
    levelRiseParameters.historyLength =
        msToSteps(m_parameters.onsetSensitivityNoiseTimeWindow_ms,
                  m_parameters.stepSize, false);
    if (levelRiseParameters.historyLength < 2) {
        levelRiseParameters.historyLength = 2;
    }
    m_onsetLevelRise.initialise(levelRiseParameters);

    m_haveStartTime = false;

    m_initialised = true;
};

void
CoreFeatures::reset()
{
    if (!m_initialised) {
        throw logic_error("CoreFeatures::reset: Never initialised");
    }
    m_finished = false;

    m_pyin.reset();
    m_power.reset();
    m_onsetLevelRise.reset();

    m_pyinPitchHz.clear();
    m_pitch.clear();
    m_filteredPitch.clear();
    m_pitchOnsetDf.clear();
    m_pitchOnsetDfValidity.clear();
    m_rawPower.clear();
    m_smoothedPower.clear();
    m_pitchOnsets.clear();
    m_levelRiseOnsets.clear();
    m_powerRiseOnsets.clear();
    m_mergedOnsets.clear();
    m_onsetOffsets.clear();
    m_normalisationGain = 1.f;

    m_haveStartTime = false;
}

void
CoreFeatures::process(const float *input, Vamp::RealTime timestamp)
{
    if (!m_initialised) {
        throw logic_error("CoreFeatures::process: Not initialised");
    }
    if (m_finished) {
        throw logic_error("CoreFeatures::process: Already finished");
    }

    if (!m_haveStartTime) {
        m_startTime = timestamp;
        m_haveStartTime = true;
    }

    if (!m_parameters.normalise) {
        actualProcess(input, timestamp);
    } else {
        vector<float> buf(input, input + m_parameters.blockSize);
        m_pending.push_back({ buf, timestamp });
    }
}

void
CoreFeatures::actualProcess(const float *input, Vamp::RealTime timestamp)
{
    const float *const *iptr = &input;
    auto pyinFeatures = m_pyin.process(iptr, timestamp);
    for (const auto &f: pyinFeatures[m_pyinSmoothedPitchTrackOutput]) {
        m_pyinPitchHz.push_back(f.values[0]);
    }

    m_power.process(input);
    m_onsetLevelRise.process(input);
}

void
CoreFeatures::finish()
{
    if (m_finished) {
        throw logic_error("CoreFeatures::finish: Already finished");
    }

    if (m_parameters.normalise) {
        float max = 0.f;
        for (const auto &p: m_pending) {
            for (float f: p.first) {
                float m = fabsf(f);
                if (m > max) {
                    max = m;
                }
            }
        }
        m_normalisationGain = 1.f / max;
#ifdef DEBUG_CORE_FEATURES
        cerr << "CoreFeatures::finish: signal max = " << max
             << ", normalisation gain = " << m_normalisationGain << endl;
#endif
        for (auto &p: m_pending) {
            auto v = p.first;
            for (int i = 0; i < int(v.size()); ++i) {
                v[i] *= m_normalisationGain;
            }
            actualProcess(v.data(), p.second);
        }
        m_pending.clear();
    }
    
    actualFinish();
}

void
CoreFeatures::actualFinish()
{
    // It's important to make sure the timings align for the values
    // returned by the various feature extractors.  They have the
    // following characteristics:
    //
    // - pYIN pitch track frames have timings offset by blockSize/4 in
    //   "imprecise"/"fast" mode or blockSize/2 in "precise" mode
    //
    // - Power frames have timings offset by blockSize/2
    //
    // - SpectralLevelRise starts recording values once its history
    //   buffer (say length n) is full. So the value at i indicates
    //   the fraction of bins that saw a significant rise during the n
    //   steps starting at input step i. Step i is calculated from
    //   time-domain samples between sample i*stepSize and i*stepSize
    //   + blockSize, so it reflects activity around the centre of the
    //   window at i*stepSize + blockSize/2
    //
    // So if we run pYIN in "precise" mode, the above are all aligned
    // with one another, and result i corresponds to time i*stepSize +
    // blockSize/2.
    //
    // Following this logic, we make no adjustments to the hop values
    // in any of this code because they all align with one another,
    // but we implement the blockSize/2 offset in our timeForStep()
    // method and expect it to be used whenever anything wants to map
    // from a hop number to a returned timestamp.
    
    auto pyinFeatures = m_pyin.getRemainingFeatures();
    for (const auto &f: pyinFeatures[m_pyinSmoothedPitchTrackOutput]) {
        m_pyinPitchHz.push_back(f.values[0]);
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
            (fabs(m_pitch[i] - m_filteredPitch[i + halfLength]));
    }

    // We need to reject cases in which the pitch onset df is small
    // because many pitches in the filter region are absent 
   int lastAbsence = -halfLength;
    for (int i = 0; i + halfLength < n; ++i) {
        bool valid = false;
        if (m_pyinPitchHz[i + halfLength] <= 0.0) {
            lastAbsence = i;
        } else {
            valid = (i - lastAbsence > halfLength);
        }
        m_pitchOnsetDfValidity.push_back(valid);
    }

    int minimumOnsetSteps = msToSteps(m_parameters.minimumOnsetInterval_ms,
                                      m_parameters.stepSize, false);

    // "subsequent onsets require o_2 to be exceeded for at least the
    // duration of o_6 [minimumOnsetSteps] first, but not exceeding
    // 120ms" (the last clause added in issue #11). The purpose is
    // just to avoid vibratos triggering pitch-based onsets. Calculate
    // that threshold now
    int vibratoSuppressionThresholdSteps =
        std::min(minimumOnsetSteps,
                 msToSteps(120.0, m_parameters.stepSize, false));

    int lastBelowThreshold = -vibratoSuppressionThresholdSteps;
    double threshold = m_parameters.onsetSensitivityPitch_cents / 100.0;
    
    for (int i = 0; i + halfLength < n; ++i) {
        // "absolute difference... falls below o_2":
        if (m_pitchOnsetDf[i] < threshold && m_pitchOnsetDfValidity[i]) {
            if (i > lastBelowThreshold + vibratoSuppressionThresholdSteps) {
                m_pitchOnsets.insert(i);
            }
            lastBelowThreshold = i;
        }
    }
    
    vector<double> riseFractions = m_onsetLevelRise.getFractions();
    double upperThreshold = m_parameters.onsetSensitivityNoise_percent / 100.0;
    double lowerThreshold = upperThreshold / 2.0;
    bool aboveThreshold = false;
    for (int i = 0; i < int(riseFractions.size()); ++i) {
        // Watch for the level to rise above threshold, then wait
        // for it to fall again and identify that moment as the
        // onset.
        if (riseFractions[i] > upperThreshold) {
            aboveThreshold = true;
        } else if (riseFractions[i] < lowerThreshold) {
            if (aboveThreshold) {
                m_levelRiseOnsets.insert(i);
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
                m_powerRiseOnsets.insert(i);
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

    map<int, OnsetType> mergingOnsets;
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

            // We have the following hierarchy for onsets, from most
            // to least precise:
            // 
            // 1. Spectral Rise
            // 2. Pitch Change
            // 3. Power Rise
            // 
            // If a higher-ranked (i.e. smaller numbered, above) type
            // of onset follows within the minimum onset interval
            // after a lower-ranked one, we drop the lower-ranked one
            // and keep the later higher-ranked one only.

            bool isHigherRanked =
                (prevType == OnsetType::PowerRise &&
                 type != OnsetType::PowerRise) ||
                (prevType != OnsetType::SpectralLevelRise &&
                 type == OnsetType::SpectralLevelRise);
            
            if (isHigherRanked) {
                m_mergedOnsets.erase(prevP);
            } else {
                // This onset follows another one within the minimum
                // onset interval, but it is of a lower-ranked type,
                // so we don't insert this one, and also don't update
                // prevP and prevType because we want it to have no
                // effect on any following onsets
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

    map<int, double> offsetDropDfEntries;
    
    for (auto i = m_mergedOnsets.begin(); i != m_mergedOnsets.end(); ++i) {
        int p = i->first;
        int limit = n;
        auto j = i;
        if (++j != m_mergedOnsets.end()) {
            limit = j->first; // stop at the next onset
        }

        set<int> binsAtBegin;
        int nBinsAtBegin = 0;
        double powerDropTarget = -100.0;

        int s = p + sustainBeginSteps;

        if (s < n) {
            auto bins = m_onsetLevelRise.getBinsAboveFloorAt(s);
            binsAtBegin.insert(bins.begin(), bins.end());
            nBinsAtBegin = bins.size();
            
            powerDropTarget =
                m_rawPower[s] - m_parameters.noteDurationThreshold_dB;

#ifdef DEBUG_CORE_FEATURES
            cerr << "at sustain begin step " << s << " found power "
                 << m_rawPower[s] << ", threshold "
                 << m_parameters.noteDurationThreshold_dB
                 << " giving target power " << powerDropTarget
                 << "; we have " << binsAtBegin.size()
                 << " bins active" << endl;
#endif
            
        } else {
#ifdef DEBUG_CORE_FEATURES
            cerr << "sustain start index " << s
                 << " out of range at end" << endl;
#endif
        }
        
        int q = s;
        OffsetType type = OffsetType::FollowingOnsetReached;
        
        while (q < limit) {

            if (m_rawPower[q] < powerDropTarget) {

#ifdef DEBUG_CORE_FEATURES
                cerr << "at step " << q << " found power " << m_rawPower[q]
                     << " which falls below target power "
                     << powerDropTarget << endl;
#endif
                
                type = OffsetType::PowerDrop;
                break;

            } else if (nBinsAtBegin > 0) {

                auto binsHere = m_onsetLevelRise.getBinsAboveFloorAt(q);
                int remaining = 0;
                for (auto bin: binsHere) {
                    if (binsAtBegin.find(bin) != binsAtBegin.end()) {
                        ++remaining;
                    }
                }

                double df = double(remaining) / double(nBinsAtBegin);
                offsetDropDfEntries[q] = df;
                                                                    
#ifdef DEBUG_CORE_FEATURES
                cerr << "at step " << q << " we have " << binsHere.size()
                     << " of which " << remaining
                     << " remain from the sustain begin step, giving df value "
                     << df << endl;
#endif

                if (df <= 0.4) {
                    type = OffsetType::SpectralLevelDrop;
                    break;
                }
            }
                
            ++q;
        }
        if (q >= limit) {
            m_onsetOffsets[p] = { limit, type };
        } else {
            m_onsetOffsets[p] = { q, type };
        }
    }

    m_offsetDropDf = vector<double>(n, 1.0);
    for (auto e: offsetDropDfEntries) {
        m_offsetDropDf[e.first] = e.second;
    }
    
    m_finished = true;
}

