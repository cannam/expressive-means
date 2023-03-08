
/*
    Expressive Means

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Glide.h"

#include "../ext/pyin/MeanFilter.h"

#include <iostream>

using std::cerr;
using std::endl;
using std::vector;
using std::map;

Glide::Extents
Glide::extract(const vector<double> &pitch_Hz,
               const CoreFeatures::OnsetOffsetMap &onsetOffsets)
{
    int n = pitch_Hz.size();

    vector<double> rawPitch;
    for (int i = 0; i < n; ++i) {
        if (pitch_Hz[i] > 0.0) {
            rawPitch.push_back(CoreFeatures::hzToPitch(pitch_Hz[i]));
        } else if (i > 0) {
            rawPitch.push_back(rawPitch[i-1]);
        } else {
            rawPitch.push_back(0.0);
        }
    }
    
    // Modestly mean-filtered pitch, just to take out jitter
    vector<double> pitch(n, 0.0);
    MeanFilter(5).filter(rawPitch.data(), pitch.data(), n);

    for (int i = 0; i < n; ++i) {
        if (pitch_Hz[i] <= 0.0) {
            pitch[i] = 0.0;
        }
    }

    // "A glide is apparent as soon as the pitch starts to constantly
    // move forward in one direction for at least [threshold:
    // duration], but each hop's value not deviating more than
    // [threshold: pitch] cents from the hop value before".
    //
    // A couple of additions to this:
    //
    // * It's implicit that there must *be* pitch measurements
    // continuously available during this time
    //
    // * Although each hop should not deviate more than the given
    // threshold from the previous one, we do insist that the total
    // deviation through the whole glide is over this threshold, to
    // eliminate tiny drifts in pitch through a note

    vector<double> pitchDelta;
    vector<int> candidates; // hop
    map<int, int> glides; // glide start hop -> glide end hop

    int lastNonCandidate = -1;
    int thresholdSteps = m_parameters.durationThreshold_steps;
    double threshold = m_parameters.pitchThreshold_semis;
    double prevDelta = 0.0;
    
    for (int i = 1; i < n; ++i) {

        bool sameDirection = false;
        bool belowThreshold = false;
        bool havePitch = (pitch[i] > 0.0);

        if (havePitch) {
            if (pitch[i-1] > 0.0) {
                double delta = pitch[i] - pitch[i-1];
                belowThreshold = (fabs(delta) <= threshold);
                sameDirection = ((delta > 0.0 && prevDelta > 0.0) ||
                                 (delta < 0.0 && prevDelta < 0.0));
                pitchDelta.push_back(delta);
                prevDelta = delta;
            } else {
                pitchDelta.push_back(0.0);
                prevDelta = 0.0;
            }
        } else {
            pitchDelta.push_back(0.0);
            prevDelta = 0.0;
        }

        bool isCandidate = (havePitch && belowThreshold && sameDirection);

        if (isCandidate) {
            candidates.push_back(i);
        } else {
            // Not a candidate: If at least thresholdSteps candidates
            // in a row previously with total pitch drift more than
            // threshold, record a glide ending here
            if (lastNonCandidate + thresholdSteps <= i &&
                fabs(pitch[i-1] - pitch[lastNonCandidate + 1]) > threshold) {
                glides[lastNonCandidate + 1] = i-1;
            }
            lastNonCandidate = i;
        }
    }

    if (lastNonCandidate + thresholdSteps < n &&
        fabs(pitch[n-1] - pitch[lastNonCandidate + 1]) > threshold) {
        glides[lastNonCandidate + 1] = n-1;
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
                cerr << "for glide from " << start << " to " << end
                     << ", found onset within glide at " << onset << endl;
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
                cerr << "for glide from " << start << " to " << end
                     << ", found no onset within glide; using closest onset at "
                     << bestOnset << endl;
                GlideProperties props;
                props.start = start;
                props.end = end;
                props.provisional = true;
                if (onsetMappedGlides.find(bestOnset) ==
                    onsetMappedGlides.end()) {
                    cerr << "no prior glide found for this onset, marking this as provisionally best" << endl;
                    onsetMappedGlides[bestOnset] = props;
                } else {
                    // already recorded a glide for this onset
                    auto existing = onsetMappedGlides.at(bestOnset);
                    if (existing.provisional) {
                        if (bestOnset > end) {
                            // we must be closer to onset than that
                            // one (because we always see glides in
                            // ascending time order)
                            cerr << "closer to onset than prior glide, marking this as provisionally best" << endl;
                            onsetMappedGlides[bestOnset] = props;
                        } else if (bestOnset > existing.end) {
                            if ((end - start > existing.end - existing.start)
                                &&
                                minDist < bestOnset - existing.end) {
                                // this glide is longer and closer,
                                // prefer it (and since we're after
                                // onset now, it's unimprovable)
                                props.provisional = false;
                                cerr << "longer and closer to onset than prior glide, marking this as best" << endl;
                                onsetMappedGlides[bestOnset] = props;
                            } else {
                                cerr << "existing glide is closer or longer, leaving it" << endl;
                            }
                        } else {
                            cerr << "existing glide is already after onset, leaving it" << endl;
                        }
                    }
                }                            
            } else {
                cerr << "for glide from " << start << " to " << end
                     << ", found no onset within glide or within proximity "
                     << "range; ignoring this glide" << endl;
            }
        }
    }

    Extents extents;
    for (auto g : onsetMappedGlides) {
        extents[g.first] = { g.second.start, g.second.end };
    }
    return extents;
}

