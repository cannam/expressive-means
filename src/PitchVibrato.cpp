
/*
    Expressive Means Pitch Vibrato

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "PitchVibrato.h"
#include "Glide.h"

#include "../ext/qm-dsp/maths/MathUtilities.h"

#include <vector>
#include <set>
#include <sstream>

using std::cerr;
using std::endl;
using std::vector;
using std::set;
using std::map;
using std::ostringstream;

//#define DEBUG_PITCH_VIBRATO 1

static const float default_vibratoRateMinimum_Hz = 4.2f;
static const float default_vibratoRateMaximum_Hz = 9.2f;
static const float default_vibratoRangeMinimum_cents = 20.f;
static const float default_vibratoRangeMaximum_cents = 200.f;
static const float default_rateBoundaryModerate_Hz = 6.2f;
static const float default_rateBoundaryFast_Hz = 7.2f;
static const float default_rangeBoundaryMedium_cents = 40.f;
static const float default_rangeBoundaryWide_cents = 60.f;
static const float default_sectionThreshold_ms = 200.f;
static const float default_developmentThreshold_cents = 10.f;
static const float default_correlationThreshold = 0.2f;
static const float default_scalingFactor = 11.1f;
static const float default_smoothingWindowLength_ms = 70.f;
static const PitchVibrato::SegmentationType default_segmentationType =
    PitchVibrato::SegmentationType::WithoutGlidesAndSegmented;

// These are not exposed as plugin parameters, so they're fixed but
// can be tweaked here
static const float default_glideThresholdPitch_cents = 60.f;
static const float default_glideThresholdHopMinimum_cents = 10.f;
static const float default_glideThresholdHopMaximum_cents = 50.f;
static const float default_glideThresholdDuration_ms = 70.f;
static const float default_glideThresholdProximity_ms = 350.f;

PitchVibrato::PitchVibrato(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_stepSize(0),
    m_blockSize(0),
    m_coreFeatures(inputSampleRate),
    m_vibratoRateMinimum_Hz(default_vibratoRateMinimum_Hz),
    m_vibratoRateMaximum_Hz(default_vibratoRateMaximum_Hz),
    m_vibratoRangeMinimum_cents(default_vibratoRangeMinimum_cents),
    m_vibratoRangeMaximum_cents(default_vibratoRangeMaximum_cents),
    m_rateBoundaryModerate_Hz(default_rateBoundaryModerate_Hz),
    m_rateBoundaryFast_Hz(default_rateBoundaryFast_Hz),
    m_rangeBoundaryMedium_cents(default_rangeBoundaryMedium_cents),
    m_rangeBoundaryWide_cents(default_rangeBoundaryWide_cents),
    m_sectionThreshold_ms(default_sectionThreshold_ms),
    m_developmentThreshold_cents(default_developmentThreshold_cents),
    m_correlationThreshold(default_correlationThreshold),
    m_scalingFactor(default_scalingFactor),
    m_smoothingWindowLength_ms(default_smoothingWindowLength_ms),
    m_glideThresholdPitch_cents(default_glideThresholdPitch_cents),
    m_glideThresholdHopMinimum_cents(default_glideThresholdHopMinimum_cents),
    m_glideThresholdHopMaximum_cents(default_glideThresholdHopMaximum_cents),
    m_glideThresholdDuration_ms(default_glideThresholdDuration_ms),
    m_glideThresholdProximity_ms(default_glideThresholdProximity_ms),
    m_segmentationType(default_segmentationType),
    m_summaryOutput(-1),
    m_pitchTrackOutput(-1),
    m_vibratoTypeOutput(-1),
    m_vibratoIndexOutput(-1),
    m_vibratoPitchTrackOutput(-1)
#ifdef WITH_DEBUG_OUTPUTS
    ,
    m_rawPeaksOutput(-1),
    m_acceptedPeaksOutput(-1)
#endif
{
}

PitchVibrato::~PitchVibrato()
{
}

string
PitchVibrato::getIdentifier() const
{
    return "pitch-vibrato";
}

string
PitchVibrato::getName() const
{
    return "Expressive Means (advanced): Pitch Vibrato";
}

string
PitchVibrato::getDescription() const
{
    return "identifies types and intensities of pitch vibrato instances in monophonic recordings (specified parameter settings)";
}

string
PitchVibrato::getMaker() const
{
    return "Frithjof Vollmer and Chris Cannam, method partly by Tilo Haehnel";
}

int
PitchVibrato::getPluginVersion() const
{
    return 1;
}

string
PitchVibrato::getCopyright() const
{
    return "GPLv2";
}

PitchVibrato::InputDomain
PitchVibrato::getInputDomain() const
{
    return TimeDomain;
}

size_t
PitchVibrato::getPreferredBlockSize() const
{
    return m_coreFeatures.getPreferredBlockSize();
}

size_t 
PitchVibrato::getPreferredStepSize() const
{
    return m_coreFeatures.getPreferredStepSize();
}

size_t
PitchVibrato::getMinChannelCount() const
{
    return 1;
}

size_t
PitchVibrato::getMaxChannelCount() const
{
    return 1;
}

PitchVibrato::ParameterList
PitchVibrato::getParameterDescriptors() const
{
    ParameterList list;

    m_coreParams.appendVampParameterDescriptors(list, true);
    
    ParameterDescriptor d;

    d.description = "";
    d.isQuantized = false;
    
    d.identifier = "vibratoRateMinimum";
    d.name = "Vibrato rate: Minimum";
    d.unit = "Hz";
    d.minValue = 0.1f;
    d.maxValue = 20.f;
    d.defaultValue = default_vibratoRateMinimum_Hz;
    list.push_back(d);
    
    d.identifier = "vibratoRateMaximum";
    d.name = "Vibrato rate: Maximum";
    d.unit = "Hz";
    d.minValue = 0.1f;
    d.maxValue = 20.f;
    d.defaultValue = default_vibratoRateMaximum_Hz;
    list.push_back(d);
    
    d.identifier = "rateBoundaryModerate";
    d.name = "Rate threshold: moderate";
    d.unit = "Hz";
    d.minValue = 0.f;
    d.maxValue = 20.f;
    d.defaultValue = default_rateBoundaryModerate_Hz;
    list.push_back(d);
    
    d.identifier = "rateBoundaryFast";
    d.name = "Rate threshold: fast";
    d.unit = "Hz";
    d.minValue = 0.f;
    d.maxValue = 20.f;
    d.defaultValue = default_rateBoundaryFast_Hz;
    list.push_back(d);
    
    d.identifier = "vibratoRangeMinimum";
    d.name = "Vibrato range: Minimum";
    d.unit = "cents";
    d.minValue = 1.f;
    d.maxValue = 1000.f;
    d.defaultValue = default_vibratoRangeMinimum_cents;
    list.push_back(d);
    
    d.identifier = "vibratoRangeMaximum";
    d.name = "Vibrato range: Maximum";
    d.unit = "cents";
    d.minValue = 1.f;
    d.maxValue = 1000.f;
    d.defaultValue = default_vibratoRangeMaximum_cents;
    list.push_back(d);
    
    d.identifier = "rangeBoundaryMedium";
    d.name = "Range threshold: medium";
    d.unit = "cents";
    d.minValue = 0.f;
    d.maxValue = 250.f;
    d.defaultValue = default_rangeBoundaryMedium_cents;
    list.push_back(d);
    
    d.identifier = "rangeBoundaryWide";
    d.name = "Range threshold: wide";
    d.unit = "cents";
    d.minValue = 0.f;
    d.maxValue = 250.f;
    d.defaultValue = default_rangeBoundaryWide_cents;
    list.push_back(d);
    
    d.identifier = "sectionThreshold";
    d.name = "Section duration threshold";
    d.unit = "ms";
    d.minValue = 0.f;
    d.maxValue = 1000.f;
    d.defaultValue = default_sectionThreshold_ms;
    list.push_back(d);
    
    d.identifier = "developmentThreshold";
    d.name = "Development threshold";
    d.unit = "cents";
    d.minValue = 0.f;
    d.maxValue = 200.f;
    d.defaultValue = default_developmentThreshold_cents;
    list.push_back(d);
    
    d.identifier = "correlationThreshold";
    d.name = "Vibrato shape: Correlation threshold";
    d.unit = "";
    d.minValue = 0.1f;
    d.maxValue = 1.f;
    d.defaultValue = default_correlationThreshold;
    list.push_back(d);
    
    d.identifier = "scalingFactor";
    d.name = "Index scaling factor";
    d.unit = "";
    d.minValue = 1.f;
    d.maxValue = 30.f;
    d.defaultValue = default_scalingFactor;
    list.push_back(d);

    d.identifier = "smoothingWindowLength";
    d.name = "Smoothing window length";
    d.description = "Length of mean filter used to smooth the pitch track for peak selection. Other measurements are always performed from the un-smoothed track.";
    d.unit = "ms";
    d.minValue = 0.f;
    d.maxValue = 150.f;
    d.defaultValue = default_smoothingWindowLength_ms;
    list.push_back(d);

    d.identifier = "segmentationType";
    d.name = "Note segmentation";
    d.description = "Preprocessing to apply before vibrato peak selection. None means the whole pitch track is considered at once. Segmented means individual notes are treated separately. Without Glides means the whole track is considered after glides have been identified and removed. Without Glides And Segmented means individual notes are considered after glides removed.";
    d.unit = "";
    d.minValue = 0.f;
    d.maxValue = 3.f;
    d.isQuantized = true;
    d.quantizeStep = 1.f;
    d.valueNames.push_back("None");
    d.valueNames.push_back("Segmented");
    d.valueNames.push_back("Without Glides");
    d.valueNames.push_back("Without Glides and Segmented");
    d.defaultValue = int(default_segmentationType);
    list.push_back(d);
        
    return list;
}

float
PitchVibrato::getParameter(string identifier) const
{
    float value = 0.f;
    if (m_coreParams.obtainVampParameter(identifier, value)) {
        return value;
    }

    if (identifier == "vibratoRateMinimum") {
        return m_vibratoRateMinimum_Hz;
    } else if (identifier == "vibratoRateMaximum") {
        return m_vibratoRateMaximum_Hz;
    } else if (identifier == "rateBoundaryModerate") {
        return m_rateBoundaryModerate_Hz;
    } else if (identifier == "rateBoundaryFast") {
        return m_rateBoundaryFast_Hz;
    } else if (identifier == "vibratoRangeMinimum") {
        return m_vibratoRangeMinimum_cents;
    } else if (identifier == "vibratoRangeMaximum") {
        return m_vibratoRangeMaximum_cents;
    } else if (identifier == "rangeBoundaryMedium") {
        return m_rangeBoundaryMedium_cents;
    } else if (identifier == "rangeBoundaryWide") {
        return m_rangeBoundaryWide_cents;
    } else if (identifier == "sectionThreshold") {
        return m_sectionThreshold_ms;
    } else if (identifier == "developmentThreshold") {
        return m_developmentThreshold_cents;
    } else if (identifier == "correlationThreshold") {
        return m_correlationThreshold;
    } else if (identifier == "scalingFactor") {
        return m_scalingFactor;
    } else if (identifier == "smoothingWindowLength") {
        return m_smoothingWindowLength_ms;
    } else if (identifier == "segmentationType") {
        return int(m_segmentationType);
    }
    
    return 0.f;
}

void
PitchVibrato::setParameter(string identifier, float value) 
{
    if (m_coreParams.acceptVampParameter(identifier, value)) {
        return;
    }

    if (identifier == "vibratoRateMinimum") {
        m_vibratoRateMinimum_Hz = value;
    } else if (identifier == "vibratoRateMaximum") {
        m_vibratoRateMaximum_Hz = value;
    } else if (identifier == "rateBoundaryModerate") {
        m_rateBoundaryModerate_Hz = value;
    } else if (identifier == "rateBoundaryFast") {
        m_rateBoundaryFast_Hz = value;
    } else if (identifier == "vibratoRangeMinimum") {
        m_vibratoRangeMinimum_cents = value;
    } else if (identifier == "vibratoRangeMaximum") {
        m_vibratoRangeMaximum_cents = value;
    } else if (identifier == "rangeBoundaryMedium") {
        m_rangeBoundaryMedium_cents = value;
    } else if (identifier == "rangeBoundaryWide") {
        m_rangeBoundaryWide_cents = value;
    } else if (identifier == "sectionThreshold") {
        m_sectionThreshold_ms = value;
    } else if (identifier == "developmentThreshold") {
        m_developmentThreshold_cents = value;
    } else if (identifier == "correlationThreshold") {
        m_correlationThreshold = value;
    } else if (identifier == "scalingFactor") {
        m_scalingFactor = value;
    } else if (identifier == "smoothingWindowLength") {
        m_smoothingWindowLength_ms = value;
    } else if (identifier == "segmentationType") {
        if (value < 0.5f) {
            m_segmentationType = SegmentationType::Unsegmented;
        } else if (value < 1.5f) {
            m_segmentationType = SegmentationType::Segmented;
        } else if (value < 2.5f) {
            m_segmentationType = SegmentationType::WithoutGlides;
        } else {
            m_segmentationType = SegmentationType::WithoutGlidesAndSegmented;
        }
    }
}

PitchVibrato::ProgramList
PitchVibrato::getPrograms() const
{
    ProgramList list;
    return list;
}

string
PitchVibrato::getCurrentProgram() const
{
    return ""; 
}

void
PitchVibrato::selectProgram(string)
{
}

PitchVibrato::OutputList
PitchVibrato::getOutputDescriptors() const
{
    OutputList list;
    OutputDescriptor d;
    
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = (m_inputSampleRate / m_stepSize);
    
    d.identifier = "smoothedPitchTrack";
    d.name = "Smoothed Pitch Track";
    d.description = "The pitch track computed by pYIN, with further smoothing as used for peak selection.";
    d.unit = "Hz";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_pitchTrackOutput = int(list.size());
    list.push_back(d);

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
    
    d.identifier = "vibratoType";
    d.name = "Vibrato Type";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 0;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_vibratoTypeOutput = int(list.size());
    list.push_back(d);
    
    d.identifier = "vibratoIndex";
    d.name = "Vibrato Index";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.hasDuration = false;
    m_vibratoIndexOutput = int(list.size());
    list.push_back(d);
    
    d.identifier = "vibratoPitchTrack";
    d.name = "Vibrato-Only Pitch Track";
    d.description = "";
    d.unit = "Hz";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = (m_inputSampleRate / m_stepSize);
    d.hasDuration = false;
    m_vibratoPitchTrackOutput = int(list.size());
    list.push_back(d);

#ifdef WITH_DEBUG_OUTPUTS
    d.identifier = "rawpeaks";
    d.name = "[Debug] Raw Peaks";
    d.description = "Simple local maxima of pitch track.";
    d.hasFixedBinCount = true;
    d.binCount = 0;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = (m_inputSampleRate / m_stepSize);
    d.hasDuration = false;
    m_rawPeaksOutput = int(list.size());
    list.push_back(d);
    
    d.identifier = "peaks";
    d.name = "[Debug] Peaks";
    d.description = "Locations and pitches of vibrato peaks.";
    d.unit = "Hz";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.sampleRate = 0.f;
    d.hasDuration = false;
    m_acceptedPeaksOutput = int(list.size());
    list.push_back(d);
#endif
    
    return list;
}

bool
PitchVibrato::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    if (channels < getMinChannelCount() || channels > getMaxChannelCount()) {
        cerr << "ERROR: PitchVibrato::initialise: unsupported channel count "
             << channels << endl;
        return false;
    }

    if (m_inputSampleRate < 8000.0) {
        cerr << "ERROR: PitchVibrato::initialise: sample rate ("
             << m_inputSampleRate << ") is too low, it must be at least 8kHz"
             << endl;
        return false;
    }
    
    if (m_inputSampleRate > 192000.0) {
        cerr << "ERROR: PitchVibrato::initialise: sample rate ("
             << m_inputSampleRate << ") is too high, maximum is 192kHz"
             << endl;
        return false;
    }
    
    if (stepSize > blockSize) {
        cerr << "ERROR: PitchVibrato::initialise: step size (" << stepSize
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
        cerr << "ERROR: PitchVibrato::initialise: Feature extractor initialisation failed: " << e.what() << endl;
        return false;
    }
    
    return true;
}

void
PitchVibrato::reset()
{
    m_coreFeatures.reset();
}

PitchVibrato::FeatureSet
PitchVibrato::process(const float *const *inputBuffers, Vamp::RealTime timestamp)
{
    m_coreFeatures.process(inputBuffers[0], timestamp);
    return {};
}

vector<PitchVibrato::VibratoElement>
PitchVibrato::extractElements(const vector<double> &pyinPitch_Hz,
                              vector<double> &smoothedPitch_semis,
                              vector<int> &rawPeaks) const
{
    // The numbered comments correspond to the numbered steps in Tilo
    // Haehnel's paper

    // 1. Convert pitch track from Hz to cents and smooth with a 35ms
    // mean filter. (The paper says 35ms, but it appears from the R
    // code that it is 35ms either side of the centre, so 70ms
    // total. We make the value configurable but with 70ms default.)

    int filterLength_steps = m_coreFeatures.msToSteps
        (m_smoothingWindowLength_ms, m_coreParams.stepSize, true);
    
#ifdef DEBUG_PITCH_VIBRATO
    cerr << "** 1. Smooth pitch track with a mean filter of "
         << m_smoothingWindowLength_ms << "ms (" << filterLength_steps << " hops)" << endl;
#endif
    
    vector<double> unsmoothedPitch_semis;
    for (auto hz : pyinPitch_Hz) {
        if (hz > 0.0) {
            unsmoothedPitch_semis.push_back(m_coreFeatures.hzToPitch(hz));
        } else {
            unsmoothedPitch_semis.push_back(0.0);
        }
    }

    int n = unsmoothedPitch_semis.size();
    smoothedPitch_semis = vector<double>(n, 0.0);

    // Filter in a way that accounts correctly for missing data (zero
    // pitch values)
    for (int i = 0; i < n; ++i) {
        if (unsmoothedPitch_semis[i] != 0.0) {
            double total = 0.0;
            int count = 0;
            for (int j = 0; j < filterLength_steps/2; ++j) {
                int ix = i - j;
                if (ix < 0 || unsmoothedPitch_semis[ix] == 0.0) {
                    break;
                }
                total += unsmoothedPitch_semis[ix];
                count += 1;
            }
            for (int j = 0; j < filterLength_steps/2; ++j) {
                int ix = i + j;
                if (ix >= n || unsmoothedPitch_semis[ix] == 0.0) {
                    break;
                }
                total += unsmoothedPitch_semis[ix];
                count += 1;
            }
            if (count > 0) {
                smoothedPitch_semis[i] = total / count;
            }
        }
    }
    
#ifdef DEBUG_PITCH_VIBRATO
    cerr << "** 1. Complete, have " << smoothedPitch_semis.size() << " hops" << endl;
#endif

    // 2. Identify peaks - simply local maxima, with values greater
    // then their immediate neighbours (where they all have valid
    // pitch values)

#ifdef DEBUG_PITCH_VIBRATO
    cerr << "** 2. Identify local maxima in smoothed pitch track" << endl;
#endif
    
    vector<int> peaks;
    for (int i = 0; i < n; ++i) {
        if (smoothedPitch_semis[i] <= 0.0) {
            continue;
        }
        bool left = (i == 0 ||
                     smoothedPitch_semis[i-1] <= 0.0 ||
                     smoothedPitch_semis[i] > smoothedPitch_semis[i-1]);
        bool right = (i + 1 == n ||
                      smoothedPitch_semis[i+1] <= 0.0 ||
                      smoothedPitch_semis[i] >= smoothedPitch_semis[i+1]);
        if (left && right) {
#ifdef DEBUG_PITCH_VIBRATO
            cerr << "-- Local maximum at " << i << ": smoothed pitch "
                 << smoothedPitch_semis[i] << " (original pitch "
                 << pyinPitch_Hz[i] << ") >= "
                 << (i > 0 ? smoothedPitch_semis[i-1] : -999.0)
                 << " (before) and > "
                 << (i + 1 < n ? smoothedPitch_semis[i+1] : -999.0)
                 << " (after)" << endl;
#endif
            peaks.push_back(i);
        }
    }
    rawPeaks = peaks;

#ifdef DEBUG_PITCH_VIBRATO
    cerr << "** 2. Complete, found " << rawPeaks.size() << " local maxima" << endl;
#endif

    // Use parabolic interpolation to identify a more precise peak
    // location. This is step 6 in the paper, but we'll do it now
    // because we want to ensure the location of the "following" peak
    // for every accepted peak is available even if the following peak
    // is itself not accepted.

#ifdef DEBUG_PITCH_VIBRATO
    cerr << "** 2a (6 in paper). Interpolate precise peak timings" << endl;
#endif

    vector<double> positions; // positions[i] is time in sec of peaks[i]
    
    for (int i = 0; i < int(peaks.size()); ++i) {
        int hop = peaks[i];
        double refinedStep = hop;
        if (hop >= 1 && hop + 1 < n) {
            double alpha = smoothedPitch_semis[hop - 1];
            double beta = smoothedPitch_semis[hop];
            double gamma = smoothedPitch_semis[hop + 1];
            if (alpha > 0.0 && beta > 0.0 && gamma > 0.0) {
                double denom = alpha - 2.0 * beta + gamma;
                double p = (denom != 0.0 ? ((alpha - gamma) / denom * 0.5) : 0.0);
                refinedStep += p;
            }
        }
        double sec =
            ((refinedStep +
              double((m_coreParams.blockSize / m_coreParams.stepSize) / 2)) *
             double(m_coreParams.stepSize))
            / double(m_inputSampleRate);
#ifdef DEBUG_PITCH_VIBRATO
        cerr << "-- Peak at hop " << hop << " refined to hop " << refinedStep
             << ", from nominal time "
             << ((double(hop) + double((m_coreParams.blockSize / m_coreParams.stepSize) / 2)) * double(m_coreParams.stepSize)) / double(m_inputSampleRate)
             << " to " << sec << "s" << endl;
#endif
        positions.push_back(sec);
    }

#ifdef DEBUG_PITCH_VIBRATO
    cerr << "** 2a (6 in paper). Complete" << endl;
#endif
    
    // Number of peak-to-peak ranges found (NB this could be -1 if no
    // peaks at all found!)
    int npairs = int(peaks.size()) - 1;

    // The original paper says
    // 
    // 3. Eliminate those peaks that don't have at least 10 valid
    // pitch values between them and their following peak, and
    //
    // 4. Eliminate those peaks that are not spaced at an interval
    // between 62.5 and 416.7 ms apart (i.e. 16 Hz to 2.4 Hz), and
    // 
    // 5. Eliminate those peaks that don't rise to within 20-800 cents
    // relative to the intervening minimum
    //
    // We have parameters for the rate and range min and max which we
    // use instead of these. We need to be careful with "at least 10
    // valid pitch values", since at 256 hop, 48kHz a 16Hz vibrato
    // only has 11.7 hops per cycle in the first place. Let's ask for
    // at least 0.8x the total number of hops.

#ifdef DEBUG_PITCH_VIBRATO
    cerr << "** 3-5. Evaluate peak-to-peak pairs against basic vibrato criteria" << endl;
#endif
    
    int minDistSteps = m_coreFeatures.msToSteps
        (1000.0 / m_vibratoRateMaximum_Hz, m_coreParams.stepSize, false);
    int maxDistSteps = m_coreFeatures.msToSteps
        (1000.0 / m_vibratoRateMinimum_Hz, m_coreParams.stepSize, false);

    int minPitchedHops = (minDistSteps / 5) * 4;

    double minHeight = m_vibratoRangeMinimum_cents / 100.0; // semitones
    double maxHeight = m_vibratoRangeMaximum_cents / 100.0;
    
    vector<VibratoElement> elements;
    
    for (int i = 0; i < npairs; ++i) {

#ifdef DEBUG_PITCH_VIBRATO
        cerr << "-- Considering peak at step " << peaks[i]
             << " (following peak is at " << peaks[i+1] << ")"
             << endl;
#endif

        // Establish time criterion (4)
        int steps = peaks[i+1] - peaks[i];
        if (steps < minDistSteps || steps > maxDistSteps) {
#ifdef DEBUG_PITCH_VIBRATO
            cerr << "-- Rejecting as step count to following peak ("
                 << steps << ") is outside range " << minDistSteps
                 << " - " << maxDistSteps << endl;
#endif
            continue;
        }
        
        // Establish at least 10 valid pitches in range (3)
        int nValid = 0;
        for (int j = peaks[i] + 1; j < peaks[i+1]; ++j) {
            if (unsmoothedPitch_semis[j] > 0.0) {
                ++nValid;
            }
        }
        if (nValid < minPitchedHops) {
#ifdef DEBUG_PITCH_VIBRATO
            cerr << "-- Rejecting as only " << nValid << " valid pitch "
                 << "measurements found before following peak, at least "
                 << minPitchedHops << " required" << endl;
#endif
            continue;
        }

        // Find the minimum
        double minimum = unsmoothedPitch_semis[peaks[i]];
        for (int j = peaks[i] + 1; j < peaks[i+1]; ++j) {
            if (unsmoothedPitch_semis[j] > 0.0 &&
                unsmoothedPitch_semis[j] < minimum) {
                minimum = unsmoothedPitch_semis[j];
            }
        }

        // Establish pitch criterion (5)
        double range = std::max(unsmoothedPitch_semis[peaks[i]],
                                unsmoothedPitch_semis[peaks[i+1]]) - minimum;
        if (range < minHeight || range > maxHeight) {
#ifdef DEBUG_PITCH_VIBRATO
            cerr << "-- Rejecting as peak height (range) of " << range
                      << " semitones relative to intervening minimum is "
                      << "outside range " << minHeight << " to " << maxHeight
                      << endl;
#endif
            continue;
        }

#ifdef DEBUG_PITCH_VIBRATO
        cerr << "-- Accepting this peak, with range " << range << " semis" << endl;
#endif
        
        VibratoElement element;
        element.hop = peaks[i];
        element.peakIndex = i;
        element.followingHop = peaks[i+1];
        element.range_semis = range;
        // fill in the remaining elements below
        elements.push_back(element);
    }

#ifdef DEBUG_PITCH_VIBRATO
    cerr << "** 3-5. Complete, have " << elements.size() << " peak pairs afterwards" << endl;
#endif
    
    // 6. Use parabolic interpolation to identify a more precise peak
    // location - we already did this, the results are in positions

    for (int i = 0; i < int(elements.size()); ++i) {
        int peakIndex = elements[i].peakIndex;
        elements[i].position_sec = positions[peakIndex];
        if (peakIndex < int(positions.size())) {
            elements[i].waveLength_sec =
                positions[peakIndex+1] - positions[peakIndex];
        } else {
            elements[i].waveLength_sec = 0.0;
        }
    }

    // We now have:
    // 
    // * peaks - all simple local maxima
    // * accepted - indices into peaks of those peaks that meet the
    //   preliminary acceptance criteria listed above
    // * positions - precise times in seconds of the accepted peaks
    // 
    // To determine whether a preliminarily accepted peak will
    // actually be considered part of a vibrato, we model two cycles
    // of a sinusoid synchronised with the minimum before the peak and
    // that two minima ahead of it (so the sinusoid also has a trough
    // somewhere around the intervening minimum). This function and
    // the section of the signal under test are both Hann windowed and
    // their correlation checked.

#ifdef DEBUG_PITCH_VIBRATO
    cerr << "** 7-8. Fit a sinusoidal model and calculate correlation within two Hann-windowed cycles" << endl;
#endif
    
    auto hann = [](int j, int m) {
        return 0.5 - 0.5 * cos(2.0 * M_PI * double(j) / double(m));
    };

    auto model = [](int j, int m) {
        return 0.5 - 0.5 * cos(4.0 * M_PI * double(j) / double(m));
    };
    
    auto windowedModel = [&](int j, int m) {
        return hann(j, m) * model(j, m);
    };

    for (int i = 0; i < int(elements.size()); ++i) {

        int peakIndex = elements[i].peakIndex;

        int peak0 = peaks[peakIndex];
        int peak1 = peaks[peakIndex + 1];
        
        int min0 = peak0;
        while (min0 > 0 &&
               smoothedPitch_semis[min0 - 1] > 0.0 &&
               smoothedPitch_semis[min0 - 1] < smoothedPitch_semis[min0]) {
//            cerr << "at hop " << min0-1 << " we see " << smoothedPitch_semis[min0 - 1] << " (unsmoothed = " << unsmoothedPitch_semis[min0 - 1] << ", " << m_coreFeatures.pitchToHz(unsmoothedPitch_semis[min0 - 1]) << " Hz)" << endl;
            --min0;
        }
        
        int min1 = peak1;
        while (min1 < n-1 &&
               smoothedPitch_semis[min1 + 1] > 0.0 &&
               smoothedPitch_semis[min1 + 1] < smoothedPitch_semis[min1]) {
            ++min1;
        }

        double minInRange = 0.0, maxInRange = 0.0;

        for (int j = min0; j <= min1; ++j) {
            if (smoothedPitch_semis[j] > 0.0 &&
                (minInRange == 0.0 || smoothedPitch_semis[j] < minInRange)) {
                minInRange = smoothedPitch_semis[j];
            }
            if (smoothedPitch_semis[j] > maxInRange) {
                maxInRange = smoothedPitch_semis[j];
            }
        }
        
        int m = min1 - min0;

#ifdef DEBUG_PITCH_VIBRATO
        cerr << "-- For accepted peak at hop " << peaks[peakIndex]
             << " with smoothed pitch " << smoothedPitch_semis[peaks[peakIndex]]
             << " we have:" << endl;
        cerr << "-- Minimum prior to this peak is at hop " << min0;
        if (min0 < 0) {
            cerr << " (synthetic minimum, off start)" << endl;
        } else {
            cerr << " with pitch " << smoothedPitch_semis[min0] << endl;
        }
        cerr << "-- Minimum following the next peak is at hop " << min1;
        if (min1 >= n) {
            cerr << " (synthetic minimum, off end)" << endl;
        } else {
            cerr << " with pitch " << smoothedPitch_semis[min1] << endl;
        }
        cerr << "-- Overall minimum within these two cycles is " << minInRange
             << " and maximum is " << maxInRange << endl;
#endif
        
        if (maxInRange <= minInRange) {
            continue;
        }

        auto normalisedSignal = [&](int j) {
            double x = 0.0;
            if (min0 + j >= 0 && min0 + j < n) {
                x = (smoothedPitch_semis[min0 + j] - minInRange) /
                    (maxInRange - minInRange);
            }
            return x;
        };

        auto windowedSignal = [&](int j, int m) {
            return hann(j, m) * normalisedSignal(j);
        };
        
#ifdef DEBUG_PITCH_VIBRATO
        /*
        cerr << "Normalised Signal,Windowed Signal,Model,Windowed Model" << endl;
        for (int j = 0; j < m; ++j) {
            cerr << normalisedSignal(j) << ","
                 << windowedSignal(j, m) << ","
                 << model(j, m) << ","
                 << windowedModel(j, m)
                 << endl;
        }
        */
#endif

        // The R implementation appears to use Pearson correlation
        
        auto measured = [&](int j) {
            return windowedSignal(j, m);
        };

        auto modelled = [&](int j) {
            return windowedModel(j, m);
        };

        double measuredTotal = 0.0, modelledTotal = 0.0;
        for (int j = 0; j < m; ++j) {
            measuredTotal += measured(j);
            modelledTotal += modelled(j);
        }

        double xmean = measuredTotal / double(m);
        double ymean = modelledTotal / double(m);
        
        double num = 0.0, xdenom = 0.0, ydenom = 0.0;
        for (int j = 0; j < m; ++j) {
            double x = measured(j);
            double y = modelled(j);
            num += (x - xmean) * (y - ymean);
            xdenom += (x - xmean) * (x - xmean);
            ydenom += (y - ymean) * (y - ymean);
        }

        double denom = sqrt(xdenom) * sqrt(ydenom);
        
        double corr = 1.0;
        if (denom != 0.0) {
            corr = num / denom;
        }

#ifdef DEBUG_PITCH_VIBRATO
        cerr << "-- Calculated correlation of " << corr << endl;
#endif

        elements[i].correlation = corr;
    }

#ifdef DEBUG_PITCH_VIBRATO
    cerr << "** 7-8. Complete, returning resulting elements" << endl;
#endif

    return elements;
}

vector<PitchVibrato::VibratoElement>
PitchVibrato::extractElementsSegmented(const vector<double> &pyinPitch_Hz,
                                       const CoreFeatures::OnsetOffsetMap &onsetOffsets,
                                       vector<double> &smoothedPitch_semis,
                                       vector<int> &rawPeaks) const
{
    vector<PitchVibrato::VibratoElement> elements;

    int peakCount = 0;

    smoothedPitch_semis.clear();
    rawPeaks.clear();

    // "In Segmented and Without Glides And Segmented modes, the first
    // 25ms of pitch track after each onset [should] be effectively
    // discarded and would not contribute to the analysis for any
    // note"

    int startClip_steps = m_coreFeatures.msToSteps
        (25.0, m_coreParams.stepSize, false);
    
    for (auto itr = onsetOffsets.begin(); itr != onsetOffsets.end(); ++itr) {

        int onset = itr->first;
        int followingOnset = itr->second.first;

        auto jtr = itr;
        ++jtr;
        if (jtr != onsetOffsets.end()) {
            followingOnset = jtr->first;
        }

        onset += startClip_steps;
        
        if (onset >= followingOnset) {
            continue;
        }
        
        vector<double> notePitches(pyinPitch_Hz.begin() + onset,
                                   pyinPitch_Hz.begin() + followingOnset);

        vector<int> notePeaks;
        vector<double> noteSmoothedPitch;
        
        auto noteElements = extractElements
            (notePitches, noteSmoothedPitch, notePeaks);

        double onsetPosition_sec = 
            m_coreFeatures.stepsToMs(onset, m_coreParams.stepSize) / 1000.0;
        
        for (auto e : noteElements) {
            e.hop += onset;
            e.followingHop += onset;
            e.peakIndex += peakCount;
            e.position_sec += onsetPosition_sec;
            elements.push_back(e);
        }

        for (auto p : notePeaks) {
            rawPeaks.push_back(p + onset);
            ++peakCount;
        }

        while (int(smoothedPitch_semis.size()) < onset) {
            smoothedPitch_semis.push_back(0.0);
        }            

        for (auto p : noteSmoothedPitch) {
            smoothedPitch_semis.push_back(p);
        }
    }

    // Should happen only if no onsets at all were found
    while (smoothedPitch_semis.size() < pyinPitch_Hz.size()) {
        smoothedPitch_semis.push_back(0.0);
    }
    
    return elements;
}

std::vector<double>
PitchVibrato::filterGlides(const std::vector<double> &pyinPitch_Hz,
                           const CoreFeatures::OnsetOffsetMap &onsetOffsets)
    const
{
#ifdef DEBUG_PITCH_VIBRATO
    cerr << "** 0. Identify glides" << endl;
#endif
    
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
    Glide::Extents glides = glide.extract_Hz(pyinPitch_Hz, onsetOffsets);

#ifdef DEBUG_PITCH_VIBRATO
    cerr << "-- Identified " << glides.size() << " glides and "
         << onsetOffsets.size() << " onsets" << endl;
#endif

    vector<double> glideFilteredPitch_Hz = pyinPitch_Hz;
    for (auto g : glides) {
#ifdef DEBUG_PITCH_VIBRATO
        cerr << "-- Removing glide from " << g.second.start << " to "
             << g.second.end << endl;
#endif
        for (auto i = g.second.start; i < g.second.end; ++i) {
            glideFilteredPitch_Hz[i] = 0.0;
        }
    }

#ifdef DEBUG_PITCH_VIBRATO
    cerr << "** 0. Complete" << endl;
#endif

    return glideFilteredPitch_Hz;
}

vector<PitchVibrato::VibratoElement>
PitchVibrato::extractElementsWithoutGlides(const vector<double> &pyinPitch_Hz,
                                           const CoreFeatures::OnsetOffsetMap &onsetOffsets,
                                           vector<double> &smoothedPitch_semis,
                                           vector<int> &rawPeaks) const
{
    auto glideFilteredPitch_Hz = filterGlides(pyinPitch_Hz, onsetOffsets);
    return extractElements(glideFilteredPitch_Hz,
                           smoothedPitch_semis, rawPeaks);
}

vector<PitchVibrato::VibratoElement>
PitchVibrato::extractElementsWithoutGlidesAndSegmented(const vector<double> &pyinPitch_Hz,
                                                       const CoreFeatures::OnsetOffsetMap &onsetOffsets,
                                                       vector<double> &smoothedPitch_semis,
                                                       vector<int> &rawPeaks) const
{
    auto glideFilteredPitch_Hz = filterGlides(pyinPitch_Hz, onsetOffsets);
    return extractElementsSegmented(glideFilteredPitch_Hz, onsetOffsets,
                                    smoothedPitch_semis, rawPeaks);
}

PitchVibrato::VibratoChains
PitchVibrato::groupElementsIntoChains(const vector<VibratoElement> &elements)
    const
{
    PitchVibrato::VibratoChains chains;
    PitchVibrato::VibratoChain currentChain;

#ifdef DEBUG_PITCH_VIBRATO
    cerr << "** Chain: grouping " << elements.size() << " elements into chains"
         << endl;
#endif

    // Elements must be in ascending order of hop - check this
    int prevHop = -1;
    for (const auto &e : elements) {
        if (e.hop <= prevHop) {
            cerr << "PitchVibrato::groupElementsIntoChains: Elements are not in ascending order of hop (" << e.hop << " <= " << prevHop << "), can't continue" << endl;
            return {};
        }
    }
    
    for (auto e : elements) {
        if (e.correlation < m_correlationThreshold) {
#ifdef DEBUG_PITCH_VIBRATO
            cerr << "-- Skipping element from " << e.hop << " to "
                 << e.followingHop << " as correlation " << e.correlation
                 << " is below threshold " << m_correlationThreshold << endl;
#endif
        } else {
#ifdef DEBUG_PITCH_VIBRATO
            cerr << "-- Accepting element from " << e.hop << " to "
                 << e.followingHop << " with correlation " << e.correlation
                 << endl;
#endif
            if (!currentChain.empty() &&
                e.hop != currentChain.rbegin()->followingHop) {
                chains.push_back(currentChain);
                currentChain = {};
            }
            currentChain.push_back(e);
        }
    }

    if (!currentChain.empty()) {
        chains.push_back(currentChain);
    }

#ifdef DEBUG_PITCH_VIBRATO
    cerr << "-- Have " << chains.size() << " chains:" << endl;
    for (const auto &c : chains) {
        cerr << "-- Chain from hop " << c.begin()->hop << ": ";
        for (const auto &e : c) {
            cerr << e.hop << "-" << e.followingHop << " ";
        }
        cerr << endl;
    }
    cerr << "** Chain: complete" << endl;
#endif

    return chains;
}

PitchVibrato::VibratoChain
PitchVibrato::selectBestChainForNote(const VibratoChains &allChains,
                                     int onset, int offset) const
{
    // Select the best vibrato chain (series of contiguous elements)
    // associated with a note. There may be more than one chain
    // intersecting with the duration of this note: we want the one
    // that spans the longest time within the note

    vector<VibratoElement> bestChain;
    int bestChainSpan = -1;

    // Start with the first chain that ends at least at the onset
    auto chainItr = std::lower_bound
        (allChains.begin(), allChains.end(), onset,
         [](const VibratoChain &chain, int onset) {
             return chain.rbegin()->followingHop < onset;
         });

    if (chainItr == allChains.end()) {
#ifdef DEBUG_PITCH_VIBRATO
        cerr << "-- Note onset " << onset
             << " is later than all vibrato chains" << endl;
#endif
        return {};
    }

    for (auto i = chainItr; i != allChains.end(); ++i) {
        const auto &chain = *i;
        int chainStart = chain.begin()->hop;
        if (chainStart >= offset) break;
        int chainEnd = chain.rbegin()->followingHop;
        int span = std::min(chainEnd, offset) - std::max(chainStart, onset);
#ifdef DEBUG_PITCH_VIBRATO
        cerr << "-- Chain from " << chainStart << " to " << chainEnd
             << " spans " << span << " hops within note from " << onset
             << " to " << offset << endl;
#endif
        if (span > bestChainSpan) {
            bestChain = chain;
            bestChainSpan = span;
        }
    }

#ifdef DEBUG_PITCH_VIBRATO
    if (bestChainSpan < 0) {
        cerr << "-- No best chain found for this note" << endl;
    } else {
        cerr << "-- Best chain is from " << bestChain.begin()->hop
             << " to " << bestChain.rbegin()->followingHop << endl;
    }
#endif
    return bestChain;
}

map<int, PitchVibrato::VibratoClassification>
PitchVibrato::classify(const vector<VibratoElement> &elements,
                       const CoreFeatures::OnsetOffsetMap &onsetOffsets) const
{
    map<int, VibratoClassification> classifications;

#ifdef DEBUG_PITCH_VIBRATO
    cerr << "** Classify: considering " << onsetOffsets.size() << " onsets" << endl;
#endif

    VibratoChains allChains = groupElementsIntoChains(elements);
    
    for (auto pitr = onsetOffsets.begin(); pitr != onsetOffsets.end(); ++pitr) {

        // Identify onset, offset, and the following onset
        
        int onset = pitr->first;
        int offset = pitr->second.first;

#ifdef DEBUG_PITCH_VIBRATO
        cerr << "-- Classifying note from " << onset << " to " << offset << endl;
#endif

        VibratoChain chain = selectBestChainForNote(allChains, onset, offset);

        int nelts = chain.size();

#ifdef DEBUG_PITCH_VIBRATO
        cerr << "-- Onset at " << onset << " has chain of " << nelts
             << " associated vibrato elements" << endl;
#endif
        
        if (nelts == 0) {            
            continue;
        }

        VibratoClassification classification;
        
        const VibratoElement &first = chain.at(0);
        const VibratoElement &last = chain.at(nelts - 1);

        double noteStart_ms =
            m_coreFeatures.stepsToMs(onset, m_coreParams.stepSize);
        double noteEnd_ms =
            m_coreFeatures.stepsToMs(offset, m_coreParams.stepSize);

        double vibratoStart_ms = first.position_sec * 1000.0;
        double vibratoEnd_ms = (last.position_sec + last.waveLength_sec) * 1000.0;

        bool nearStart =
            (vibratoStart_ms < noteStart_ms + m_sectionThreshold_ms);
        bool nearEnd =
            (vibratoEnd_ms >= noteEnd_ms - m_sectionThreshold_ms);

        if (nearStart) {
            if (nearEnd) {
                classification.duration = VibratoDuration::Continuous;
            } else {
                classification.duration = VibratoDuration::Onset;
            }
        } else {
            if (nearEnd) {
                classification.duration = VibratoDuration::Offset;
            } else {
                classification.duration = VibratoDuration::Section;
            }
        }

        classification.relativeDuration =
            (vibratoEnd_ms - vibratoStart_ms) / (noteEnd_ms - noteStart_ms);

        classification.soundDuration = (noteEnd_ms - noteStart_ms) / 1000.0;
        
#ifdef DEBUG_PITCH_VIBRATO
        cerr << "-- Onset at " << onset << ": note start (ms) " << noteStart_ms
             << ", end " << noteEnd_ms << "; vibrato start " << vibratoStart_ms
             << ", end " << vibratoEnd_ms << endl;
        cerr << "-> Duration classification: "
             << vibratoDurationToString(classification.duration)
             << endl;
#endif
        
        double meanRate_Hz = 0.0;
        for (auto e : chain) {
            meanRate_Hz += 1.0 / e.waveLength_sec;
        }
        meanRate_Hz /= nelts;

        classification.meanRate = meanRate_Hz;
        
        if (meanRate_Hz > m_rateBoundaryFast_Hz) {
            classification.rate = VibratoRate::Fast;
        } else if (meanRate_Hz > m_rateBoundaryModerate_Hz) {
            classification.rate = VibratoRate::Moderate;
        } else {
            classification.rate = VibratoRate::Slow;
        }

#ifdef DEBUG_PITCH_VIBRATO
        cerr << "-- Onset at " << onset << ": vibrato element rates (Hz):";
        for (auto e : chain) {
            cerr << " " << 1.0 / e.waveLength_sec;
        }
        cerr << ", mean rate " << meanRate_Hz << endl;
        cerr << "-> Rate classification: "
             << vibratoRateToString(classification.rate)
             << endl;
#endif

        double meanRange_cents = 0.0;
        double maxRange_cents = 0.0;
        int maxRangeIndex = 0;
        for (int i = 0; i < int(chain.size()); ++i) {
            double r = 100.0 * chain.at(i).range_semis;
            meanRange_cents += r;
            if (i == 0 || r > maxRange_cents) {
                maxRange_cents = r;
                maxRangeIndex = i;
            }
        }
        meanRange_cents /= nelts;

        classification.maxRange = maxRange_cents;
        
        if (maxRange_cents > m_rangeBoundaryWide_cents) {
            classification.range = VibratoRange::Wide;
        } else if (maxRange_cents > m_rangeBoundaryMedium_cents) {
            classification.range = VibratoRange::Medium;
        } else {
            classification.range = VibratoRange::Narrow;
        }

#ifdef DEBUG_PITCH_VIBRATO
        cerr << "-- Onset at " << onset << ": max range (cents) "
             << maxRange_cents << endl;
        cerr << "-> Range classification: "
             << vibratoRangeToString(classification.range)
             << endl;
#endif

        classification.maxRangeTime = chain[maxRangeIndex].position_sec;
        double maxRangeTime_ms = classification.maxRangeTime * 1000.0;
        
        if (maxRange_cents - meanRange_cents < m_developmentThreshold_cents) {
            classification.development = VibratoDevelopment::Stable;

#ifdef DEBUG_PITCH_VIBRATO
            cerr << "-- Onset at " << onset << ": max range " << maxRange_cents
                 << " is within " << m_developmentThreshold_cents
                 << " of mean range " << meanRange_cents
                 << endl;
            cerr << "-> Development classification: "
                 << developmentToString(classification.development)
                 << endl;
#endif
        
        } else {
            double margin_ms = (vibratoEnd_ms - vibratoStart_ms) / 4.0;
            double early_ms = vibratoStart_ms + margin_ms;
            double late_ms = vibratoEnd_ms - margin_ms;
            if (maxRangeTime_ms > late_ms) {
                classification.development = VibratoDevelopment::Increasing;
            } else if (maxRangeTime_ms < early_ms) {
                classification.development = VibratoDevelopment::Decreasing;
            } else {
                classification.development = VibratoDevelopment::InAndDecreasing;
            }
            
#ifdef DEBUG_PITCH_VIBRATO
            cerr << "-- Onset at " << onset << ": max range " << maxRange_cents
                 << " is at (ms) " << maxRangeTime_ms
                 << " with vibrato start at " << vibratoStart_ms
                 << " and end at " << vibratoEnd_ms
                 << endl;
            cerr << "-> Development classification: "
                 << developmentToString(classification.development)
                 << endl;
#endif
        
        }

        classifications[onset] = classification;
    }

#ifdef DEBUG_PITCH_VIBRATO
    cerr << "** Classify: complete" << endl;
#endif
    
    return classifications;
}

string
PitchVibrato::classificationToCode(const PitchVibrato::VibratoClassification &
                                   classification) const
{
    string code;
    code += vibratoDurationToCode(classification.duration);
    code += vibratoRateToCode(classification.rate);
    code += vibratoRangeToCode(classification.range);
    code += developmentToCode(classification.development);
    return code;
}

double
PitchVibrato::classificationToIndex(const PitchVibrato::VibratoClassification &
                                    classification) const
{
    double index = 1.0;
    index *= vibratoDurationToFactor(classification.duration);
    index *= vibratoRateToFactor(classification.rate);
    index *= vibratoRangeToFactor(classification.range);
    index *= developmentToFactor(classification.development);
    index *= m_scalingFactor;
    return index;
}

PitchVibrato::FeatureSet
PitchVibrato::getRemainingFeatures()
{
    FeatureSet fs;

    m_coreFeatures.finish();

    auto pyinPitch_Hz = m_coreFeatures.getPYinPitch_Hz();
    auto onsetOffsets = m_coreFeatures.getOnsetOffsets();

    vector<int> rawPeaks;
    vector<double> smoothedPitch_semis;
    vector<VibratoElement> elements;

    switch (m_segmentationType) {
    case SegmentationType::Unsegmented:
        elements = extractElements
            (pyinPitch_Hz, smoothedPitch_semis, rawPeaks);
        break;

    case SegmentationType::Segmented:
        elements = extractElementsSegmented
            (pyinPitch_Hz, onsetOffsets, smoothedPitch_semis, rawPeaks);
        break;

    case SegmentationType::WithoutGlides:
        elements = extractElementsWithoutGlides 
           (pyinPitch_Hz, onsetOffsets, smoothedPitch_semis, rawPeaks);
        break;

    case SegmentationType::WithoutGlidesAndSegmented:
        elements = extractElementsWithoutGlidesAndSegmented
           (pyinPitch_Hz, onsetOffsets, smoothedPitch_semis, rawPeaks);
        break;
    }

    int n = int(pyinPitch_Hz.size());
    
    for (int i = 0; i < n; ++i) {
        if (smoothedPitch_semis[i] <= 0.0) continue;
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.timeForStep(i);
        f.values.push_back(m_coreFeatures.pitchToHz(smoothedPitch_semis[i]));
        fs[m_pitchTrackOutput].push_back(f);
    }

    map<int, VibratoClassification> classifications =
        classify(elements, onsetOffsets);
    
    for (auto pitr = onsetOffsets.begin(); pitr != onsetOffsets.end(); ++pitr) {

        int onset = pitr->first;

        int followingOnset = n;
        auto pj = pitr;
        if (++pj != onsetOffsets.end()) {
            followingOnset = pj->first;
        }

        if (classifications.find(onset) == classifications.end()) {
            
#ifdef DEBUG_PITCH_VIBRATO
            cerr << "returning features for onset " << onset
                 << " with no vibrato detected" << endl;
#endif

            Feature f;
            f.hasTimestamp = true;

            string code = "N";
            
            f.timestamp = m_coreFeatures.timeForStep(onset);
            f.hasDuration = false;
            f.label = code;
            fs[m_vibratoTypeOutput].push_back(f);

            f.label = "";
            f.values.clear();
            f.values.push_back(0.f);
            fs[m_vibratoIndexOutput].push_back(f);
        
            ostringstream os;
            os << m_coreFeatures.timeForStep(onset).toText() << " / "
               << (m_coreFeatures.timeForStep(followingOnset) -
                   m_coreFeatures.timeForStep(onset)).toText() << "\n"
               << code << "\n"
               << "IVibr = " << 0.0;
            f.label = os.str();
            f.values.clear();
            fs[m_summaryOutput].push_back(f);
            
        } else {
            
#ifdef DEBUG_PITCH_VIBRATO
            cerr << "returning features for onset " << onset
                 << " with vibrato" << endl;
#endif

            VibratoClassification classification = classifications.at(onset);

            string code = classificationToCode(classification);
            double index = classificationToIndex(classification);
            
            Feature f;
            f.hasTimestamp = true;
            f.timestamp = m_coreFeatures.timeForStep(onset);
            f.hasDuration = false;
            f.label = code;
            fs[m_vibratoTypeOutput].push_back(f);

            f.label = "";
            f.values.clear();
            f.values.push_back(index);
            fs[m_vibratoIndexOutput].push_back(f);

            double clampedRelativeDuration =
                (classification.relativeDuration > 1.0 ?
                 1.0 :
                 classification.relativeDuration);
            
            ostringstream os;
            os << m_coreFeatures.timeForStep(onset).toText() << " / "
               << (m_coreFeatures.timeForStep(followingOnset) -
                   m_coreFeatures.timeForStep(onset)).toText() << "\n"
               << code << "\n"
               << int(round(clampedRelativeDuration * 100.0)) << "%\n"
               << classification.meanRate << "Hz\n"
               << classification.maxRange << "c\n"
               << classification.maxRangeTime << " ("
               << classification.soundDuration << ")\n"
               << "IVibr = " << round(index);
            f.label = os.str();
            f.values.clear();
            fs[m_summaryOutput].push_back(f);
        }
    }        

    for (auto e: elements) {
        if (e.correlation < m_correlationThreshold) {
#ifdef DEBUG_PITCH_VIBRATO
            cerr << "Not reporting element at step " << e.hop
                 << ", as correlation " << e.correlation
                 << " is below threshold " << m_correlationThreshold
                 << endl;
#endif
            continue;
        }
        for (int j = rawPeaks[e.peakIndex]; j < rawPeaks[e.peakIndex + 1]; ++j) {
            if (j < n && pyinPitch_Hz[j] > 0.0) {
                Feature f;
                f.hasTimestamp = true;
                f.timestamp = m_coreFeatures.timeForStep(j);
                f.values.push_back(pyinPitch_Hz[j]);
                fs[m_vibratoPitchTrackOutput].push_back(f);
            }
        }
    }
    
#ifdef WITH_DEBUG_OUTPUTS
    
    for (int i = 0; i < int(rawPeaks.size()); ++i) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.timeForStep(rawPeaks[i]);
        fs[m_rawPeaksOutput].push_back(f);
    }
    
    for (auto e: elements) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.getStartTime() +
            Vamp::RealTime::fromSeconds(e.position_sec);
        f.values.push_back(pyinPitch_Hz[e.hop]);
        fs[m_acceptedPeaksOutput].push_back(f);
    }
    
#endif // WITH_DEBUG_OUTPUTS
    
    return fs;
}

