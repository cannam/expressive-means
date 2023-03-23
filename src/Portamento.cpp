
/*
    Expressive Means Portamento

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Portamento.h"
#include "Glide.h"

#include <vector>
#include <set>

using std::cerr;
using std::endl;
using std::vector;
using std::set;
using std::map;
using std::lower_bound;
using std::ostringstream;

static const float default_glideThresholdPitch_cents = 50.f;
static const float default_glideThresholdDuration_ms = 70.f;
static const float default_glideThresholdProximity_ms = 350.f;
static const float default_linkThreshold_ms = 30.f;
static const float default_rangeBoundaryMedium_cents = 250.f;
static const float default_rangeBoundaryLarge_cents = 550.f;
static const float default_durationBoundaryMedium_ms = 120.f;
static const float default_durationBoundaryLong_ms = 210.f;
static const float default_dynamicsThreshold_dB = 1.f;
static const float default_scalingFactor = 7.4f;

Portamento::Portamento(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_stepSize(0),
    m_blockSize(0),
    m_coreFeatures(inputSampleRate),
    m_glideThresholdPitch_cents(default_glideThresholdPitch_cents),
    m_glideThresholdDuration_ms(default_glideThresholdDuration_ms),
    m_glideThresholdProximity_ms(default_glideThresholdProximity_ms),
    m_linkThreshold_ms(default_linkThreshold_ms),
    m_rangeBoundaryMedium_cents(default_rangeBoundaryMedium_cents),
    m_rangeBoundaryLarge_cents(default_rangeBoundaryLarge_cents),
    m_durationBoundaryMedium_ms(default_durationBoundaryMedium_ms),
    m_durationBoundaryLong_ms(default_durationBoundaryLong_ms),
    m_dynamicsThreshold_dB(default_dynamicsThreshold_dB),
    m_scalingFactor(default_scalingFactor),
    m_summaryOutput(-1),
    m_portamentoTypeOutput(-1),
    m_pitchTrackOutput(-1),
    m_portamentoIndexOutput(-1),
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
    
    d.identifier = "rangeBoundaryMedium";
    d.name = "Range threshold: medium";
    d.unit = "cents";
    d.minValue = 0.f;
    d.maxValue = 1200.f;
    d.defaultValue = default_rangeBoundaryMedium_cents;
    list.push_back(d);
    
    d.identifier = "rangeBoundaryLarge";
    d.name = "Range threshold: large";
    d.unit = "cents";
    d.minValue = 0.f;
    d.maxValue = 1200.f;
    d.defaultValue = default_rangeBoundaryLarge_cents;
    list.push_back(d);
    
    d.identifier = "durationBoundaryMedium";
    d.name = "Duration threshold: medium";
    d.unit = "ms";
    d.minValue = 0.f;
    d.maxValue = 1000.f;
    d.defaultValue = default_durationBoundaryMedium_ms;
    list.push_back(d);
    
    d.identifier = "durationBoundaryLong";
    d.name = "Duration threshold: long";
    d.unit = "ms";
    d.minValue = 0.f;
    d.maxValue = 4000.f;
    d.defaultValue = default_durationBoundaryLong_ms;
    list.push_back(d);
    
    d.identifier = "dynamicsThreshold";
    d.name = "Dynamics threshold";
    d.unit = "dB";
    d.minValue = 0.f;
    d.maxValue = 10.f;
    d.defaultValue = default_dynamicsThreshold_dB;
    list.push_back(d);
        
    d.identifier = "scalingFactor";
    d.name = "Index scaling factor";
    d.unit = "";
    d.minValue = 1.f;
    d.maxValue = 30.f;
    d.defaultValue = default_scalingFactor;
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
    } else if (identifier == "rangeBoundaryMedium") {
        return m_rangeBoundaryMedium_cents;
    } else if (identifier == "rangeBoundaryLarge") {
        return m_rangeBoundaryLarge_cents;
    } else if (identifier == "durationBoundaryMedium") {
        return m_durationBoundaryMedium_ms;
    } else if (identifier == "durationBoundaryLong") {
        return m_durationBoundaryLong_ms;
    } else if (identifier == "dynamicsThreshold") {
        return m_dynamicsThreshold_dB;
    } else if (identifier == "scalingFactor") {
        return m_scalingFactor;
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
    } else if (identifier == "rangeBoundaryMedium") {
        m_rangeBoundaryMedium_cents = value;
    } else if (identifier == "rangeBoundaryLarge") {
        m_rangeBoundaryLarge_cents = value;
    } else if (identifier == "durationBoundaryMedium") {
        m_durationBoundaryMedium_ms = value;
    } else if (identifier == "durationBoundaryLong") {
        m_durationBoundaryLong_ms = value;
    } else if (identifier == "dynamicsThreshold") {
        m_dynamicsThreshold_dB = value;
    } else if (identifier == "scalingFactor") {
        m_scalingFactor = value;
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
    d.binCount = 0;
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
    
    d.identifier = "glideDynamic";
    d.name = "[Debug] Glide Dynamic";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 0;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_glideDynamicOutput = int(list.size());
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

Portamento::GlideClassification
Portamento::classifyGlide(const Glide::Extent &extent,
                          const vector<double> &pyinPitch,
                          const vector<double> &smoothedPower)
{
    GlideClassification classification;

    // Direction
    
    if (pyinPitch[extent.start] < pyinPitch[extent.end]) {
        classification.direction = GlideDirection::Ascending;
    } else {
        classification.direction = GlideDirection::Descending;
    }

    // Range
    
    double range =
        m_coreFeatures.hzToPitch(pyinPitch[extent.end]) -
        m_coreFeatures.hzToPitch(pyinPitch[extent.start]);

    classification.range_cents = range;
    
    if (fabs(range) > m_rangeBoundaryLarge_cents / 100.0) {
        classification.range = GlideRange::Large;
    } else if (fabs(range) > m_rangeBoundaryMedium_cents / 100.0) {
        classification.range = GlideRange::Medium;
    } else {
        classification.range = GlideRange::Small;
    }

    // Duration
    
    double duration =
        m_coreFeatures.stepsToMs(extent.end - extent.start + 1,
                                 m_coreParams.stepSize);

    if (duration > m_durationBoundaryLong_ms) {
        classification.duration = GlideDuration::Long;
    } else if (duration > m_durationBoundaryMedium_ms) {
        classification.duration = GlideDuration::Medium;
    } else {
        classification.duration = GlideDuration::Short;
    }

    // Link
    
    bool haveBefore = false, haveAfter = false;

    int linkThresholdSteps = m_coreFeatures.msToSteps
        (m_linkThreshold_ms, m_coreParams.stepSize, false);
    
    int dist = 1;
    for (int i = extent.start - 1; i >= 0; --i) {
        if (pyinPitch[i] > 0) {
            break;
        }
        ++dist;
    }
    
    if (dist < linkThresholdSteps) {
        haveBefore = true;
    }
    
    dist = 1;
    int n = int(pyinPitch.size());
    for (int i = extent.end + 1; i < n; ++i) {
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
            classification.link = GlideLink::Interconnecting;
        } else {
            classification.link = GlideLink::Targeting;
        }
    } else {
        if (haveAfter) {
            classification.link = GlideLink::Starting;
        } else {
            classification.link = GlideLink::Interconnecting;
        }
    }

    // Dynamic

    double preceding, succeeding;

    if (extent.start > 0) {
        preceding = smoothedPower[extent.start - 1];
    } else {
        preceding = smoothedPower[0];
    }

    if (extent.end + 1 < n) {
        succeeding = smoothedPower[extent.end + 1];
    } else {
        succeeding = smoothedPower[n - 1];
    }

    double max, min;

    for (int i = extent.start; i <= extent.end; ++i) {
        if (i == extent.start || smoothedPower[i] < min) {
            min = smoothedPower[i];
        }
        if (i == extent.start || smoothedPower[i] > max) {
            max = smoothedPower[i];
        }
    }

    double threshold = m_dynamicsThreshold_dB;
    
    if (max > preceding + threshold && max > succeeding + threshold) {
        classification.dynamic = GlideDynamic::Loud;
    } else if (min < preceding - threshold && min < succeeding - threshold) {
        classification.dynamic = GlideDynamic::Quiet;
    } else {
        classification.dynamic = GlideDynamic::Stable;
    }

    if (preceding > succeeding) {
        classification.dynamicMax = max - preceding;
    } else {
        classification.dynamicMax = max - succeeding;
    }

    if (preceding > succeeding) {
        classification.dynamicMin = min - succeeding;
    } else {
        classification.dynamicMin = min - preceding;
    }
    
    return classification;
}

Portamento::FeatureSet
Portamento::getRemainingFeatures()
{
    FeatureSet fs;

    m_coreFeatures.finish();

    auto pyinPitch = m_coreFeatures.getPYinPitch_Hz();
    auto smoothedPower = m_coreFeatures.getSmoothedPower_dB();
    auto onsetOffsets = m_coreFeatures.getOnsetOffsets();

    for (int i = 0; i < int(pyinPitch.size()); ++i) {
        if (pyinPitch[i] <= 0) continue;
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.timeForStep(i);
        f.values.push_back(pyinPitch[i]);
        fs[m_pitchTrackOutput].push_back(f);
    }

    int n = int(pyinPitch.size());
    
    Glide::Parameters glideParams;
    glideParams.durationThreshold_steps =
        m_coreFeatures.msToSteps(m_glideThresholdDuration_ms,
                                 m_coreParams.stepSize, false);
    glideParams.onsetProximityThreshold_steps =
        m_coreFeatures.msToSteps(m_glideThresholdProximity_ms,
                                 m_coreParams.stepSize, false);
    glideParams.pitchThreshold_semis =
        m_glideThresholdPitch_cents / 100.0;

    Glide glide(glideParams);
    Glide::Extents glides = glide.extract(pyinPitch, onsetOffsets);

    // Using onset step number as the key
    map<int, GlideClassification> classifications;
    
    for (auto m : glides) {
        classifications[m.first] =
            classifyGlide(m.second, pyinPitch, smoothedPower);
    }

#ifdef WITH_DEBUG_OUTPUTS
    int j = 1;
    for (auto m : glides) {

        int onset = m.first;
        int glideStart = m.second.start;
        int glideEnd = m.second.end;
        string code;
        double index = 1.0;

        cerr << "returning features for glide " << j
             << " associated with onset " << onset
             << " with glide from " << glideStart << " to " << glideEnd
             << endl;

        int followingOnset = onset;
        auto itr = onsetOffsets.find(onset);
        if (++itr != onsetOffsets.end()) {
            followingOnset = itr->first;
        }
        
        Feature f;
        f.hasTimestamp = true;

        GlideDirection direction = classifications[onset].direction;
        code += glideDirectionToCode(direction);
        index *= glideDirectionToFactor(direction);

        GlideLink link = classifications[onset].link;
        code += glideLinkToCode(link);
        index *= glideLinkToFactor(link);

        GlideRange range = classifications[onset].range;
        code += glideRangeToCode(range);
        index *= glideRangeToFactor(range);

        GlideDuration duration = classifications[onset].duration;
        code += glideDurationToCode(duration);
        index *= glideDurationToFactor(duration);

        GlideDynamic dynamic = classifications[onset].dynamic;
        code += glideDynamicToCode(dynamic);
        index *= glideDynamicToFactor(dynamic);

        index *= m_scalingFactor;
        
        f.timestamp = m_coreFeatures.timeForStep(onset);
        f.hasDuration = false;
        f.label = code;
        fs[m_portamentoTypeOutput].push_back(f);

        f.label = "";
        f.values.push_back(round(index));
        fs[m_portamentoIndexOutput].push_back(f);
        
        double sp2dp = round(pyinPitch.at(glideStart) * 100.0) / 100.0;
        double ep2dp = round(pyinPitch.at(glideEnd) * 100.0) / 100.0;
        double range2dp = round(classifications[onset].range_cents * 100.0) / 100.0;
        double emin2dp = round(classifications[onset].dynamicMin * 100.0) / 100.0;
        double emax2dp = round(classifications[onset].dynamicMax * 100.0) / 100.0;

        ostringstream os;
        os << m_coreFeatures.timeForStep(onset).toText() << " / "
           << (m_coreFeatures.timeForStep(followingOnset) -
               m_coreFeatures.timeForStep(onset)).toText() << "\n"
           << code << "\n"
           << sp2dp << "Hz / " << ep2dp << "Hz (" << range2dp << "c)\n"
           << m_coreFeatures.timeForStep(glideStart).toText() << " / "
           << m_coreFeatures.timeForStep(glideEnd).toText() << " ("
           << round(m_coreFeatures.stepsToMs
                    (glideEnd - glideStart + 1, m_coreParams.stepSize))
           << "ms)\n"
           << emax2dp << "dB / " << emin2dp << "dB\n"
           << "IPort = " << index;
        f.label = os.str();
        f.values.clear();
        fs[m_summaryOutput].push_back(f);
        
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

            f.label = glideDirectionToString(direction);
            fs[m_glideDirectionOutput].push_back(f);

            f.label = glideLinkToString(link);
            fs[m_glideLinkOutput].push_back(f);
            
            f.label = glideDynamicToString(dynamic);
            fs[m_glideDynamicOutput].push_back(f);
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
