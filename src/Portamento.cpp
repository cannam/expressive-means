
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
Portamento::getRemainingFeatures()
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
    Glide::Extents glides = glide.extract
        (pyinPitch, m_coreFeatures.getOnsetOffsets());

    // The following all use the onset step number as the identifier
    // for a specific glide (the one associated with that onset)
    // 
    map<int, GlideDirection> directions;  // onset -> direction
    map<int, double> ranges;              // onset -> pitch change, semis
    map<int, double> durations;           // onset -> duration in msec
    map<int, GlideLink> links;            // onset -> links
    
    int linkThresholdSteps = m_coreFeatures.msToSteps
        (m_linkThreshold_ms, m_coreParams.stepSize, false);
    
    for (auto m : glides) {

        int onset = m.first;
        int glideStart = m.second.start;
        int glideEnd = m.second.end;

        if (pyinPitch[glideStart] < pyinPitch[glideEnd]) {
            directions[onset] = GlideDirection::Ascending;
        } else {
            directions[onset] = GlideDirection::Descending;
        }

        ranges[onset] =
            m_coreFeatures.hzToPitch(pyinPitch[glideEnd]) -
            m_coreFeatures.hzToPitch(pyinPitch[glideStart]);

        durations[onset] =
            m_coreFeatures.stepsToMs(glideEnd - glideStart + 1,
                                     m_coreParams.stepSize);
            
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
    int j = 1;
    for (auto m : glides) {

        int onset = m.first;
        int glideStart = m.second.start;
        int glideEnd = m.second.end;

        cerr << "returning features for glide " << j
             << " associated with onset " << onset
             << " with glide from " << glideStart << " to " << glideEnd
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
