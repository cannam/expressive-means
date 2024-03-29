/*
    Expressive Means

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef EXPRESSIVE_MEANS_GLIDE_H
#define EXPRESSIVE_MEANS_GLIDE_H

#include "CoreFeatures.h"

#include <map>
#include <vector>

class Glide
{
public:
    struct Parameters {
        int durationThreshold_steps;
        int onsetProximityThreshold_steps;
        float minimumPitchThreshold_cents;
        float minimumHopDifference_cents;
        float maximumHopDifference_cents;
        int medianFilterLength_steps;
        bool useSmoothing;

        Parameters() :
            durationThreshold_steps(10),
            onsetProximityThreshold_steps(100),
            minimumPitchThreshold_cents(60.f),
            minimumHopDifference_cents(15.f),
            maximumHopDifference_cents(50.f),
            medianFilterLength_steps(29),
            useSmoothing(false)
        {}
    };

    Glide(Parameters parameters) :
        m_parameters(parameters) { }
    
    struct Extent {
        int start;  // first hop within the glide
        int end;    // last hop within the glide
    };

    typedef std::map<int, Extent> Extents; // onset step -> glide extent

    /**
     * Identify and return glide extents from the given pitch track
     * and onset/offsets. pitch_Hz is as returned by
     * CoreFeatures::getPYinPitch_Hz() (with unvoiced steps indicated
     * using zero or negative values) and the onset/offset map is as
     * returned by CoreFeatures::getOnsetOffsets().
     */     
    Extents extract_Hz(const std::vector<double> &pitch_Hz,
                       const CoreFeatures::OnsetOffsetMap &onsetOffsets);

    /**
     * Identify and return glide extents from the given pitch track
     * and onset/offsets. pitch_semis is as returned by
     * CoreFeatures::getPYinPitch_Hz() (with unvoiced steps indicated
     * using zero or negative values) and the onset/offset map is as
     * returned by CoreFeatures::getOnsetOffsets().
     */     
    Extents extract_semis(const std::vector<double> &pitch_semis,
                          const CoreFeatures::OnsetOffsetMap &onsetOffsets);

private:
    Parameters m_parameters;
};

#endif
