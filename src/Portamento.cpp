
/*
    Expressive Means Portamento

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Portamento.h"

#include "../ext/qm-dsp/maths/MedianFilter.h"

#include <vector>
#include <set>

using std::cerr;
using std::endl;
using std::vector;
using std::set;
using std::map;
using std::lower_bound;
using std::ostringstream;

static float default_glideThresholdPitch_cents = 20.f;
static float default_glideThresholdDuration_ms = 30.f;
static float default_glideThresholdProximity_ms = 500.f;
static float default_linkThreshold_ms = 30.f;

Portamento::Portamento(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_stepSize(0),
    m_blockSize(0),
    m_coreFeatures(inputSampleRate),
    m_glideThresholdPitch_cents(default_glideThresholdPitch_cents),
    m_glideThresholdDuration_ms(default_glideThresholdDuration_ms),
    m_glideThresholdProximity_ms(default_glideThresholdProximity_ms),
    m_linkThreshold_ms(default_linkThreshold_ms),
    m_summaryOutput(-1),
    m_portamentoTypeOutput(-1),
    m_pitchTrackOutput(-1),
    m_portamentoIndexOutput(-1),
    m_pitchDiffOutput1(-1),
    m_pitchDiffOutput2(-1),
    m_candidateHopsOutput(-1),
    m_portamentoPointsOutput(-1),
    m_glideDirectionOutput(-1),
    m_glideLinkOutput(-1),
    m_glidePitchTrackOutput(-1)
{
}

Portamento::~Portamento()
{
}

string
Portamento::getIdentifier() const
{
    return "portamento";
}

string
Portamento::getName() const
{
    return "Expressive Means: Portamento";
}

string
Portamento::getDescription() const
{
    return "";
}

string
Portamento::getMaker() const
{
    return "Frithjof Vollmer and Chris Cannam";
}

int
Portamento::getPluginVersion() const
{
    return 1;
}

string
Portamento::getCopyright() const
{
    return "GPLv2";
}

Portamento::InputDomain
Portamento::getInputDomain() const
{
    return TimeDomain;
}

size_t
Portamento::getPreferredBlockSize() const
{
    return m_coreFeatures.getPreferredBlockSize();
}

size_t 
Portamento::getPreferredStepSize() const
{
    return m_coreFeatures.getPreferredStepSize();
}

size_t
Portamento::getMinChannelCount() const
{
    return 1;
}

size_t
Portamento::getMaxChannelCount() const
{
    return 1;
}

Portamento::ParameterList
Portamento::getParameterDescriptors() const
{
    ParameterList list;

    m_coreParams.appendVampParameterDescriptors(list);
    
    ParameterDescriptor d;

    d.description = "";
    d.isQuantized = false;
    
    d.identifier = "glideThresholdPitch";
    d.name = "Glide detection threshold: Pitch";
    d.unit = "cents";
    d.minValue = 0.f;
    d.maxValue = 100.f;
    d.defaultValue = default_glideThresholdPitch_cents;
    list.push_back(d);
    
    d.identifier = "glideThresholdDuration";
    d.name = "Glide detection threshold: Duration";
    d.unit = "ms";
    d.minValue = 0.f;
    d.maxValue = 200.f;
    d.defaultValue = default_glideThresholdDuration_ms;
    list.push_back(d);
    
    d.identifier = "glideThresholdProximity";
    d.name = "Glide detection threshold: Onset Proximity";
    d.unit = "ms";
    d.minValue = 0.f;
    d.maxValue = 2000.f;
    d.defaultValue = default_glideThresholdProximity_ms;
    list.push_back(d);
    
    d.identifier = "linkThreshold";
    d.name = "Link threshold";
    d.unit = "ms";
    d.minValue = 0.f;
    d.maxValue = 150.f;
    d.defaultValue = default_linkThreshold_ms;
    list.push_back(d);
    
    return list;
}

float
Portamento::getParameter(string identifier) const
{
    float value = 0.f;
    if (m_coreParams.obtainVampParameter(identifier, value)) {
        return value;
    }

    if (identifier == "glideThresholdPitch") {
        return m_glideThresholdPitch_cents;
    } else if (identifier == "glideThresholdDuration") {
        return m_glideThresholdDuration_ms;
    } else if (identifier == "glideThresholdProximity") {
        return m_glideThresholdProximity_ms;
    } else if (identifier == "linkThreshold") {
        return m_linkThreshold_ms;
    }
    
    return 0.f;
}

void
Portamento::setParameter(string identifier, float value) 
{
    if (m_coreParams.acceptVampParameter(identifier, value)) {
        return;
    }

    if (identifier == "glideThresholdPitch") {
        m_glideThresholdPitch_cents = value;
    } else if (identifier == "glideThresholdDuration") {
        m_glideThresholdDuration_ms = value;
    } else if (identifier == "glideThresholdProximity") {
        m_glideThresholdProximity_ms = value;
    } else if (identifier == "linkThreshold") {
        m_linkThreshold_ms = value;
    }
}

Portamento::ProgramList
Portamento::getPrograms() const
{
    ProgramList list;
    return list;
}

string
Portamento::getCurrentProgram() const
{
    return ""; 
}

void
Portamento::selectProgram(string)
{
}

Portamento::OutputList
Portamento::getOutputDescriptors() const
{
    OutputList list;
    OutputDescriptor d;

    // Common to (almost) all
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = (m_inputSampleRate / m_stepSize);
    
    d.identifier = "summary";
    d.name = "Summary";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_summaryOutput = int(list.size());
    list.push_back(d);
    
    d.identifier = "portamentoType";
    d.name = "Portamento Type";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 0;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_portamentoTypeOutput = int(list.size());
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
    
    d.identifier = "portamentoIndex";
    d.name = "Portamento Index";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_portamentoIndexOutput = int(list.size());
    list.push_back(d);

#ifdef WITH_DEBUG_OUTPUTS
    d.identifier = "pitchdiff1";
    d.name = "[Debug] Pitch Difference Function (Mean Filter)";
    d.description = "";
    d.unit = "semitones";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_pitchDiffOutput1 = int(list.size());
    list.push_back(d);

    d.identifier = "pitchdiff2";
    d.name = "[Debug] Pitch Difference Function (Median+)";
    d.description = "";
    d.unit = "semitones";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_pitchDiffOutput2 = int(list.size());
    list.push_back(d);

    d.identifier = "candidateHops";
    d.name = "[Debug] Candidate Hops";
    d.description = "";
    d.unit = "semitones";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_candidateHopsOutput = int(list.size());
    list.push_back(d);
    
    d.identifier = "portamentoPoints";
    d.name = "[Debug] Portamento Significant Points";
    d.description = "";
    d.unit = "Hz";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_portamentoPointsOutput = int(list.size());
    list.push_back(d);
    
    d.identifier = "glideDirection";
    d.name = "[Debug] Glide Direction";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 0;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_glideDirectionOutput = int(list.size());
    list.push_back(d);
    
    d.identifier = "glideLink";
    d.name = "[Debug] Glide Link";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 0;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_glideLinkOutput = int(list.size());
    list.push_back(d);
    
    d.identifier = "glidePitchTrack";
    d.name = "[Debug] Glide-Only Pitch Track";
    d.description = "";
    d.unit = "Hz";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_glidePitchTrackOutput = int(list.size());
    list.push_back(d);
#endif
    
    return list;
}

bool
Portamento::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    if (channels < getMinChannelCount() || channels > getMaxChannelCount()) {
        cerr << "ERROR: Portamento::initialise: unsupported channel count "
             << channels << endl;
        return false;
    }

    if (m_inputSampleRate < 8000.0) {
        cerr << "ERROR: Portamento::initialise: sample rate ("
             << m_inputSampleRate << ") is too low, it must be at least 8kHz"
             << endl;
        return false;
    }
    
    if (m_inputSampleRate > 192000.0) {
        cerr << "ERROR: Portamento::initialise: sample rate ("
             << m_inputSampleRate << ") is too high, maximum is 192kHz"
             << endl;
        return false;
    }
    
    if (stepSize > blockSize) {
        cerr << "ERROR: Portamento::initialise: step size (" << stepSize
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
        cerr << "ERROR: Portamento::initialise: Feature extractor initialisation failed: " << e.what() << endl;
        return false;
    }
    
    return true;
}

void
Portamento::reset()
{
    m_coreFeatures.reset();
}

Portamento::FeatureSet
Portamento::process(const float *const *inputBuffers, Vamp::RealTime timestamp)
{
    m_coreFeatures.process(inputBuffers[0], timestamp);
    return {};
}

Portamento::FeatureSet
Portamento::getRemainingFeatures_glideModel1()
{
    FeatureSet fs;

    m_coreFeatures.finish();

    auto pyinPitch = m_coreFeatures.getPYinPitch_Hz();

    for (int i = 0; i < int(pyinPitch.size()); ++i) {
        if (pyinPitch[i] <= 0) continue;
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.timeForStep(i);
        f.values.push_back(pyinPitch[i]);
        fs[m_pitchTrackOutput].push_back(f);
    }

    auto onsetOffsets = m_coreFeatures.getOnsetOffsets();
    auto pitch = m_coreFeatures.getPitch_semis();

    int n = pitch.size();
    
    int pitchFilterLength =
        m_coreFeatures.msToSteps(m_coreParams.pitchAverageWindow_ms,
                                 m_coreParams.stepSize, true);
    int halfLength = pitchFilterLength / 2;

    auto meanFilteredPitch = m_coreFeatures.getFilteredPitch_semis();
    
    // The filtered pitch from CoreFeatures is mean-filtered, we want median too
    vector<double> medianFilteredPitch =
        MedianFilter<double>::filter(pitchFilterLength, pitch);

    // And for comparison against it, use a much more modestly
    // mean-filtered version of the original pitch
    vector<double> slightlyFilteredPitch(n, 0.0);
    MeanFilter(5).filter
        (medianFilteredPitch.data(), slightlyFilteredPitch.data(), n);
    
    // "Glides are apparent if the absolute difference of a pitch [o]
    // and its following moving pitch average window [o1] exceeds [g1]
    // cents for at least [g2] ms and if there continuously is pitch
    // data within this time frame. Filter for glides that start
    // and/or end +/- [g3] ms around a note onset"

    vector<double> pitchDiff1;
    vector<double> pitchDiff2;
    vector<int> candidates; // hop
    map<int, int> glides; // glide start hop -> glide end hop

    int lastNonCandidate = -1;
    int thresholdSteps = m_coreFeatures.msToSteps(m_glideThresholdDuration_ms,
                                                  m_coreParams.stepSize, false);
    double threshold = m_glideThresholdPitch_cents / 100.0;
    double peakHold = 0.0;
    int peakHop = 0;
    bool isCandidate = false;
    
    for (int i = 0; i + halfLength < n; ++i) {

        pitchDiff1.push_back(fabs(pitch[i] - meanFilteredPitch[i + halfLength]));
        
        double diff = fabs(slightlyFilteredPitch[i] -
                           medianFilteredPitch[i + halfLength]);
        pitchDiff2.push_back(diff);

        bool aboveThreshold = (pyinPitch[i] > 0.0) && (diff >= threshold);

        if (!aboveThreshold) {
            isCandidate = false;
            peakHold = 0.0;
        } else {
            if (!isCandidate) {
                if (aboveThreshold && diff < peakHold * 0.95) {
                    isCandidate = true;
                    for (int j = peakHop; j < i; ++j) {
                        candidates.push_back(j);
                    }
                } else {
                    if (diff > peakHold) {
                        peakHold = diff;
                        peakHop = i;
                    }
                }
            }
        }
        
        cerr << "hop " << i << ": pitch = " << pitch[i] << ", filtered = "
             << medianFilteredPitch[i + halfLength] << ", diff = " << diff
             << ", peakHold = " << peakHold << ", peakHop = " << peakHop
             << ", threshold = " << threshold
             << ", isCandidate = " << isCandidate << endl;
        
        if (isCandidate) {
            candidates.push_back(i);
        } else {
            if (lastNonCandidate + thresholdSteps <= i) {
                glides[lastNonCandidate + 1] = i - 1;
            }
            lastNonCandidate = i;
        }
    }

    if (lastNonCandidate + thresholdSteps + halfLength <= n) {
        glides[lastNonCandidate + 1] = n - halfLength;
    }

    int proximitySteps = m_coreFeatures.msToSteps(m_glideThresholdProximity_ms,
                                                  m_coreParams.stepSize, false);

    map<int, pair<int, int>> onsetMappedGlides; // onset -> glide start, end
    
    for (auto g : glides) {

        int start = g.first;
        int end = g.second;

        int rangeStart = start - proximitySteps;
        int rangeEnd = end + proximitySteps;
        
        auto onsetItr = onsetOffsets.lower_bound(rangeStart);

        auto scout = onsetItr;
        bool found = false;
        
        while (scout != onsetOffsets.end()) {
            int onset = scout->first;
            if (onset >= start && onset <= end) {
                cerr << "for glide from " << start << " to " << end
                     << ", found onset within glide at " << onset << endl;
                onsetMappedGlides[onset] = g;
                found = true;
                break;
            }
            ++scout;
        }

        if (!found) {
            int minDist = proximitySteps + 1;
            int best = -1;
            scout = onsetItr;
            while (scout != onsetOffsets.end()) {
                int onset = scout->first;
                int dist = std::min(abs(start - onset), abs(end - onset));
                if (dist < minDist) {
                    minDist = dist;
                    best = onset;
                    found = true;
                }
                ++scout;
            }
            if (found) {
                cerr << "for glide from " << start << " to " << end
                     << ", found no onset within glide; using closest onset at "
                     << best << endl;
                onsetMappedGlides[best] = g;
            } else {
                cerr << "for glide from " << start << " to " << end
                     << ", found no onset within glide or within proximity "
                     << "range; ignoring this glide" << endl;
            }
        }
    }

    map<int, GlideDirection> directions; // onset -> direction
    map<int, GlideLink> links; // onset -> links
    int linkThresholdSteps = m_coreFeatures.msToSteps
        (m_linkThreshold_ms, m_coreParams.stepSize, false);
    
    for (auto m : onsetMappedGlides) {

        int onset = m.first;
        int glideStart = m.second.first;
        int glideEnd = m.second.second;

        if (pyinPitch[glideStart] < pyinPitch[glideEnd]) {
            directions[onset] = GlideDirection::Ascending;
        } else {
            directions[onset] = GlideDirection::Descending;
        }

        bool haveBefore = false, haveAfter = false;
        
        int dist = 1;
        for (int i = glideStart - 1; i >= 0; --i) {
            if (pyinPitch[i] > 0) {
                break;
            }
            ++dist;
        }
        if (dist < linkThresholdSteps) {
            haveBefore = true;
        }

        dist = 1;
        for (int i = glideEnd + 1; i < n; ++i) {
            if (pyinPitch[i] > 0) {
                break;
            }
            ++dist;
        }
        if (dist < linkThresholdSteps) {
            haveAfter = true;
        }

        if (haveBefore) {
            if (haveAfter) {
                links[onset] = GlideLink::Interconnecting;
            } else {
                links[onset] = GlideLink::Targeting;
            }
        } else {
            if (haveAfter) {
                links[onset] = GlideLink::Starting;
            } else {
                links[onset] = GlideLink::Interconnecting;
            }
        }
    }

#ifdef WITH_DEBUG_OUTPUTS
    for (size_t i = 0; i < pitchDiff1.size(); ++i) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.timeForStep(i);
        f.values.push_back(pitchDiff1[i]);
        fs[m_pitchDiffOutput1].push_back(f);
    }
    
    for (size_t i = 0; i < pitchDiff2.size(); ++i) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.timeForStep(i);
        f.values.push_back(pitchDiff2[i]);
        fs[m_pitchDiffOutput2].push_back(f);
    }
    
    for (size_t i = 0; i < candidates.size(); ++i) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.timeForStep(candidates[i]);
        f.values.push_back(pitchDiff2[candidates[i]]);
        fs[m_candidateHopsOutput].push_back(f);
    }

    int j = 1;
    for (auto m : onsetMappedGlides) {

        int onset = m.first;
        int glideStart = m.second.first;
        int glideEnd = m.second.second;

        cerr << "returning features for glide " << j
             << " associated with onset " << onset
             << " with glide range from " << glideStart << " to " << glideEnd
             << endl;
        
        Feature f;
        f.hasTimestamp = true;
        
        {
            ostringstream os;
            os << "Glide " << j << ": Start";
            f.timestamp = m_coreFeatures.timeForStep(glideStart);
            f.values.clear();
            f.values.push_back(pyinPitch[glideStart]);
            f.label = os.str();
            fs[m_portamentoPointsOutput].push_back(f);
        }

        {
            ostringstream os;
            os << "Glide " << j << ": Onset";
            f.timestamp = m_coreFeatures.timeForStep(onset);
            f.values.clear();
            f.values.push_back(pyinPitch[onset]);
            f.label = os.str();
            fs[m_portamentoPointsOutput].push_back(f);

#ifdef WITH_DEBUG_OUTPUTS
            f.values.clear();

            f.label = glideDirectionToString(directions[onset]);
            fs[m_glideDirectionOutput].push_back(f);

            f.label = glideLinkToString(links[onset]);
            fs[m_glideLinkOutput].push_back(f);
#endif
        }

        {
            ostringstream os;
            os << "Glide " << j << ": End";
            f.timestamp = m_coreFeatures.timeForStep(glideEnd);
            f.values.clear();
            f.values.push_back(pyinPitch[glideEnd]);
            f.label = os.str();
            fs[m_portamentoPointsOutput].push_back(f);
        }

        ++j;
    }
#endif
    
    return fs;
}

Portamento::FeatureSet
Portamento::getRemainingFeatures_glideModel2()
{
    FeatureSet fs;

    m_coreFeatures.finish();

    auto pyinPitch = m_coreFeatures.getPYinPitch_Hz();

    for (int i = 0; i < int(pyinPitch.size()); ++i) {
        if (pyinPitch[i] <= 0) continue;
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.timeForStep(i);
        f.values.push_back(pyinPitch[i]);
        fs[m_pitchTrackOutput].push_back(f);
    }

    auto onsetOffsets = m_coreFeatures.getOnsetOffsets();
    auto rawPitch = m_coreFeatures.getPitch_semis();
    
    int n = rawPitch.size();

    // Modestly mean-filtered pitch, just to take out jitter
    vector<double> pitch(n, 0.0);
    MeanFilter(5).filter(rawPitch.data(), pitch.data(), n);
    for (int i = 0; i < n; ++i) {
        if (pyinPitch[i] <= 0.0) {
            pitch[i] = 0.0;
        }
    }

    // "A glide is apparent as soon as the pitch starts to constantly
    // move forward in one direction for at least [threshold:
    // duration], but each hop's value not deviating more than
    // [threshold: pitch] cents from the hop value before".
    //
    // A couple of additions to this:
    //
    // * It's implicit that there must *be* pitch measurements
    // continuously available during this time
    //
    // * Although each hop should not deviate more than the given
    // threshold from the previous one, we do insist that the total
    // deviation through the whole glide is over this threshold, to
    // eliminate tiny drifts in pitch through a note

    vector<double> pitchDelta;
    vector<int> candidates; // hop
    map<int, int> glides; // glide start hop -> glide end hop

    int lastNonCandidate = -1;
    int thresholdSteps = m_coreFeatures.msToSteps(m_glideThresholdDuration_ms,
                                                  m_coreParams.stepSize, false);
    double threshold = m_glideThresholdPitch_cents / 100.0;
    double prevDelta = 0.0;
    
    for (int i = 1; i < n; ++i) {

        bool sameDirection = false;
        bool belowThreshold = false;
        bool havePitch = (pitch[i] > 0.0);

        if (havePitch) {
            if (pitch[i-1] > 0.0) {
                double delta = pitch[i] - pitch[i-1];
                belowThreshold = (fabs(delta) <= threshold);
                sameDirection = ((delta > 0.0 && prevDelta > 0.0) ||
                                 (delta < 0.0 && prevDelta < 0.0));
                pitchDelta.push_back(delta);
                prevDelta = delta;
            } else {
                pitchDelta.push_back(0.0);
                prevDelta = 0.0;
            }
        } else {
            pitchDelta.push_back(0.0);
            prevDelta = 0.0;
        }

        bool isCandidate = (havePitch && belowThreshold && sameDirection);

        if (isCandidate) {
            candidates.push_back(i);
        } else {
            // Not a candidate: If at least thresholdSteps candidates
            // in a row previously with total pitch drift more than
            // threshold, record a glide ending here
            if (lastNonCandidate + thresholdSteps <= i &&
                fabs(pitch[i-1] - pitch[lastNonCandidate + 1]) > threshold) {
                glides[lastNonCandidate + 1] = i-1;
            }
            lastNonCandidate = i;
        }
    }

    if (lastNonCandidate + thresholdSteps < n &&
        fabs(pitch[n-1] - pitch[lastNonCandidate + 1]) > threshold) {
        glides[lastNonCandidate + 1] = n-1;
    }

    int proximitySteps = m_coreFeatures.msToSteps(m_glideThresholdProximity_ms,
                                                  m_coreParams.stepSize, false);

    struct GlideProperties {
        int start;
        int end;
        bool provisional;
    };
    map<int, GlideProperties> onsetMappedGlides; // key is onset step
    
    for (auto g : glides) {

        // Each glide has a nearest onset (by some measure), and each
        // onset has zero or one nearest glides (by some measure).
        //
        // If there is an onset actually within the glide, we can
        // treat that as unambiguously the nearest onset for the glide
        // *and* the nearest glide for the onset (since glides don't
        // overlap one another).
        //
        // If there is no onset within the glide, then we can find a
        // closest onset e.g. by counting hops between the onset and
        // the start or end of the glide (whichever is closer to the
        // onset). But we may end up with more than one glide having
        // the same nearest onset - in a test case I'm finding one
        // onset that is the nearest onset for 7 (!) different glides,
        // all within our onset proximity threshold:
        //
        // for glide from 2044 to 2120, closest onset at 2130
        // for glide from 2139 to 2154, closest onset at 2130
        // for glide from 2156 to 2170, closest onset at 2130
        // for glide from 2172 to 2183, closest onset at 2130
        // for glide from 2185 to 2203, closest onset at 2130
        // for glide from 2205 to 2215, closest onset at 2130
        // for glide from 2217 to 2232, closest onset at 2130
        //
        // We need to pick one of these to associate with the
        // onset. The "correct" one is the first, the rest are all
        // just noise within or after the following note. But the
        // second one is closer than the first, so we shouldn't just
        // choose the closest - the fact that the first one is so much
        // longer has to be relevant.
        //
        // Since we consider glides in order of start time and they
        // can't overlap, let's use the following rules of thumb. For
        // each onset we will have either no "best" glide, a
        // provisional best, or a definitive best. For each glide:
        //
        // - Find the nearest onset for the glide, as above
        // 
        // - If it's within the glide, mark glide as definitive best
        //
        // - If it's after the glide, mark glide as provisional best
        //
        // - If it's before the glide and we don't have a glide
        //   already marked as best for this onset, mark this one best
        //
        // - Otherwise if the glide currently marked is 
        
        int start = g.first;
        int end = g.second;

        int rangeStart = start - proximitySteps;
        int rangeEnd = end + proximitySteps;
        
        auto onsetItr = onsetOffsets.lower_bound(rangeStart);

        auto scout = onsetItr;
        bool found = false;
        
        while (scout != onsetOffsets.end()) {
            int onset = scout->first;
            if (onset >= start && onset <= end) {
                cerr << "for glide from " << start << " to " << end
                     << ", found onset within glide at " << onset << endl;
                GlideProperties props;
                props.start = start;
                props.end = end;
                props.provisional = false;
                onsetMappedGlides[onset] = props;
                found = true;
                break;
            }
            ++scout;
        }

        if (!found) {
            int minDist = proximitySteps + 1;
            int bestOnset = -1;
            scout = onsetItr;
            while (scout != onsetOffsets.end()) {
                int onset = scout->first;
                if (onset > rangeEnd) {
                    break;
                }
                int dist = std::min(abs(start - onset), abs(end - onset));
                if (dist < minDist) {
                    minDist = dist;
                    bestOnset = onset;
                    found = true;
                }
                ++scout;
            }
            if (found) {
                cerr << "for glide from " << start << " to " << end
                     << ", found no onset within glide; using closest onset at "
                     << bestOnset << endl;
                GlideProperties props;
                props.start = start;
                props.end = end;
                props.provisional = true;
                if (onsetMappedGlides.find(bestOnset) ==
                    onsetMappedGlides.end()) {
                    cerr << "no prior glide found for this onset, marking this as provisionally best" << endl;
                    onsetMappedGlides[bestOnset] = props;
                } else {
                    // already recorded a glide for this onset
                    auto existing = onsetMappedGlides.at(bestOnset);
                    if (existing.provisional) {
                        if (bestOnset > end) {
                            // we must be closer to onset than that
                            // one (because we always see glides in
                            // ascending time order)
                            cerr << "closer to onset than prior glide, marking this as provisionally best" << endl;
                            onsetMappedGlides[bestOnset] = props;
                        } else if (bestOnset > existing.end) {
                            if ((end - start > existing.end - existing.start)
                                &&
                                minDist < bestOnset - existing.end) {
                                // this glide is longer and closer,
                                // prefer it (and since we're after
                                // onset now, it's unimprovable)
                                props.provisional = false;
                                cerr << "longer and closer to onset than prior glide, marking this as best" << endl;
                                onsetMappedGlides[bestOnset] = props;
                            } else {
                                cerr << "existing glide is closer or longer, leaving it" << endl;
                            }
                        } else {
                            cerr << "existing glide is already after onset, leaving it" << endl;
                        }
                    }
                }                            
            } else {
                cerr << "for glide from " << start << " to " << end
                     << ", found no onset within glide or within proximity "
                     << "range; ignoring this glide" << endl;
            }
        }
    }

    map<int, GlideDirection> directions; // onset -> direction
    map<int, GlideLink> links; // onset -> links
    int linkThresholdSteps = m_coreFeatures.msToSteps
        (m_linkThreshold_ms, m_coreParams.stepSize, false);
    
    for (auto m : onsetMappedGlides) {

        int onset = m.first;
        int glideStart = m.second.start;
        int glideEnd = m.second.end;

        if (pyinPitch[glideStart] < pyinPitch[glideEnd]) {
            directions[onset] = GlideDirection::Ascending;
        } else {
            directions[onset] = GlideDirection::Descending;
        }

        bool haveBefore = false, haveAfter = false;
        
        int dist = 1;
        for (int i = glideStart - 1; i >= 0; --i) {
            if (pyinPitch[i] > 0) {
                break;
            }
            ++dist;
        }
        if (dist < linkThresholdSteps) {
            haveBefore = true;
        }

        dist = 1;
        for (int i = glideEnd + 1; i < n; ++i) {
            if (pyinPitch[i] > 0) {
                break;
            }
            ++dist;
        }
        if (dist < linkThresholdSteps) {
            haveAfter = true;
        }

        if (haveBefore) {
            if (haveAfter) {
                links[onset] = GlideLink::Interconnecting;
            } else {
                links[onset] = GlideLink::Targeting;
            }
        } else {
            if (haveAfter) {
                links[onset] = GlideLink::Starting;
            } else {
                links[onset] = GlideLink::Interconnecting;
            }
        }
    }

#ifdef WITH_DEBUG_OUTPUTS
    for (size_t i = 0; i < pitchDelta.size(); ++i) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.timeForStep(i);
        f.values.push_back(pitchDelta[i]);
        fs[m_pitchDiffOutput1].push_back(f);
    }

    // NB m_pitchDiffOutput2 is unused here (and the descriptions for
    // the other diff/candidate outputs are wrong)
    
    for (size_t i = 0; i < candidates.size(); ++i) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.timeForStep(candidates[i]);
        f.values.push_back(pitchDelta[candidates[i]]);
        fs[m_candidateHopsOutput].push_back(f);
    }

    int j = 1;
    for (auto m : onsetMappedGlides) {

        int onset = m.first;
        int glideStart = m.second.start;
        int glideEnd = m.second.end;

        cerr << "returning features for glide " << j
             << " associated with onset " << onset
             << " with glide range from " << glideStart << " to " << glideEnd
             << endl;
        
        Feature f;
        f.hasTimestamp = true;
        
        {
            ostringstream os;
            os << "Glide " << j << ": Start";
            f.timestamp = m_coreFeatures.timeForStep(glideStart);
            f.values.clear();
            f.values.push_back(pyinPitch[glideStart]);
            f.label = os.str();
            fs[m_portamentoPointsOutput].push_back(f);
        }

        {
            ostringstream os;
            os << "Glide " << j << ": Onset";
            f.timestamp = m_coreFeatures.timeForStep(onset);
            f.values.clear();
            f.values.push_back(pyinPitch[onset]);
            f.label = os.str();
            fs[m_portamentoPointsOutput].push_back(f);

#ifdef WITH_DEBUG_OUTPUTS
            f.values.clear();

            f.label = glideDirectionToString(directions[onset]);
            fs[m_glideDirectionOutput].push_back(f);

            f.label = glideLinkToString(links[onset]);
            fs[m_glideLinkOutput].push_back(f);
#endif
        }

        {
            ostringstream os;
            os << "Glide " << j << ": End";
            f.timestamp = m_coreFeatures.timeForStep(glideEnd);
            f.values.clear();
            f.values.push_back(pyinPitch[glideEnd]);
            f.label = os.str();
            fs[m_portamentoPointsOutput].push_back(f);
        }

#ifdef WITH_DEBUG_OUTPUTS
        for (int k = glideStart; k <= glideEnd; ++k) {
            if (pyinPitch[k] > 0.0) {
                f.timestamp = m_coreFeatures.timeForStep(k);
                f.values.clear();
                f.values.push_back(pyinPitch[k]);
                f.label = "";
                fs[m_glidePitchTrackOutput].push_back(f);
            }
        }
#endif
        
        ++j;
    }
#endif
    
    return fs;
}

Portamento::FeatureSet
Portamento::getRemainingFeatures()
{
    return getRemainingFeatures_glideModel2();
}

