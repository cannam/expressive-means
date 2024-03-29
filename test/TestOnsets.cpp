
/*
    Expressive Means Onsets

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <boost/test/unit_test.hpp>

#include "../src/CoreFeatures.h"

#include "bqaudiostream/AudioWriteStream.h"
#include "bqaudiostream/AudioWriteStreamFactory.h"

#include <iostream>

using std::cerr;
using std::endl;

static int testSignalRate = 44100;

static
std::vector<float>
makeTestSignal()
{
    int rate = testSignalRate;
    int halfsec = rate / 2;
    float f1 = 220.0;
    float f2 = 196.0;

    // Signal consists of
    // 0.5 sec (0.0 to 0.5) silence
    // 1.0 sec (0.5 to 1.5) sine tone at freq f1
    // 0.5 sec (1.5 to 2.0) glide to freq f2
    // 1.0 sec (2.0 to 3.0) sine tone at f2
    // 1.0 sec (3.0 to 4.0) sine + harmonics at f2
    // 0.5 sec (4.0 to 4.5) silence
    // Total 4.5 sec
    
    int duration = halfsec * 9;
    std::vector<float> signal(duration, 0.f);
    float freq = f1;
    float arg = 0.f;
    float mag = 0.5f;

    for (int i = halfsec; i < halfsec * 8; ++i) {

        if (i == halfsec) {
            freq = f1;
        } else if (i == halfsec * 4) {
            freq = f2;
        } else if (i >= halfsec * 3 && i < halfsec * 4) {
            freq = f1 + ((f2 - f1) * float(i - halfsec * 3)) / float(halfsec);
        }

        signal[i] = mag * sinf(arg);
        arg += 2.0 * M_PI * freq / float(rate);
        
        if (i > halfsec * 6) {
            for (int h = 2; h <= 8; ++h) {
                signal[i] += (mag / h) * sinf(arg * h);
            }
        }
    }

    auto str = breakfastquay::AudioWriteStreamFactory::createWriteStream
        ("testsignal.wav", 1, rate);
    str->putInterleavedFrames(duration, signal.data());
    delete str;
    
//    for (int i = 0; i < duration; ++i) {
//        cerr << "# " << i << "," << signal[i] << endl;
//    }
    
    return signal;
}

BOOST_AUTO_TEST_SUITE(TestOnsets)

BOOST_AUTO_TEST_CASE(defaultParams)
{
    auto signal = makeTestSignal();

    CoreFeatures cf(testSignalRate);
    int bs = cf.getPreferredBlockSize();
    int hop = cf.getPreferredStepSize();

    CoreFeatures::Parameters params;

    // Necessary for this synthetic example
    params.pyinFixedLag = false;
        
    cf.initialise(params);
    
    for (int i = 0; i + bs <= int(signal.size()); i += hop) {
        cf.process(signal.data() + i,
                   Vamp::RealTime::frame2RealTime(i, testSignalRate));
    }

    cf.finish();

    /*
    auto pitches = cf.getPYinPitch_Hz();
    int i = 0;
    for (auto pitch : pitches) {
        cerr << i++ << ": " << pitch << endl;
    }
    */
    
    auto onsets = cf.getMergedOnsets();

    // We should see onsets at 0.5 sec, 2.0 sec, 3.0 sec. The first
    // and third are spectral rise type, the second pitch change.
    BOOST_CHECK(onsets.size() == 3);

    vector<int> hops;
    vector<Vamp::RealTime> times;
    vector<CoreFeatures::OnsetType> types;
    
    for (auto onset : onsets) {
        /*
        cerr << "Onset at " << onset.first
             << " (" << cf.timeForStep(onset.first) << " )"
             << " of type " << int(onset.second) << endl;
        */
        hops.push_back(onset.first);
        times.push_back(cf.timeForStep(onset.first));
        types.push_back(onset.second);
    }

    // These are the "acceptance" ranges
    
    BOOST_CHECK(times[0] > Vamp::RealTime::fromSeconds(0.47));
    BOOST_CHECK(times[0] < Vamp::RealTime::fromSeconds(0.51));
    BOOST_CHECK(types[0] == CoreFeatures::OnsetType::SpectralLevelRise);

    BOOST_CHECK(times[1] > Vamp::RealTime::fromSeconds(1.97));
    BOOST_CHECK(times[1] < Vamp::RealTime::fromSeconds(2.01));
    BOOST_CHECK(types[1] == CoreFeatures::OnsetType::Pitch);

    BOOST_CHECK(times[2] > Vamp::RealTime::fromSeconds(2.97));
    BOOST_CHECK(times[2] < Vamp::RealTime::fromSeconds(3.01));
    BOOST_CHECK(types[2] == CoreFeatures::OnsetType::SpectralLevelRise);

    // These are the "regression" values

    BOOST_CHECK(hops[0] == 80);
    BOOST_CHECK(hops[1] == 336);
    BOOST_CHECK(hops[2] == 511);
}

BOOST_AUTO_TEST_SUITE_END()
