
/*
    Expressive Means Articulation

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Articulation.h"
#include "Glide.h"

#include "version.h"

#include <vector>
#include <set>
#include <sstream>

using std::cerr;
using std::endl;
using std::vector;
using std::set;
using std::map;
using std::ostringstream;

static const float default_volumeDevelopmentThreshold_dB = 2.f;
static const float default_scalingFactor = 15.5f;
static const float default_impulseNoiseRatioPlosive_percent =  26.f;
static const float default_impulseNoiseRatioFricative_percent = 13.f;
static const float default_reverbDurationFactor = 1.5f;
static const float default_overlapCompensationFactor = 1.6f;

// These are not exposed as plugin parameters, so they're fixed but
// can be tweaked here
static const float default_glideThresholdPitch_cents = 60.f;
static const float default_glideThresholdHopMinimum_cents = 10.f;
static const float default_glideThresholdHopMaximum_cents = 50.f;
static const float default_glideThresholdDuration_ms = 70.f;
static const float default_glideThresholdProximity_ms = 350.f;

Articulation::Articulation(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_stepSize(0),
    m_blockSize(0),
    m_coreFeatures(inputSampleRate),
    m_volumeDevelopmentThreshold_dB(default_volumeDevelopmentThreshold_dB),
    m_scalingFactor(default_scalingFactor),
    m_impulseNoiseRatioPlosive_percent(default_impulseNoiseRatioPlosive_percent),
    m_impulseNoiseRatioFricative_percent(default_impulseNoiseRatioFricative_percent),
    m_reverbDurationFactor(default_reverbDurationFactor),
    m_overlapCompensationFactor(default_overlapCompensationFactor),
    m_glideThresholdPitch_cents(default_glideThresholdPitch_cents),
    m_glideThresholdHopMinimum_cents(default_glideThresholdHopMinimum_cents),
    m_glideThresholdHopMaximum_cents(default_glideThresholdHopMaximum_cents),
    m_glideThresholdDuration_ms(default_glideThresholdDuration_ms),
    m_glideThresholdProximity_ms(default_glideThresholdProximity_ms),
    m_summaryOutput(-1),
    m_noiseTypeOutput(-1),
    m_volumeDevelopmentOutput(-1),
    m_articulationTypeOutput(-1),
    m_pitchTrackOutput(-1),
    m_articulationIndexOutput(-1)
{
}

Articulation::~Articulation()
{
}

string
Articulation::getIdentifier() const
{
    return TAGGED_ID("articulation");
}

string
Articulation::getName() const
{
    return TAGGED_NAME("Expressive Means (advanced): Articulation");
}

string
Articulation::getDescription() const
{
    return "identifies types and intensities of articulation in monophonic recordings (specified parameter settings)";
}

string
Articulation::getMaker() const
{
    return "Frithjof Vollmer and Chris Cannam";
}

int
Articulation::getPluginVersion() const
{
    return EXPRESSIVE_MEANS_PLUGIN_VERSION;
}

string
Articulation::getCopyright() const
{
    return "GPLv2";
}

Articulation::InputDomain
Articulation::getInputDomain() const
{
    return TimeDomain;
}

size_t
Articulation::getPreferredBlockSize() const
{
    return m_coreFeatures.getPreferredBlockSize();
}

size_t 
Articulation::getPreferredStepSize() const
{
    return m_coreFeatures.getPreferredStepSize();
}

size_t
Articulation::getMinChannelCount() const
{
    return 1;
}

size_t
Articulation::getMaxChannelCount() const
{
    return 1;
}

Articulation::ParameterList
Articulation::getParameterDescriptors() const
{
    ParameterList list;

    m_coreParams.appendVampParameterDescriptors(list, true);
    
    ParameterDescriptor d;

    d.description = "";
    d.isQuantized = false;
    
    d.identifier = "impulseNoiseRatioPlosive";
    d.name = "Impulse noise ratio: Plosive";
    d.unit = "%";
    d.minValue = 1.f;
    d.maxValue = 100.f;
    d.defaultValue = default_impulseNoiseRatioPlosive_percent;
    list.push_back(d);
    
    d.identifier = "impulseNoiseRatioFricative";
    d.name = "Impulse noise ratio: Fricative";
    d.unit = "%";
    d.minValue = 1.f;
    d.maxValue = 100.f;
    d.defaultValue = default_impulseNoiseRatioFricative_percent;
    list.push_back(d);
    
    d.identifier = "volumeDevelopmentThreshold";
    d.name = "Volume development threshold";
    d.unit = "dB";
    d.minValue = 0.f;
    d.maxValue = 10.f;
    d.defaultValue = default_volumeDevelopmentThreshold_dB;
    list.push_back(d);
    
    d.identifier = "reverbDurationFactor";
    d.name = "Reverb duration factor";
    d.unit = "";
    d.minValue = 1.f;
    d.maxValue = 5.f;
    d.defaultValue = default_reverbDurationFactor;
    list.push_back(d);
    
    d.identifier = "overlapCompensationFactor";
    d.name = "Overlap compensation factor";
    d.unit = "";
    d.minValue = 1.f;
    d.maxValue = 3.f;
    d.defaultValue = default_overlapCompensationFactor;
    list.push_back(d);
    
    d.identifier = "scalingFactor";
    d.name = "Index scaling factor";
    d.unit = "";
    d.minValue = 0.f;
    d.maxValue = 50.f;
    d.defaultValue = default_scalingFactor;
    list.push_back(d);
    
    return list;
}

float
Articulation::getParameter(string identifier) const
{
    float value = 0.f;
    if (m_coreParams.obtainVampParameter(identifier, value)) {
        return value;
    }
    
    if (identifier == "volumeDevelopmentThreshold") {
        return m_volumeDevelopmentThreshold_dB;
    } else if (identifier == "scalingFactor") {
        return m_scalingFactor;
    } else if (identifier == "impulseNoiseRatioPlosive") {
        return m_impulseNoiseRatioPlosive_percent;
    } else if (identifier == "impulseNoiseRatioFricative") {
        return m_impulseNoiseRatioFricative_percent;
    } else if (identifier == "reverbDurationFactor") {
        return m_reverbDurationFactor;
    } else if (identifier == "overlapCompensationFactor") {
        return m_overlapCompensationFactor;
    }
    
    return 0.f;
}

void
Articulation::setParameter(string identifier, float value) 
{
    if (m_coreParams.acceptVampParameter(identifier, value)) {
        return;
    }

    if (identifier == "volumeDevelopmentThreshold") {
        m_volumeDevelopmentThreshold_dB = value;
    } else if (identifier == "scalingFactor") {
        m_scalingFactor = value;
    } else if (identifier == "impulseNoiseRatioPlosive") {
        m_impulseNoiseRatioPlosive_percent = value;
    } else if (identifier == "impulseNoiseRatioFricative") {
        m_impulseNoiseRatioFricative_percent = value;
    } else if (identifier == "reverbDurationFactor") {
        m_reverbDurationFactor = value;
    } else if (identifier == "overlapCompensationFactor") {
        m_overlapCompensationFactor = value;
    }
}

Articulation::ProgramList
Articulation::getPrograms() const
{
    ProgramList list;
    return list;
}

string
Articulation::getCurrentProgram() const
{
    return ""; 
}

void
Articulation::selectProgram(string)
{
}

Articulation::OutputList
Articulation::getOutputDescriptors() const
{
    OutputList list;
    OutputDescriptor d;

    // Common to all
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = (m_inputSampleRate / m_stepSize);
    
    d.identifier = "summary";
    d.name = "Summary";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 0;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_summaryOutput = int(list.size());
    list.push_back(d);
    
    d.identifier = "noiseType";
    d.name = "Noise Type";
    d.description = "Coding of transient noise for each onset. Values are 1 = Sonorous, 2 = Fricative, 3 = Plosive, 4 = Affricative";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_noiseTypeOutput = int(list.size());
    list.push_back(d);
    
    d.identifier = "volumeDevelopment";
    d.name = "Volume Development";
    d.description = "Coding of volume development during the sustain phase. Time and duration indicate the sustain phase for each note; values are 0 = unclassifiable or other, 1 = Decreasing, 2 = De-and-Increasing, 3 = Constant, 4 = In-and-Decreasing, 5 = Increasing";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.hasDuration = true;
    m_volumeDevelopmentOutput = int(list.size());
    list.push_back(d);
    
    d.identifier = "articulationType";
    d.name = "Articulation Type";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 0;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_articulationTypeOutput = int(list.size());
    list.push_back(d);
    
    d.identifier = "pitchTrack";
    d.name = "Pitch Track";
    d.description = "The smoothed pitch track computed by pYIN.";
    d.unit = "Hz";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_pitchTrackOutput = int(list.size());
    list.push_back(d);
    
    d.identifier = "articulationIndex";
    d.name = "Articulation Index";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_articulationIndexOutput = int(list.size());
    list.push_back(d);

    d.identifier = "meanNoiseRatio";
    d.name = "Mean Noise Ratio";
    d.description = "Returns a single label containing the mean noise ratio value across all onsets";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 0;
    d.hasKnownExtents = false;
    d.hasDuration = true;
    m_meanNoiseRatioOutput = int(list.size());
    list.push_back(d);

    d.identifier = "meanDynamics";
    d.name = "Mean Dynamics";
    d.description = "Returns two labels containing the mean maximum and minimum dB developments of all sustain phases";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 0;
    d.hasKnownExtents = false;
    d.hasDuration = true;
    m_meanDynamicsOutput = int(list.size());
    list.push_back(d);

    d.identifier = "meanToneRatio";
    d.name = "Mean Tone Ratio";
    d.description = "Returns a single label containing the mean noise ratio value across all onsets";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 0;
    d.hasKnownExtents = false;
    d.hasDuration = true;
    m_meanToneRatioOutput = int(list.size());
    list.push_back(d);
    
    return list;
}

bool
Articulation::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    if (channels < getMinChannelCount() || channels > getMaxChannelCount()) {
        cerr << "ERROR: Articulation::initialise: unsupported channel count "
             << channels << endl;
        return false;
    }

    if (m_inputSampleRate < 8000.0) {
        cerr << "ERROR: Articulation::initialise: sample rate ("
             << m_inputSampleRate << ") is too low, it must be at least 8kHz"
             << endl;
        return false;
    }
    
    if (m_inputSampleRate > 192000.0) {
        cerr << "ERROR: Articulation::initialise: sample rate ("
             << m_inputSampleRate << ") is too high, maximum is 192kHz"
             << endl;
        return false;
    }
    
    if (stepSize > blockSize) {
        cerr << "ERROR: Articulation::initialise: step size (" << stepSize
             << ") may not exceed block size (" << blockSize << ")" << endl;
        return false;
    }

    if (m_summaryOutput < 0) {
        (void)getOutputDescriptors(); // initialise output indices
    }
    
    m_stepSize = stepSize;
    m_blockSize = blockSize;

    try {
        m_coreParams.stepSize = m_stepSize;
        m_coreParams.blockSize = m_blockSize;
        m_coreFeatures.initialise(m_coreParams);
    } catch (const std::logic_error &e) {
        cerr << "ERROR: Articulation::initialise: Feature extractor initialisation failed: " << e.what() << endl;
        return false;
    }
    
    return true;
}

void
Articulation::reset()
{
    m_coreFeatures.reset();
}

Articulation::FeatureSet
Articulation::process(const float *const *inputBuffers, Vamp::RealTime timestamp)
{
    m_coreFeatures.process(inputBuffers[0], timestamp);
    return {};
}

Articulation::LevelDevelopment
Articulation::classifyLevelDevelopment(double begin,
                                       double end,
                                       double max,
                                       double min,
                                       double threshold)
{
    // "maxima or minima are only relevant if they exceed the
    // threshold relative to both sustain phase begin level and offset
    // level"
    bool maxIsSignificant = (max > begin + threshold && max > end + threshold);
    bool minIsSignificant = (min < begin - threshold && min < end - threshold);

    if (!maxIsSignificant && !minIsSignificant) {
        if (end > begin + threshold) {
            return LevelDevelopment::Increasing;
        } else if (end < begin - threshold) {
            return LevelDevelopment::Decreasing;
        } else {
            return LevelDevelopment::Constant;
        }
    }

    if (maxIsSignificant && minIsSignificant) {
        // In-and-de-and-in-and-de-etc - but we don't have a code for these
        return LevelDevelopment::Other;
    }

    if (maxIsSignificant) {
        return LevelDevelopment::InAndDecreasing;
    } else {
        return LevelDevelopment::DeAndIncreasing;
    }
}

Articulation::NoiseRec
Articulation::classifyOnsetNoise(const vector<vector<int>> &activeBinsAfterOnset,
                                 int binCount,
                                 double plosiveRatio,
                                 double fricativeRatio,
                                 bool forceSonorous)
{
    NoiseRec rec;
    int n = activeBinsAfterOnset.size();
    if (n < 2) return rec;

    int maxConsecutiveHopsAboveP = 0;
    int currentHopsAboveP = 0;

    int maxConsecutiveHopsAboveF = 0;
    int currentHopsAboveF = 0;

    for (const auto &active: activeBinsAfterOnset) {
        double ratio = double(active.size()) / double(binCount);
        if (ratio > plosiveRatio) {
            if (++currentHopsAboveP > maxConsecutiveHopsAboveP) {
                maxConsecutiveHopsAboveP = currentHopsAboveP;
            }
        } else {
            currentHopsAboveP = 0;
        }
        if (ratio > fricativeRatio) {
            if (++currentHopsAboveF > maxConsecutiveHopsAboveF) {
                maxConsecutiveHopsAboveF = currentHopsAboveF;
            }
        } else {
            currentHopsAboveF = 0;
        }
    }

    // Just for informational purposes, this % value in the Summary
    // output now shows the relative duration of consecutive hops that
    // have at least ratio p of bins above the floor value. This would
    // need to exceed 50% for an onset to be classified as Affricative
    // or Plosive.
    rec.total = double(maxConsecutiveHopsAboveP) / double(n);

    if (forceSonorous) {

        // Special case where the caller has already found a glide
        // (but we are still called, so as to fill in the
        // informational total field above)
        rec.type = NoiseType::Sonorous;
    
    } else if (maxConsecutiveHopsAboveP >= n/2 && maxConsecutiveHopsAboveF >= n) {
    
        // at least [p] FFT bins within [o5]/2 (half the time span)
        // and at least [f] FFT windows within [o5] (full span)
        // following an onset exceed [L] = allocate type code "a"
        
        rec.type = NoiseType::Affricative;
        
    } else if (maxConsecutiveHopsAboveP >= n/2) {

        // at least [p] FFT bins within [o5]/2 (half span) following
        // an onset exceed [L] = allocate type code "p"

        rec.type = NoiseType::Plosive;

    } else if (maxConsecutiveHopsAboveF >= n) {
        
        // at least [f] FFT windows within [o5] (full span) following
        // an onset exceed [L] = allocate type code "f"

        rec.type = NoiseType::Fricative;

    } else {

        rec.type = NoiseType::Sonorous;
    }

    return rec;
}

Articulation::FeatureSet
Articulation::getRemainingFeatures()
{
    FeatureSet fs;

    m_coreFeatures.finish();

    const auto pyinPitch = m_coreFeatures.getPYinPitch_Hz();

    for (int i = 0; i < int(pyinPitch.size()); ++i) {
        if (pyinPitch[i] <= 0) continue;
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.timeForStep(i);
        f.values.push_back(pyinPitch[i]);
        fs[m_pitchTrackOutput].push_back(f);
    }

    auto onsetOffsets = m_coreFeatures.getOnsetOffsets();
    auto rawPower = m_coreFeatures.getRawPower_dB();
    auto smoothedPower = m_coreFeatures.getSmoothedPower_dB();

    const auto &analysisPower = smoothedPower;
    
    int n = rawPower.size();

    map<int, NoiseRec> onsetToNoise;

    auto noiseRatioFractions = m_coreFeatures.getOnsetLevelRiseFractions();
    int noiseWindowSteps = m_coreFeatures.msToSteps
        (m_coreParams.onsetSensitivityNoiseTimeWindow_ms, m_stepSize, false);

    double plosiveRatio =
        (m_impulseNoiseRatioPlosive_percent * m_reverbDurationFactor) / 100.0;
    double fricativeRatio =
        (m_impulseNoiseRatioFricative_percent * m_reverbDurationFactor) / 100.0;
    
    map<int, int> onsetToFollowingOnset;
    for (auto itr = onsetOffsets.begin(); itr != onsetOffsets.end(); ++itr) {
        int onset = itr->first;
        int offset = itr->second.first;
        int following = n;
        auto probe = itr;
        if (++probe != onsetOffsets.end()) {
            following = probe->first;
        }
        if (following > onset) {
            onsetToFollowingOnset[onset] = following;
        } else {
            onsetToFollowingOnset[onset] = offset;
        }
    }
            
    map<int, double> onsetToRelativeDuration;
    double meanRelativeDuration = 0.0;
    for (auto pq : onsetToFollowingOnset) {
        int onset = pq.first;
        int following = pq.second;
        int offset = onsetOffsets.at(pq.first).first;
        double relativeDuration =
            double(offset - onset) / double(following - onset);
        onsetToRelativeDuration[onset] = relativeDuration;
        meanRelativeDuration += relativeDuration;
    }
    if (!onsetToFollowingOnset.empty()) {
        meanRelativeDuration /= onsetToFollowingOnset.size();
    }

    Glide::Parameters glideParams;
    glideParams.durationThreshold_steps =
        m_coreFeatures.msToSteps(m_glideThresholdDuration_ms,
                                 m_coreParams.stepSize, false);
    glideParams.onsetProximityThreshold_steps =
        m_coreFeatures.msToSteps(m_glideThresholdProximity_ms,
                                 m_coreParams.stepSize, false);
    glideParams.minimumPitchThreshold_cents = m_glideThresholdPitch_cents;
    glideParams.minimumHopDifference_cents = m_glideThresholdHopMinimum_cents;
    glideParams.maximumHopDifference_cents = m_glideThresholdHopMaximum_cents;
    glideParams.medianFilterLength_steps =
        m_coreFeatures.msToSteps(m_coreParams.pitchAverageWindow_ms,
                                 m_coreParams.stepSize, true);
    glideParams.useSmoothing = false;

    Glide glide(glideParams);
    Glide::Extents glides = glide.extract_Hz(pyinPitch, onsetOffsets);
    
    int prevOnset = -1;
    double meanNoiseRatio = 0.0;
    for (auto pq: onsetOffsets) {
        int onset = pq.first;
        vector<vector<int>> binsAboveFloor;
        for (int i = 0; i < noiseWindowSteps; ++i) {
            if (i < n) {
                binsAboveFloor.push_back
                    (m_coreFeatures.getOnsetBinsAboveNoiseFloorAt(onset + i));
            }
        }
        bool lungoPrecedes = false;
        bool lungoAndGlide = false;
        if (prevOnset >= 0) {
            if (onsetToRelativeDuration.at(prevOnset) >= 0.95) {
#ifdef DEBUG_ARTICULATION
                cerr << "Onset " << onset << " has lungo at preceding onset "
                     << prevOnset << endl;
#endif
                lungoPrecedes = true;
                if (glides.find(onset) != glides.end() &&
                    glides.at(onset).start < onset) {
#ifdef DEBUG_ARTICULATION
                    cerr << "... and there is a glide from "
                         << glides.at(onset).start << " to "
                         << glides.at(onset).end << endl;
#endif
                    lungoAndGlide = true;
                }
            }
        }
        double effectiveFricativeRatio =
            (lungoPrecedes ?
             fricativeRatio * m_overlapCompensationFactor :
             fricativeRatio);
        NoiseRec rec = classifyOnsetNoise
            (binsAboveFloor, m_coreFeatures.getOnsetBinCount(),
             plosiveRatio, effectiveFricativeRatio, lungoAndGlide);
        onsetToNoise[onset] = rec;
        meanNoiseRatio += rec.total;
        prevOnset = onset;
    }
    if (!onsetOffsets.empty()) {
        meanNoiseRatio /= onsetOffsets.size();
    }
    
    int sustainBeginSteps = m_coreFeatures.msToSteps
        (m_coreParams.sustainBeginThreshold_ms, m_stepSize, false);
    int sustainEndSteps = m_coreFeatures.msToSteps
        (m_coreParams.minimumOnsetInterval_ms / 2.0, m_stepSize, false);

    struct LDRec {
        int sustainBegin;
        int sustainEnd;
        double minDiff;
        double maxDiff;
        LevelDevelopment development;
    };
    
    map<int, LDRec> onsetToLD;
    double meanMaxDiff = 0.0;
    double meanMinDiff = 0.0;
    for (auto pq: onsetOffsets) {
        int onset = pq.first;
        int sustainBegin = onset + sustainBeginSteps;
        int sustainEnd = pq.second.first - 1;
        int following = onsetToFollowingOnset.at(onset);
        if (following - sustainEndSteps > sustainBegin &&
            sustainEnd > following - sustainEndSteps) {
            // Volume development is considered until note offset, but
            // it stops at half the minimum onset interval before next
            // onset in any case
            sustainEnd = following - sustainEndSteps;
        }
        auto development = LevelDevelopment::Unclassifiable;
        double minDiff = 0.0, maxDiff = 0.0;
        if (sustainEnd - sustainBegin >= 2 &&
            sustainBegin < n &&
            sustainEnd < n) {
            double sbl = analysisPower[sustainBegin];
            double sel = analysisPower[sustainEnd];
            double min = 0.0, max = 0.0;
            // sustainEnd - sustainBegin is at least 2 (checked above)
            // so we always assign some level to min and max
            for (int i = 1; i < sustainEnd - sustainBegin; ++i) {
                int ix = sustainBegin + i;
                if (i == 1 || analysisPower[ix] > max) {
                    max = analysisPower[ix];
                }
                if (i == 1 || analysisPower[ix] < min) {
                    min = analysisPower[ix];
                }
            }
            development = classifyLevelDevelopment
                (sbl, sel, max, min, m_volumeDevelopmentThreshold_dB);
            minDiff = min - sbl;
            maxDiff = max - sbl;
        }
        LDRec rec;
        rec.development = development;
        rec.sustainBegin = sustainBegin;
        rec.sustainEnd = sustainEnd;
        rec.minDiff = minDiff;
        rec.maxDiff = maxDiff;
        if (rec.development == LevelDevelopment::Unclassifiable) {
            rec.sustainBegin = onset;
        }

        // We wanted to distinguish Unclassifiable from the rest of
        // the development types above, but we should now simplify our
        // reporting by filing Unclassifiable (very short sustain) as
        // Constant
        if (rec.development == LevelDevelopment::Unclassifiable) {
            rec.development = LevelDevelopment::Constant;
        }
        
        onsetToLD[onset] = rec;

        meanMaxDiff += rec.maxDiff;
        meanMinDiff += rec.minDiff;
    }
    if (!onsetOffsets.empty()) {
        meanMaxDiff /= onsetOffsets.size();
        meanMinDiff /= onsetOffsets.size();
    }

    for (auto pq : onsetToNoise) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.timeForStep(pq.first);
        f.hasDuration = false;
        f.values.push_back(static_cast<int>(pq.second.type) + 1);
        f.label = noiseTypeToString(pq.second.type);
        fs[m_noiseTypeOutput].push_back(f);
    }

    for (auto pq : onsetToLD) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.timeForStep(pq.second.sustainBegin);
        f.hasDuration = true;
        f.duration =
            m_coreFeatures.timeForStep(pq.second.sustainEnd + 1) - f.timestamp;
        auto development = pq.second.development;
        if (development == LevelDevelopment::Other) {
            f.values.push_back(0);
        } else {
            f.values.push_back(static_cast<int>(development));
        }
        f.label = developmentToString(development);
        fs[m_volumeDevelopmentOutput].push_back(f);
    }

    for (auto pq : onsetOffsets) {
        
        auto onset = pq.first;
        auto offset = pq.second.first;
        string code;
        double index = 1.0;

        NoiseType noise = onsetToNoise[onset].type;
        code += noiseTypeToCode(noise);
        index *= noiseTypeToFactor(noise);

        LevelDevelopment development = onsetToLD[onset].development;
        code += developmentToCode(development);
        index *= developmentToFactor(development);

        double relativeDuration = onsetToRelativeDuration[onset];
        if (relativeDuration < 0.6) {
            code += "S";
        } else if (relativeDuration < 0.95) {
            code += "E";
        } else {
            code += "L";
        }
        if (relativeDuration > 0.0) {
            index /= relativeDuration;
        }
        
        index *= m_scalingFactor;
        
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.timeForStep(onset);
        f.hasDuration = false;
        f.label = code;
        fs[m_articulationTypeOutput].push_back(f);

        f.label = "";
        f.values.clear();
        f.values.push_back(round(index));
        fs[m_articulationIndexOutput].push_back(f);

        double max2dp = round(onsetToLD.at(onset).maxDiff * 100.0) / 100.0;
        double min2dp = round(onsetToLD.at(onset).minDiff * 100.0) / 100.0;
        
        ostringstream os;
        os << m_coreFeatures.timeForStep(onset).toText() << " / "
           << (m_coreFeatures.timeForStep(onsetToFollowingOnset.at(onset)) -
               m_coreFeatures.timeForStep(onset)).toText() << "\n"
           << code << "\n"
           << int(round(onsetToNoise.at(onset).total * 100.0)) << "%\n"
           << max2dp << "dB / " << min2dp << "dB\n"
           << relativeDuration << " ("
           << (m_coreFeatures.timeForStep(offset) -
               m_coreFeatures.timeForStep(onset)).toText() << ")\n"
           << "IArt = " << round(index);
        f.label = os.str();
        f.values.clear();
        fs[m_summaryOutput].push_back(f);
    }

    Feature f;
    
    f.hasTimestamp = true;
    f.timestamp = m_coreFeatures.getStartTime();
    f.hasDuration = true;
    f.duration = m_coreFeatures.timeForStep(n) - f.timestamp;
    f.values.clear();
    {
        ostringstream os;
        os << (meanNoiseRatio * 100.0) << "%";
        f.label = os.str();
    }
    fs[m_meanNoiseRatioOutput].push_back(f);

    f.values.clear();
    {
        ostringstream os;
        os << meanMinDiff << "dB minimum";
        f.label = os.str();
    }
    fs[m_meanDynamicsOutput].push_back(f);

    f.values.clear();
    {
        ostringstream os;
        os << meanMaxDiff << "dB maximum";
        f.label = os.str();
    }
    fs[m_meanDynamicsOutput].push_back(f);

    f.values.clear();
    {
        ostringstream os;
        os << (meanRelativeDuration * 100.0) << "%";
        f.label = os.str();
    }
    fs[m_meanToneRatioOutput].push_back(f);
    
    return fs;
}

