
/*
    Expressive Means

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Glide.h"

//#define DEBUG_GLIDE 1

#include "../ext/pyin/MeanFilter.h"
#include "../ext/qm-dsp/maths/MedianFilter.h"

#include <iostream>

using std::cerr;
using std::endl;
using std::vector;
using std::map;

Glide::Extents
Glide::extract_Hz(const vector<double> &pitch_Hz,
                  const CoreFeatures::OnsetOffsetMap &onsetOffsets)
{
    int n = int(pitch_Hz.size());
    vector<double> pitch_semis;
    pitch_semis.reserve(n);
    
    for (int i = 0; i < n; ++i) {
        if (pitch_Hz[i] > 0.0) {
            pitch_semis.push_back(CoreFeatures::hzToPitch(pitch_Hz[i]));
        } else {
            pitch_semis.push_back(0.0);
        }
    }
    
    return extract_semis(pitch_semis, onsetOffsets);
}

Glide::Extents
Glide::extract_semis(const vector<double> &rawPitch,
                     const CoreFeatures::OnsetOffsetMap &onsetOffsets)
{
    int n = int(rawPitch.size());
    
    int halfMedianFilterLength = m_parameters.medianFilterLength_steps / 2;
    vector<double> medianFilterInput = rawPitch;
    for (int i = 0; i < n; ++i) {
        if (medianFilterInput[i] <= 0.0 && i > 0) {
            medianFilterInput[i] = medianFilterInput[i-1];
        }
    }
    vector<double> medianFilteredPitch = MedianFilter<double>::filter
        (m_parameters.medianFilterLength_steps, medianFilterInput);

    vector<double> pitch(n, 0.0);
        
    if (m_parameters.useSmoothing) {
        
        // Modestly mean-filtered pitch, just to take out jitter
        MeanFilter(5).filter(rawPitch.data(), pitch.data(), n);

        for (int i = 0; i < n; ++i) {
            if (rawPitch[i] <= 0.0) {
                pitch[i] = 0.0;
            }
        }

    } else {
        pitch = rawPitch;
    }
    
    // A glide is apparent as soon as the pitch starts to constantly
    // move forward in one direction for at least [threshold:
    // duration], *and* the absolute difference of a pitch and its
    // following median exceeds [threshold: minimum pitch], *and* the
    // absolute difference of a pitch from its previous pitch exceeds
    // [threshold: minimum hop difference].
    //
    // A glide ends as soon as the difference to the median falls
    // below [threshold: minimum hop difference]. If within this span
    // a pitch value deviates by more than [threshold: maximum hop
    // difference] from the previous hop, rule it out as a glide.  For
    // practical reasons we also need to end a glide if a hop is found
    // without a pitch measurement.

    map<int, int> glides; // glide start hop -> glide end hop

    double minimumPitchThreshold_semis =
        m_parameters.minimumPitchThreshold_cents / 100.0;
    double minimumHopDifference_semis =
        m_parameters.minimumHopDifference_cents / 100.0;
    double maximumHopDifference_semis =
        m_parameters.maximumHopDifference_cents / 100.0;

    int glideStart = -1;
    double prevDelta = 0.0;

    // Latches - when set true, these are not set false again until we
    // reach a hop that fails the other thresholds for candidacy
    bool surpassedMedianThreshold = false;
    bool surpassedStartingHopDifference = false;
    
    for (int i = 1; i + halfMedianFilterLength < n; ++i) {

        bool sameDirection = false;
        bool belowMaxDiff = false;
        bool backToMedian = false;
        bool havePitch = (pitch[i] > 0.0);

#ifdef DEBUG_GLIDE
        cerr << "Hop " << i << ": pitch = " << pitch[i]
             << " (" << (havePitch ? CoreFeatures::pitchToHz(pitch[i]) : 0.0)
             << " Hz), surpassed median = " << surpassedMedianThreshold
             << ", surpassed hop = " << surpassedStartingHopDifference
             << endl;
#endif
        
        if (!havePitch) {
#ifdef DEBUG_GLIDE
            cerr << "No pitch" << endl;
#endif
            prevDelta = 0.0;
        } else {
            if (pitch[i-1] > 0.0) {
                double delta = pitch[i] - pitch[i-1];
                double diff = fabs(delta);
                sameDirection = ((delta > 0.0 && prevDelta > 0.0) ||
                                 (delta < 0.0 && prevDelta < 0.0));
                belowMaxDiff = (diff <= maximumHopDifference_semis);
#ifdef DEBUG_GLIDE
                cerr << "Delta = " << delta << ", prevDelta = " << prevDelta
                     << ", sameDirection = " << sameDirection
                     << ", belowMaxDiff = " << belowMaxDiff << endl;
#endif
                if (!surpassedStartingHopDifference &&
                    (diff > minimumHopDifference_semis)) {
                    surpassedStartingHopDifference = true;
#ifdef DEBUG_GLIDE
                    cerr << "Latching surpassedStartingHopDifference" << endl;
#endif
                }
                prevDelta = delta;
            } else {
#ifdef DEBUG_GLIDE
                cerr << "No previous pitch" << endl;
#endif
                prevDelta = 0.0;
            }

            double medianDiff = fabs
                (pitch[i] - medianFilteredPitch[i + halfMedianFilterLength]);
#ifdef DEBUG_GLIDE
            cerr << "Median windowed pitch = "
                 << medianFilteredPitch[i + halfMedianFilterLength]
                 << ", medianDiff = " << medianDiff << endl;
#endif

            if (medianDiff < minimumHopDifference_semis) {
                backToMedian = true;
            } else if (!surpassedMedianThreshold &&
                       medianDiff > minimumPitchThreshold_semis) {
                surpassedMedianThreshold = true;
#ifdef DEBUG_GLIDE
                cerr << "Latching surpassedMedianThreshold" << endl;
#endif
            }
        }

        if (havePitch && belowMaxDiff && sameDirection && !backToMedian) {
            if (glideStart < 0) {
#ifdef DEBUG_GLIDE
                cerr << "This may be start of a glide if the median and hop thresholds are passed" << endl;
#endif
                glideStart = i;
            }
        } else {

            if (glideStart >= 0 &&
                surpassedMedianThreshold &&
                surpassedStartingHopDifference) {
                
                // If at least thresholdSteps candidates in a row
                // previously with total pitch drift more than
                // threshold, record a glide ending here
                
                if (glideStart + m_parameters.durationThreshold_steps <= i &&
                    fabs(pitch[glideStart] - pitch[i-1]) >=
                    minimumPitchThreshold_semis) {
                    glides[glideStart] = i-1;
#ifdef DEBUG_GLIDE
                    cerr << "Noting a glide from " << glideStart
                         << " to " << i-1 << endl;
#endif
                } else {
#ifdef DEBUG_GLIDE
                    cerr << "Glide ended without thresholds having been met "
                         << "between potential glide start "
                         << glideStart << " and " << i-1 << endl;
#endif
                }
            }
                
            glideStart = -1;
            
            if (surpassedMedianThreshold || surpassedStartingHopDifference) {
#ifdef DEBUG_GLIDE
                cerr << "Releasing latches" << endl;
#endif
                surpassedMedianThreshold = false;
                surpassedStartingHopDifference = false;
            }
        }
    }

    if (glideStart >= 0 &&
        surpassedMedianThreshold && surpassedStartingHopDifference &&
        glideStart + m_parameters.durationThreshold_steps < n &&
        fabs(pitch[glideStart] - pitch[n-1]) >= minimumPitchThreshold_semis) {
        glides[glideStart] = n-1;
#ifdef DEBUG_GLIDE
        cerr << "Noting a final glide from " << glideStart
             << " to " << n-1 << endl;
#endif
    }

    int proximitySteps = m_parameters.onsetProximityThreshold_steps;
    
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
#ifdef DEBUG_GLIDE
                cerr << "for glide from " << start << " to " << end
                     << ", found onset within glide at " << onset << endl;
#endif
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
#ifdef DEBUG_GLIDE
                cerr << "for glide from " << start << " to " << end
                     << ", found no onset within glide; using closest onset at "
                     << bestOnset << endl;
#endif
                GlideProperties props;
                props.start = start;
                props.end = end;
                props.provisional = true;
                if (onsetMappedGlides.find(bestOnset) ==
                    onsetMappedGlides.end()) {
#ifdef DEBUG_GLIDE
                    cerr << "no prior glide found for this onset, marking this as provisionally best" << endl;
#endif
                    onsetMappedGlides[bestOnset] = props;
                } else {
                    // already recorded a glide for this onset
                    auto existing = onsetMappedGlides.at(bestOnset);
                    if (existing.provisional) {
                        if (bestOnset > end) {
                            // we must be closer to onset than that
                            // one (because we always see glides in
                            // ascending time order)
#ifdef DEBUG_GLIDE
                            cerr << "closer to onset than prior glide, marking this as provisionally best" << endl;
#endif
                            onsetMappedGlides[bestOnset] = props;
                        } else if (bestOnset > existing.end) {
                            if ((end - start > existing.end - existing.start)
                                &&
                                minDist < bestOnset - existing.end) {
                                // this glide is longer and closer,
                                // prefer it (and since we're after
                                // onset now, it's unimprovable)
                                props.provisional = false;
#ifdef DEBUG_GLIDE
                                cerr << "longer and closer to onset than prior glide, marking this as best" << endl;
#endif
                                onsetMappedGlides[bestOnset] = props;
                            } else {
#ifdef DEBUG_GLIDE
                                cerr << "existing glide is closer or longer, leaving it" << endl;
#endif
                            }
                        } else {
#ifdef DEBUG_GLIDE
                            cerr << "existing glide is already after onset, leaving it" << endl;
#endif
                        }
                    }
                }                            
            } else {
#ifdef DEBUG_GLIDE
                cerr << "for glide from " << start << " to " << end
                     << ", found no onset within glide or within proximity "
                     << "range; ignoring this glide" << endl;
#endif
            }
        }
    }

    Extents extents;
    for (auto g : onsetMappedGlides) {
        extents[g.first] = { g.second.start, g.second.end };
    }
    return extents;
}

