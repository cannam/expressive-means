
/*
    Expressive Means Portamento

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef EXPRESSIVE_MEANS_PORTAMENTO_H
#define EXPRESSIVE_MEANS_PORTAMENTO_H

#include <vamp-sdk/Plugin.h>

#include "CoreFeatures.h"

#include "Glide.h"

#define WITH_DEBUG_OUTPUTS 1

using std::string;

class Portamento : public Vamp::Plugin
{
public:
    Portamento(float inputSampleRate);
    virtual ~Portamento();

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
    
    enum class GlideDirection {
        Ascending, Descending
    };

    static std::string glideDirectionToString(GlideDirection d) {
        switch (d) {
        case GlideDirection::Ascending: return "Ascending";
        case GlideDirection::Descending: return "Descending";
        default: throw std::logic_error("unknown GlideDirection");
        }
    }

    static std::string glideDirectionToCode(GlideDirection d) {
        switch (d) {
        case GlideDirection::Ascending: return "/";
        case GlideDirection::Descending: return "\\";
        default: throw std::logic_error("unknown GlideDirection");
        }
    }

    static double glideDirectionToFactor(GlideDirection) {
        return 1.0;
    }
    
    enum class GlideLink {
        Targeting, Interconnecting, Starting
    };

    static std::string glideLinkToString(GlideLink d) {
        switch (d) {
        case GlideLink::Targeting: return "Targeting";
        case GlideLink::Interconnecting: return "Interconnecting";
        case GlideLink::Starting: return "Starting";
        default: throw std::logic_error("unknown GlideLink");
        }
    }

    static std::string glideLinkToCode(GlideLink d) {
        switch (d) {
        case GlideLink::Targeting: return "3";
        case GlideLink::Interconnecting: return "2";
        case GlideLink::Starting: return "1";
        default: throw std::logic_error("unknown GlideLink");
        }
    }

    static double glideLinkToFactor(GlideLink d) {
        switch (d) {
        case GlideLink::Targeting: return 0.9;
        case GlideLink::Interconnecting: return 1.0;
        case GlideLink::Starting: return 0.9;
        default: throw std::logic_error("unknown GlideLink");
        }
    }

    enum class GlideRange {
        Small, Medium, Large
    };

    static std::string glideRangeToString(GlideRange d) {
        switch (d) {
        case GlideRange::Small: return "Small";
        case GlideRange::Medium: return "Medium";
        case GlideRange::Large: return "Large";
        default: throw std::logic_error("unknown GlideRange");
        }
    }

    static std::string glideRangeToCode(GlideRange d) {
        switch (d) {
        case GlideRange::Small: return "S";
        case GlideRange::Medium: return "M";
        case GlideRange::Large: return "L";
        default: throw std::logic_error("unknown GlideRange");
        }
    }
    
    enum class GlideDuration {
        Short, Medium, Long
    };

    static std::string glideDurationToString(GlideDuration d) {
        switch (d) {
        case GlideDuration::Short: return "Short";
        case GlideDuration::Medium: return "Medium";
        case GlideDuration::Long: return "Long";
        default: throw std::logic_error("unknown GlideDuration");
        }
    }

    static std::string glideDurationToCode(GlideDuration d) {
        switch (d) {
        case GlideDuration::Short: return "s";
        case GlideDuration::Medium: return "m";
        case GlideDuration::Long: return "l";
        default: throw std::logic_error("unknown GlideDuration");
        }
    }
    
    enum class GlideDynamic {
        Loud, Stable, Quiet
    };

    static std::string glideDynamicToString(GlideDynamic d) {
        switch (d) {
        case GlideDynamic::Loud: return "Loud";
        case GlideDynamic::Stable: return "Stable";
        case GlideDynamic::Quiet: return "Quiet";
        default: throw std::logic_error("unknown GlideDynamic");
        }
    }

    static std::string glideDynamicToCode(GlideDynamic d) {
        switch (d) {
        case GlideDynamic::Loud: return "+";
        case GlideDynamic::Stable: return "=";
        case GlideDynamic::Quiet: return "-";
        default: throw std::logic_error("unknown GlideDynamic");
        }
    }

    static double glideDynamicToFactor(GlideDynamic d) {
        switch (d) {
        case GlideDynamic::Loud: return 1.5;
        case GlideDynamic::Stable: return 1.0;
        case GlideDynamic::Quiet: return 0.5;
        default: throw std::logic_error("unknown GlideDynamic");
        }
    }

    struct GlideClassification {
        GlideDirection direction;
        GlideRange range;
        double range_cents;
        GlideDuration duration;
        double duration_ms;
        GlideLink link;
        GlideDynamic dynamic;
        double dynamicMax;
        double dynamicMin;
    };

    GlideClassification classifyGlide(const std::pair<int, Glide::Extent> &,
                                      const CoreFeatures::OnsetOffsetMap &onsetOffsets,
                                      const std::vector<double> &pyinPitch,
                                      const std::vector<double> &smoothedPower);
    
protected:
    int m_stepSize;
    int m_blockSize;
    
    CoreFeatures m_coreFeatures;

    CoreFeatures::Parameters m_coreParams;
    float m_glideThresholdPitch_cents;  // 3.1, g_1
    float m_glideThresholdHopMinimum_cents;
    float m_glideThresholdHopMaximum_cents;
    float m_glideThresholdDuration_ms;  // 3.2, g_2
    float m_glideThresholdProximity_ms; // 3.3, g_3
    float m_linkThreshold_cents; // b_1
    float m_rangeBoundaryMedium_cents; // c_1.M
    float m_rangeBoundaryLarge_cents; // c_1.L
    float m_durationBoundaryMedium_ms; // d_1.m
    float m_durationBoundaryLong_ms; // d_1.l
    float m_dynamicsThreshold_dB; // e_1
    float m_scalingFactor; // s
    
    mutable int m_summaryOutput;
    mutable int m_portamentoTypeOutput;
    mutable int m_pitchTrackOutput;
    mutable int m_portamentoIndexOutput;
    mutable int m_portamentoPointsOutput;
    mutable int m_glideDirectionOutput;
    mutable int m_glideLinkOutput;
    mutable int m_glideDynamicOutput;
    mutable int m_glidePitchTrackOutput;

    mutable int m_meanRangeOutput;
    mutable int m_meanDurationOutput;
    mutable int m_meanDynamicsOutput;
};

#endif
