
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

PitchVibrato::PitchVibrato(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_stepSize(0),
    m_blockSize(0),
    m_coreFeatures(inputSampleRate),
    m_pitchTrackOutput(-1),
    m_peaksOutput(-1)
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
    d.unit = "semitones";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = (m_inputSampleRate / m_stepSize);
    d.hasDuration = false;
    m_pitchTrackOutput = int(list.size());
    list.push_back(d);
    
    d.identifier = "peaks";
    d.name = "Peaks";
    d.description = "Locations and pitches of vibrato peaks.";
    d.unit = "semitones";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.sampleRate = 0.f;
    d.hasDuration = false;
    m_peaksOutput = int(list.size());
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
    // (either side? so 70ms total) mean filter

    auto unfilteredPitch = m_coreFeatures.getPitch_semis();
    int n = unfilteredPitch.size();

    MeanFilter meanFilter(m_coreFeatures.msToSteps
                          (70.f, m_coreParams.stepSize, true));
    vector<double> pitch(n, 0.0);
    meanFilter.filter(unfilteredPitch.data(), pitch.data(), n);
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
        f.values.push_back(pitch[i]);
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

    vector<int> accepted; // NB this contains indices into peaks, so
                          // the *pitch* of accepted[i] is
                          // pitch[peaks[accepted[i]]]

    int minDistSteps = m_coreFeatures.msToSteps
        (62.5, m_coreParams.stepSize, false);
    int maxDistSteps = m_coreFeatures.msToSteps
        (416.7, m_coreParams.stepSize, false);
    
    for (int i = 0; i < npairs; ++i) {

        // Establish time criterion (4)
        int steps = peaks[i+1] - peaks[i];
        if (steps < minDistSteps || steps > maxDistSteps) {
            continue;
        }
        
        // Establish at least 10 valid pitches in range (3)
        int nvalid = 0;
        for (int j = peaks[i] + 1; j < peaks[i+1]; ++j) {
            if (pitch[j] > 0.0) {
                ++nvalid;
            }
        }
        if (nvalid < 10) {
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
        if (peakHeight < 0.2 || peakHeight > 8.0) { // semitones
            continue;
        }

        accepted.push_back(i);
    }

    // 6. Use parabolic interpolation to identify a more precise peak
    // location

    vector<double> positions; // positions[i] is time in seconds of
                              // accepted[i], relative to start time
    
    for (int i = 0; i < int(accepted.size()); ++i) {
        double alpha = pitch[peaks[accepted[i]] - 1];
        double beta = pitch[peaks[accepted[i]]];
        double gamma = pitch[peaks[accepted[i]] + 1];
        double denom = alpha - 2.0 * beta + gamma;
        double p = (denom != 0.0 ? ((alpha - gamma) / denom * 0.5) : 0.0);
        double refinedStep = double(peaks[accepted[i]]) + p;
        double sec =
            ((refinedStep +
              double((m_coreParams.blockSize / m_coreParams.stepSize) / 2)) *
             double(m_coreParams.stepSize))
            / double(m_inputSampleRate);
        positions.push_back(sec);
    }

    for (int i = 0; i < int(accepted.size()); ++i) {
        Feature f;
        f.hasTimestamp = true;
        f.timestamp = m_coreFeatures.getStartTime() +
            Vamp::RealTime::fromSeconds(positions[i]);
        f.values.push_back(pitch[peaks[accepted[i]]]);
        fs[m_peaksOutput].push_back(f);
    }

    return fs;
}

