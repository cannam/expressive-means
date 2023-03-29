
/*
    Expressive Means Pitch Vibrato

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "PitchVibrato.h"

#include <vector>
#include <set>
#include <sstream>

using std::cerr;
using std::endl;
using std::vector;
using std::set;
using std::map;
using std::ostringstream;

#define DEBUG_PITCH_VIBRATO 1

static const float default_vibratoRateMinimum_Hz = 3.f;
static const float default_vibratoRateMaximum_Hz = 12.f;
static const float default_vibratoRangeMinimum_cents = 10.f;
static const float default_vibratoRangeMaximum_cents = 500.f;
static const float default_rateBoundaryModerate_Hz = 6.2f;
static const float default_rateBoundaryFast_Hz = 7.2f;
static const float default_rangeBoundaryMedium_cents = 40.f;
static const float default_rangeBoundaryWide_cents = 60.f;
static const float default_sectionThreshold_ms = 200.f;
static const float default_developmentThreshold_cents = 10.f;
static const float default_correlationThreshold = 0.75f;
static const float default_scalingFactor = 11.1f;

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
    m_summaryOutput(-1),
    m_pitchTrackOutput(-1),
    m_vibratoTypeOutput(-1),
    m_vibratoIndexOutput(-1)
#ifdef WITH_DEBUG_OUTPUTS
    ,
    m_rawPeaksOutput(-1),
    m_acceptedPeaksOutput(-1),
    m_vibratoPitchTrackOutput(-1)
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
    return "Expressive Means: Pitch Vibrato";
}

string
PitchVibrato::getDescription() const
{
    return "";
}

string
PitchVibrato::getMaker() const
{
    return "Frithjof Vollmer and Chris Cannam";
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

    m_coreParams.appendVampParameterDescriptors(list);
    
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
    d.minValue = 0.3f;
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
    
    d.identifier = "pitchTrack";
    d.name = "Pitch Track";
    d.description = "The pitch track computed by pYIN, with further smoothing.";
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
    
    d.identifier = "vibratoPitchTrack";
    d.name = "[Debug] Vibrato-Only Pitch Track";
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

PitchVibrato::FeatureSet
PitchVibrato::getRemainingFeatures()
{
    FeatureSet fs;

    m_coreFeatures.finish();

    auto pyinPitch = m_coreFeatures.getPYinPitch_Hz();

    // The numbered comments correspond to the numbered steps in Tilo
    // Haehnel's paper

    // 1. Convert pitch track from Hz to cents and smooth with a 35ms
    // (either side? so 70ms total? or 35ms total?) mean filter
    
    auto unfilteredPitch = m_coreFeatures.getPitch_semis();
    int n = unfilteredPitch.size();

    MeanFilter meanFilter(m_coreFeatures.msToSteps
                          (35.f, m_coreParams.stepSize, true));
    vector<double> pitch(n, 0.0);
    meanFilter.filter(unfilteredPitch.data(), pitch.data(), n);

    // (Reinstate unvoiced pitches)
    for (int i = 0; i < n; ++i) {
        if (pyinPitch[i] <= 0) {
            pitch[i] = 0.0;
        }
    }

    for (int i = 0; i < n; ++i) {
        if (pitch[i] == 0.0) continue;
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.timeForStep(i);
        f.values.push_back(m_coreFeatures.pitchToHz(pitch[i]));
        fs[m_pitchTrackOutput].push_back(f);
    }

    // 2. Identify peaks - simply local maxima, with values greater
    // then their immediate neighbours (where they all have valid
    // pitch values)
    
    vector<int> peaks;
    for (int i = 1; i + 1 < n; ++i) {
        if (pitch[i] >= pitch[i-1] && pitch[i] > pitch[i+1]) {
            if (pitch[i] > 0.0 && pitch[i-1] > 0.0 && pitch[i+1] > 0.0) {
                peaks.push_back(i);
            }
        }
    }

    // Use parabolic interpolation to identify a more precise peak
    // location. This is step 6 in the paper, but we'll do it now
    // because we want to ensure the location of the "following" peak
    // for every accepted peak is available even if the following peak
    // is itself not accepted.

    vector<double> positions; // positions[i] is time in sec of peaks[i]
    
    for (int i = 0; i < int(peaks.size()); ++i) {
        int hop = peaks[i];
        double refinedStep = hop;
        if (hop >= 1 && hop + 1 < n) {
            double alpha = pitch[hop - 1];
            double beta = pitch[hop];
            double gamma = pitch[hop + 1];
            double denom = alpha - 2.0 * beta + gamma;
            double p = (denom != 0.0 ? ((alpha - gamma) / denom * 0.5) : 0.0);
            refinedStep += p;
        }
        double sec =
            ((refinedStep +
              double((m_coreParams.blockSize / m_coreParams.stepSize) / 2)) *
             double(m_coreParams.stepSize))
            / double(m_inputSampleRate);
        positions.push_back(sec);
    }
    
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
        cerr << "Considering peak at step " << peaks[i]
             << " (following peak is at " << peaks[i+1] << ")"
             << endl;
#endif

        // Establish time criterion (4)
        int steps = peaks[i+1] - peaks[i];
        if (steps < minDistSteps || steps > maxDistSteps) {
#ifdef DEBUG_PITCH_VIBRATO
            cerr << "Rejecting as step count to following peak ("
                 << steps << ") is outside range " << minDistSteps
                 << " - " << maxDistSteps << endl;
#endif
            continue;
        }
        
        // Establish at least 10 valid pitches in range (3)
        int nValid = 0;
        for (int j = peaks[i] + 1; j < peaks[i+1]; ++j) {
            if (pitch[j] > 0.0) {
                ++nValid;
            }
        }
        if (nValid < minPitchedHops) {
#ifdef DEBUG_PITCH_VIBRATO
            cerr << "Rejecting as only " << nValid << " valid pitch "
                 << "measurements found before following peak, at least "
                 << minPitchedHops << " required" << endl;
#endif
            continue;
        }

        // Find the minimum
        double minimum = pitch[peaks[i]];
        for (int j = peaks[i] + 1; j < peaks[i+1]; ++j) {
            if (pitch[j] > 0.0 && pitch[j] < minimum) {
                minimum = pitch[j];
            }
        }

        // Establish pitch criterion (5)
        double range =
            std::max(pitch[peaks[i]], pitch[peaks[i+1]]) - minimum;
        if (range < minHeight || range > maxHeight) {
#ifdef DEBUG_PITCH_VIBRATO
            cerr << "Rejecting as peak height (range) of " << range
                      << " semitones relative to intervening minimum is "
                      << "outside range " << minHeight << " to " << maxHeight
                      << endl;
#endif
            continue;
        }

        VibratoElement element;
        element.hop = peaks[i];
        element.peakIndex = i;
        element.range_semis = range;
        // fill in the remaining elements below
        elements.push_back(element);
    }

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

    auto hann = [](int i, int n) {
        return 0.5 - 0.5 * cos(2.0 * M_PI * double(i) / double(n));
    };

    auto winSine = [&](int i, int n) {
        return hann(i, n) *
            (0.5 - 0.5 * cos(4.0 * M_PI * double(i) / double(n)));
    };

    for (int i = 0; i < int(elements.size()); ++i) {

        int peakIndex = elements[i].peakIndex;
        
        int min0 = peaks[peakIndex];
        while (min0 > 0 && pitch[min0 - 1] < pitch[min0]) {
            --min0;
        }
        
        int min1 = peaks[peakIndex + 1];
        while (min1 < n-1 && pitch[min1 + 1] < pitch[min1]) {
            ++min1;
        }

        int m = min1 - min0;

        double minInRange = 0.0, maxInRange = 0.0;

        for (int j = 0; j < m; ++j) {
            if (pitch[min0 + j] > 0.0 &&
                (minInRange == 0.0 || pitch[min0 + j] < minInRange)) {
                minInRange = pitch[min0 + j];
            }
            if (pitch[min0 + j] > maxInRange) {
                maxInRange = pitch[min0 + j];
            }
        }
            
        cerr << "for accepted peak at " << peaks[peakIndex]
             << " with pitch " << pitch[peaks[peakIndex]]
             << " we have previous minimum " << pitch[min0]
             << " at " << min0 << " and following-following minimum "
             << pitch[min1] << " at " << min1
             << "; overall minimum is " << minInRange
             << " and maximum is " << maxInRange << endl;
        
        if (maxInRange <= minInRange) {
            continue;
        }

#ifdef DEBUG_PITCH_VIBRATO
        cerr << "window of signal (" << m << "): ";
        for (int j = 0; j < m; ++j) {
            double signal = (pitch[min0 + j] - minInRange) /
                (maxInRange - minInRange);
            double winSignal = signal * hann(j, m);
            cerr << winSignal << " ";
        }
        cerr << endl;
            
        cerr << "window of modelled sinusoid (" << m << "): ";
        for (int j = 0; j < m; ++j) {
            cerr << winSine(j, m) << " ";
        }
        cerr << endl;
#endif

        // The R implementation appears to use Pearson correlation
        
        auto measured = [&](int i) {
            double s = (pitch[min0 + i] - minInRange) /
                (maxInRange - minInRange);
            return s * hann(i, m);
        };

        auto modelled = [&](int i) {
            return winSine(i, m);
        };

        double measuredTotal = 0.0, modelledTotal = 0.0;
        for (int i = 0; i < m; ++i) {
            measuredTotal += measured(i);
            modelledTotal += modelled(i);
        }

        double xmean = measuredTotal / double(m);
        double ymean = modelledTotal / double(m);
        
        double num = 0.0, xdenom = 0.0, ydenom = 0.0;
        for (int i = 0; i < m; ++i) {
            double x = measured(i);
            double y = modelled(i);
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
        cerr << "correlation = " << corr << endl;
#endif

        elements[i].correlation = corr;
    }

    auto eitr = elements.begin();
    auto onsetOffsets = m_coreFeatures.getOnsetOffsets();

    map<int, VibratoClassification> classifications; // key is onset hop
    
    for (auto pitr = onsetOffsets.begin(); pitr != onsetOffsets.end(); ++pitr) {

        int onset = pitr->first;
        int offset = pitr->second.first;

        int followingOnset = onset;
        auto pj = pitr;
        if (++pj != onsetOffsets.end()) {
            followingOnset = pj->first;
        }
        
        vector<VibratoElement> ee; // elements associated with this onset
        while (eitr != elements.end() &&
               (followingOnset == onset || eitr->hop < followingOnset)) {
            if (eitr->correlation >= m_correlationThreshold) {
                ee.push_back(*eitr);
            }
            ++eitr;
        }

        int nelts = ee.size();

#ifdef DEBUG_PITCH_VIBRATO
        cerr << "onset at " << onset << ": " << nelts
             << " associated vibrato elements above correlation threshold"
             << endl;
#endif
        
        if (nelts == 0) {            
            continue;
        }

        VibratoClassification classification;
        
        const VibratoElement &first = ee.at(0);
        const VibratoElement &last = ee.at(nelts - 1);

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

#ifdef DEBUG_PITCH_VIBRATO
        cerr << "onset at " << onset << ": note start (ms) " << noteStart_ms
             << ", end " << noteEnd_ms << "; vibrato start " << vibratoStart_ms
             << ", end " << vibratoEnd_ms << ": duration classification: "
             << vibratoDurationToString(classification.duration)
             << endl;
#endif
        
        double meanRate_Hz = 0.0;
        for (auto e : ee) {
            meanRate_Hz += 1.0 / e.waveLength_sec;
        }
        meanRate_Hz /= nelts;

        if (meanRate_Hz > m_rateBoundaryFast_Hz) {
            classification.rate = VibratoRate::Fast;
        } else if (meanRate_Hz > m_rateBoundaryModerate_Hz) {
            classification.rate = VibratoRate::Moderate;
        } else {
            classification.rate = VibratoRate::Slow;
        }

#ifdef DEBUG_PITCH_VIBRATO
        cerr << "onset at " << onset << ": mean vibrato rate " << meanRate_Hz
             << ": rate classification: "
             << vibratoRateToString(classification.rate)
             << endl;
#endif

        double meanRange_cents = 0.0;
        double maxRange_cents = 0.0;
        int maxRangeIndex = 0;
        for (int i = 0; i < int(ee.size()); ++i) {
            double r = 100.0 * ee.at(i).range_semis;
            meanRange_cents += r;
            if (i == 0 || r > maxRange_cents) {
                maxRange_cents = r;
                maxRangeIndex = i;
            }
        }
        meanRange_cents /= nelts;

        if (maxRange_cents > m_rangeBoundaryWide_cents) {
            classification.range = VibratoRange::Wide;
        } else if (maxRange_cents > m_rangeBoundaryMedium_cents) {
            classification.range = VibratoRange::Medium;
        } else {
            classification.range = VibratoRange::Narrow;
        }

#ifdef DEBUG_PITCH_VIBRATO
        cerr << "onset at " << onset << ": max range " << maxRange_cents
             << ": range classification: "
             << vibratoRangeToString(classification.range)
             << endl;
#endif
        
        if (maxRange_cents - meanRange_cents < m_developmentThreshold_cents) {
            classification.development = VibratoDevelopment::Stable;

#ifdef DEBUG_PITCH_VIBRATO
            cerr << "onset at " << onset << ": max range " << maxRange_cents
                 << " is within " << m_developmentThreshold_cents
                 << " of mean range " << meanRange_cents
                 << ": development classification: "
                 << developmentToString(classification.development)
                 << endl;
#endif
        
        } else {
            double maxRangeTime_ms = ee[maxRangeIndex].position_sec * 1000.0;
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
            cerr << "onset at " << onset << ": max range " << maxRange_cents
                 << " is at (ms) " << maxRangeTime_ms
                 << " with vibrato start at " << vibratoStart_ms
                 << " and end at " << vibratoEnd_ms
                 << ": development classification: "
                 << developmentToString(classification.development)
                 << endl;
#endif
        
        }
    }
    
    for (auto pitr = onsetOffsets.begin(); pitr != onsetOffsets.end(); ++pitr) {

        int onset = pitr->first;

        int followingOnset = onset;
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

            
        }
    }        
        
    
#ifdef WITH_DEBUG_OUTPUTS
    
    for (int i = 0; i < int(peaks.size()); ++i) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.timeForStep(peaks[i]);
        fs[m_rawPeaksOutput].push_back(f);
    }
    
    for (auto e: elements) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.getStartTime() +
            Vamp::RealTime::fromSeconds(e.position_sec);
        f.values.push_back(m_coreFeatures.pitchToHz(pitch[e.hop]));
        fs[m_acceptedPeaksOutput].push_back(f);
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
        for (int j = peaks[e.peakIndex]; j < peaks[e.peakIndex + 1]; ++j) {
            if (j < n && pitch[j] > 0.0) {
                Feature f;
                f.hasTimestamp = true;
                f.timestamp = m_coreFeatures.timeForStep(j);
                f.values.push_back(m_coreFeatures.pitchToHz(pitch[j]));
                fs[m_vibratoPitchTrackOutput].push_back(f);
            }
        }
    }
    
#endif // WITH_DEBUG_OUTPUTS
    
    return fs;
}

