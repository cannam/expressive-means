
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

using std::cerr;
using std::endl;
using std::vector;
using std::set;

#define DEBUG_PITCH_VIBRATO 1

PitchVibrato::PitchVibrato(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_stepSize(0),
    m_blockSize(0),
    m_coreFeatures(inputSampleRate),
    m_pitchTrackOutput(-1)
#ifdef WITH_DEBUG_OUTPUTS
    ,
    m_rawPeaksOutput(-1),
    m_acceptedPeaksOutput(-1),
    m_vibratoPitchTrackOutput(-1)
#endif
    /*,
    m_summaryOutput(-1),
    m_pitchvibratoTypeOutput(-1),
    m_pitchvibratoIndexOutput(-1)
    */
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
    
    
    return list;
}

float
PitchVibrato::getParameter(string identifier) const
{
    float value = 0.f;
    if (m_coreParams.obtainVampParameter(identifier, value)) {
        return value;
    }
    
    
    return 0.f;
}

void
PitchVibrato::setParameter(string identifier, float value) 
{
    if (m_coreParams.acceptVampParameter(identifier, value)) {
        return;
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
    
    d.identifier = "pitchTrack";
    d.name = "Pitch Track";
    d.description = "The pitch track computed by pYIN, with further smoothing.";
    d.unit = "Hz";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = (m_inputSampleRate / m_stepSize);
    d.hasDuration = false;
    m_pitchTrackOutput = int(list.size());
    list.push_back(d);

/*!!!    
    d.identifier = "summary";
    d.name = "Summary";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.hasDuration = false;
    m_summaryOutput = int(list.size());
    list.push_back(d);
    
    d.identifier = "pitchvibratoType";
    d.name = "Pitch Vibrato Type";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 0;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.hasDuration = false;
    m_pitchvibratoTypeOutput = int(list.size());
    list.push_back(d);
    
    d.identifier = "pitchvibratoIndex";
    d.name = "Pitch Vibrato Index";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.hasDuration = false;
    m_pitchvibratoIndexOutput = int(list.size());
    list.push_back(d);
*/

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
/*!!!
    if (m_summaryOutput < 0) {
        (void)getOutputDescriptors(); // initialise output indices
    }
*/    
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

    // Number of peak-to-peak ranges found (NB this could be -1 if no
    // peaks at all found!)
    int npairs = int(peaks.size()) - 1;

    // 3. Eliminate those peaks that don't have at least 10 valid
    // pitch values between them and their following peak, and
    //
    // 4. Eliminate those peaks that are not spaced at an interval
    // between 62.5 and 416.7 ms apart (these values are not as
    // precise as they look, judging from the paper), and
    // 
    // 5. Eliminate those peaks that don't rise to within 20-800 cents
    // relative to the intervening minimum

    int minDistSteps = m_coreFeatures.msToSteps
        (62.5, m_coreParams.stepSize, false);
    int maxDistSteps = m_coreFeatures.msToSteps
        (416.7, m_coreParams.stepSize, false);

    vector<VibratoElement> elements;
    
    for (int i = 0; i < npairs; ++i) {

#ifdef DEBUG_PITCH_VIBRATO
        std::cerr << "Considering peak at step " << peaks[i]
                  << " (following peak is at " << peaks[i+1] << ")"
                  << std::endl;
#endif

        // Establish time criterion (4)
        int steps = peaks[i+1] - peaks[i];
        if (steps < minDistSteps || steps > maxDistSteps) {
#ifdef DEBUG_PITCH_VIBRATO
            std::cerr << "Rejecting as step count to following peak ("
                      << steps << ") is outside range " << minDistSteps
                      << " - " << maxDistSteps << std::endl;
#endif
            continue;
        }
        
        // Establish at least 10 valid pitches in range (3)
        int nValid = 0;
        const int minValid = 10;
        for (int j = peaks[i] + 1; j < peaks[i+1]; ++j) {
            if (pitch[j] > 0.0) {
                ++nValid;
            }
        }
        if (nValid < minValid) {
#ifdef DEBUG_PITCH_VIBRATO
            std::cerr << "Rejecting as only " << nValid << " valid pitch "
                      << "measurements found before following peak, at least "
                      << minValid << " required" << std::endl;
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
        double peakHeight =
            std::max(pitch[peaks[i]], pitch[peaks[i+1]]) - minimum;
        const double minHeight = 0.2, maxHeight = 8.0; // semitones
        if (peakHeight < minHeight || peakHeight > maxHeight) {
#ifdef DEBUG_PITCH_VIBRATO
            std::cerr << "Rejecting as peak height of " << peakHeight
                      << " semitones relative to intervening minimum is "
                      << "outside range " << minHeight << " to " << maxHeight
                      << std::endl;
#endif
            continue;
        }

        VibratoElement element;
        element.hop = peaks[i];
        element.peakIndex = i;
        element.peakHeight = peakHeight;
        // fill in the remaining elements below
        elements.push_back(element);
    }

    // 6. Use parabolic interpolation to identify a more precise peak
    // location

    for (int i = 0; i < int(elements.size()); ++i) {
        int hop = elements[i].hop;
        double alpha = pitch[hop - 1];
        double beta = pitch[hop];
        double gamma = pitch[hop + 1];
        double denom = alpha - 2.0 * beta + gamma;
        double p = (denom != 0.0 ? ((alpha - gamma) / denom * 0.5) : 0.0);
        double refinedStep = double(hop) + p;
        double sec =
            ((refinedStep +
              double((m_coreParams.blockSize / m_coreParams.stepSize) / 2)) *
             double(m_coreParams.stepSize))
            / double(m_inputSampleRate);
        elements[i].position = sec;
    }

    for (int i = 0; i + 1 < int(elements.size()); ++i) {
        elements[i].waveLength = elements[i+1].position - elements[i].position;
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
            
        std::cerr << "for accepted peak at " << peaks[peakIndex]
                  << " with pitch " << pitch[peaks[peakIndex]]
                  << " we have previous minimum " << pitch[min0]
                  << " at " << min0 << " and following-following minimum "
                  << pitch[min1] << " at " << min1
                  << "; overall minimum is " << minInRange
                  << " and maximum is " << maxInRange << std::endl;

        if (maxInRange <= minInRange) {
            continue;
        }

#ifdef DEBUG_PITCH_VIBRATO
        std::cerr << "window of signal (" << m << "): ";
        for (int j = 0; j < m; ++j) {
            double signal = (pitch[min0 + j] - minInRange) /
                (maxInRange - minInRange);
            double winSignal = signal * hann(j, m);
            std::cerr << winSignal << " ";
        }
        std::cerr << std::endl;
            
        std::cerr << "window of modelled sinusoid (" << m << "): ";
        for (int j = 0; j < m; ++j) {
            std::cerr << winSine(j, m) << " ";
        }
        std::cerr << std::endl;
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
        std::cerr << "correlation = " << corr << std::endl;
#endif

        elements[i].correlation = corr;
    }

    const double correlationThreshold = 0.75;

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
            Vamp::RealTime::fromSeconds(e.position);
        f.values.push_back(m_coreFeatures.pitchToHz(pitch[e.hop]));
        fs[m_acceptedPeaksOutput].push_back(f);
    }

    for (auto e: elements) {
        if (e.correlation < correlationThreshold) {
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
    
#endif
    
    return fs;
}

