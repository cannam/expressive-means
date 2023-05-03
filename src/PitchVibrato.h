
/*
    Expressive Means Pitch Vibrato

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef EXPRESSIVE_MEANS_PITCHVIBRATO_H
#define EXPRESSIVE_MEANS_PITCHVIBRATO_H

#include <vamp-sdk/Plugin.h>

#include "CoreFeatures.h"

using std::string;

#define WITH_DEBUG_OUTPUTS 1

class PitchVibrato : public Vamp::Plugin
{
public:
    PitchVibrato(float inputSampleRate);
    virtual ~PitchVibrato();

    string getIdentifier() const;
    string getName() const;
    string getDescription() const;
    string getMaker() const;
    int getPluginVersion() const;
    string getCopyright() const;

    InputDomain getInputDomain() const;
    size_t getPreferredBlockSize() const;
    size_t getPreferredStepSize() const;
    size_t getMinChannelCount() const;
    size_t getMaxChannelCount() const;

    ParameterList getParameterDescriptors() const;
    float getParameter(string identifier) const;
    void setParameter(string identifier, float value);

    ProgramList getPrograms() const;
    string getCurrentProgram() const;
    void selectProgram(string name);

    OutputList getOutputDescriptors() const;

    bool initialise(size_t channels, size_t stepSize, size_t blockSize);
    void reset();

    FeatureSet process(const float *const *inputBuffers,
                       Vamp::RealTime timestamp);

    FeatureSet getRemainingFeatures();

    struct VibratoElement {
        int hop;
        int peakIndex;
        double range_semis; // min-to-max, in semitones
        double position_sec; // interpolated
        double waveLength_sec; // time to the following element's position
        double correlation;
        VibratoElement() :
            hop(-1), peakIndex(-1),
            range_semis(0.0), position_sec(0.0),
            waveLength_sec(0.0), correlation(0.0) { }
    };

    enum class VibratoDuration {
        Continuous, Onset, Offset, Section
    };

    static std::string vibratoDurationToString(VibratoDuration d) {
        switch (d) {
        case VibratoDuration::Continuous: return "Continuous";
        case VibratoDuration::Onset: return "Onset";
        case VibratoDuration::Offset: return "Offset";
        case VibratoDuration::Section: return "Section";
        default: throw std::logic_error("unknown VibratoDuration");
        }
    }

    static std::string vibratoDurationToCode(VibratoDuration d) {
        switch (d) {
        case VibratoDuration::Continuous: return "4";
        case VibratoDuration::Onset: return "3";
        case VibratoDuration::Offset: return "2";
        case VibratoDuration::Section: return "1";
        default: throw std::logic_error("unknown VibratoDuration");
        }
    }
    
    static double vibratoDurationToFactor(VibratoDuration d) {
        switch (d) {
        case VibratoDuration::Continuous: return 1.0;
        case VibratoDuration::Onset: return 0.8;
        case VibratoDuration::Offset: return 0.8;
        case VibratoDuration::Section: return 0.6;
        default: throw std::logic_error("unknown VibratoDuration");
        }
    }

    enum class VibratoRate {
        Slow, Moderate, Fast
    };

    static std::string vibratoRateToString(VibratoRate d) {
        switch (d) {
        case VibratoRate::Slow: return "Slow";
        case VibratoRate::Moderate: return "Moderate";
        case VibratoRate::Fast: return "Fast";
        default: throw std::logic_error("unknown VibratoRate");
        }
    }

    static std::string vibratoRateToCode(VibratoRate d) {
        switch (d) {
        case VibratoRate::Slow: return "S";
        case VibratoRate::Moderate: return "M";
        case VibratoRate::Fast: return "F";
        default: throw std::logic_error("unknown VibratoRate");
        }
    }
    
    static double vibratoRateToFactor(VibratoRate d) {
        switch (d) {
        case VibratoRate::Slow: return 1.0;
        case VibratoRate::Moderate: return 2.0;
        case VibratoRate::Fast: return 3.0;
        default: throw std::logic_error("unknown VibratoRate");
        }
    }

    enum class VibratoRange {
        Narrow, Medium, Wide
    };

    static std::string vibratoRangeToString(VibratoRange d) {
        switch (d) {
        case VibratoRange::Narrow: return "Narrow";
        case VibratoRange::Medium: return "Medium";
        case VibratoRange::Wide: return "Wide";
        default: throw std::logic_error("unknown VibratoRange");
        }
    }

    static std::string vibratoRangeToCode(VibratoRange d) {
        switch (d) {
        case VibratoRange::Narrow: return "n";
        case VibratoRange::Medium: return "m";
        case VibratoRange::Wide: return "w";
        default: throw std::logic_error("unknown VibratoRange");
        }
    }
    
    static double vibratoRangeToFactor(VibratoRange d) {
        switch (d) {
        case VibratoRange::Narrow: return 1.0;
        case VibratoRange::Medium: return 2.0;
        case VibratoRange::Wide: return 3.0;
        default: throw std::logic_error("unknown VibratoRange");
        }
    }

    enum class VibratoDevelopment {
        Decreasing, DeAndIncreasing, Stable, InAndDecreasing, Increasing
    };
    
    static std::string developmentToString(VibratoDevelopment d) {
        switch (d) {
        case VibratoDevelopment::Decreasing: return "Decreasing";
        case VibratoDevelopment::DeAndIncreasing: return "De-and-Increasing";
        case VibratoDevelopment::Stable: return "Stable";
        case VibratoDevelopment::InAndDecreasing: return "In-And-Decreasing";
        case VibratoDevelopment::Increasing: return "Increasing";
        default: throw std::logic_error("unknown VibratoDevelopment");
        }
    }

    static std::string developmentToCode(VibratoDevelopment d) {
        switch (d) {
        case VibratoDevelopment::Decreasing:      return ">";
        case VibratoDevelopment::DeAndIncreasing: return ":";
        case VibratoDevelopment::Stable:          return "=";
        case VibratoDevelopment::InAndDecreasing: return ":";
        case VibratoDevelopment::Increasing:      return "<";
        default: throw std::logic_error("unknown VibratoDevelopment");
        }
    }

    static double developmentToFactor(VibratoDevelopment d) {
        switch (d) {
        case VibratoDevelopment::Decreasing:      return 0.9;
        case VibratoDevelopment::DeAndIncreasing: return 0.8;
        case VibratoDevelopment::Stable:          return 1.0;
        case VibratoDevelopment::InAndDecreasing: return 0.8;
        case VibratoDevelopment::Increasing:      return 0.9;
        default: throw std::logic_error("unknown VibratoDevelopment");
        }
    }

    struct VibratoClassification {
        VibratoDuration duration;
        double relativeDuration;
        double soundDuration;
        VibratoRate rate;
        double meanRate;
        VibratoRange range;
        double maxRange;
        double maxRangeTime;
        VibratoDevelopment development;
    };

    std::vector<VibratoElement> extractElements
    (const std::vector<double> &pyinPitch_Hz,  // in
     std::vector<double> &smoothedPitch_semis, // out
     std::vector<int> &rawPeaks) const;        // out

    std::vector<VibratoElement> extractElementsSegmented
    (const std::vector<double> &pyinPitch_Hz,  // in
     const CoreFeatures::OnsetOffsetMap &onsetOffsets, // in
     std::vector<double> &smoothedPitch_semis, // out
     std::vector<int> &rawPeaks) const;        // out

    std::vector<VibratoElement> extractElementsWithoutGlides
    (const std::vector<double> &pyinPitch_Hz,  // in
     const CoreFeatures::OnsetOffsetMap &onsetOffsets, // in
     std::vector<double> &smoothedPitch_semis, // out
     std::vector<int> &rawPeaks) const;        // out

    std::vector<VibratoElement> extractElementsWithoutGlidesAndSegmented
    (const std::vector<double> &pyinPitch_Hz,  // in
     const CoreFeatures::OnsetOffsetMap &onsetOffsets, // in
     std::vector<double> &smoothedPitch_semis, // out
     std::vector<int> &rawPeaks) const;        // out

    std::vector<VibratoElement> extractElementsFlattened
    (const std::vector<double> &pyinPitch_Hz,  // in
     const CoreFeatures::OnsetOffsetMap &onsetOffsets, // in
     std::vector<double> &smoothedPitch_semis, // out
     std::vector<int> &rawPeaks) const;        // out

    std::map<int, VibratoClassification> classify
    (const std::vector<VibratoElement> &elements,
     const CoreFeatures::OnsetOffsetMap &onsetOffsets) const;

    std::string classificationToCode(const VibratoClassification &) const;
    double classificationToIndex(const VibratoClassification &) const;

    enum class SegmentationType {
        Unsegmented,
        Segmented,
        WithoutGlides
    };

protected:
    int m_stepSize;
    int m_blockSize;
    
    CoreFeatures m_coreFeatures;

    CoreFeatures::Parameters m_coreParams;
    float m_vibratoRateMinimum_Hz;
    float m_vibratoRateMaximum_Hz;
    float m_vibratoRangeMinimum_cents;
    float m_vibratoRangeMaximum_cents;
    float m_rateBoundaryModerate_Hz;
    float m_rateBoundaryFast_Hz;
    float m_rangeBoundaryMedium_cents;
    float m_rangeBoundaryWide_cents;
    float m_sectionThreshold_ms;
    float m_developmentThreshold_cents;
    float m_correlationThreshold;
    float m_scalingFactor;

    float m_smoothingWindowLength_ms;

    SegmentationType m_segmentationType;
    
    mutable int m_summaryOutput;
    mutable int m_pitchTrackOutput;
    mutable int m_vibratoTypeOutput;
    mutable int m_vibratoIndexOutput;

#ifdef WITH_DEBUG_OUTPUTS
    mutable int m_rawPeaksOutput;
    mutable int m_acceptedPeaksOutput;
    mutable int m_vibratoPitchTrackOutput;
#endif

};

#endif
