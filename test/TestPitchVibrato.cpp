
/*
    Expressive Means 

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <boost/test/unit_test.hpp>

#include "../src/PitchVibrato.h"

#include <iostream>
using std::cerr;
using std::endl;

BOOST_AUTO_TEST_SUITE(TestPitchVibrato)

// Test case name is file id + approx time in msec of the note we are
// testing (for reference against SV plot). Pitch values are taken
// direct from the actual pitch track.

static void testVibratoClassification(std::string testName,
                                      const std::vector<double> &pitch_Hz,
                                      const CoreFeatures::OnsetOffsetMap &onsetOffsets,
                                      std::string expectedClassification)
{
    PitchVibrato pv(44100.f);
    pv.initialise(1, pv.getPreferredStepSize(), pv.getPreferredBlockSize());

    cerr << endl << testName << " test: Running extractElements" << endl;
    
    vector<double> smoothedPitch;
    vector<int> rawPeaks;
    auto elements = pv.extractElements(pitch_Hz, smoothedPitch, rawPeaks);

    cerr << endl << testName << " test: extractElements finished" << endl;
    
    cerr << testName << ": extractElements returned the following smoothed pitch curve (semitones):" << endl;
    for (auto p : smoothedPitch) {
        cerr << p << " ";
    }
    cerr << endl;
    
    cerr << testName << ": extractElements returned the following raw peak indices:" << endl;
    for (auto p : rawPeaks) {
        cerr << p << " ";
    }
    cerr << endl;
    
    cerr << testName << ": extractElements returned the following elements:" << endl;
    for (auto e : elements) {
        cerr << "hop " << e.hop << " (peak index " << e.peakIndex
             << "): range = " << e.range_semis << " semitones, position = "
             << e.position_sec << " seconds, wavelength = "
             << e.waveLength_sec << " seconds, correlation = "
             << e.correlation << endl;
    }

    cerr << endl << testName << " test: Running classify" << endl;

    auto classification = pv.classify(elements, onsetOffsets);

    cerr << endl << testName << " test: classify finished" << endl;
    
    cerr << testName << ": classify returned the following classifications:" << endl;
    int i = 1;
    for (auto c : classification) {
        cerr << i << ". Onset at " << c.first << ": "
             << pv.classificationToCode(c.second)
             << endl;
        ++i;
    }
    cerr << endl;
    
    BOOST_TEST(classification.size() == 1);
    BOOST_TEST(pv.classificationToCode(classification.begin()->second) ==
               expectedClassification);
}

BOOST_AUTO_TEST_CASE(huberman_812)
{
    vector<double> pitch_Hz {
        1155.18, 1155.36, 1155.8, 1156.08, 1156.01,   // hop 0
        1155.59, 1155.59, 1155.24, 1155.82, 1155.89,
        1155.53, 1155.75, 1155.87, 1155.54, 1156.26,  // 10
        1156.56, 1157.1,
        // Onset is reported here at 17
        1161.32, 1169.43, 1296.37,
        1343.02, 1357.89, 1356.7, 1357.32, 1361.72,   // 20
        1358.45, 1361.45, 1363.11, 1356.86, 1350, 1342.1,
        1335.08, 1327.26, 1329.03, 1332.42, 1337.08,  // 30
        1341.52, 1340.79, 1341.72, 1344.31, 1358.94,
        1368.62, 1374.74, 1377.8, 1380.71, 1385.64,   // 40
        1384.68, 1385.46, 1383.55, 1382.32, 1383.29,
        1381.38, 1384.53, 1384.17, 1381.61, 1380.98,  // 50
        1377.23, 1374.23, 1376.81, 1377.82, 1379.95,
        1383.01, 1385.37, 1387.05, 1388.1, 1388.15,   // 60
        1387.62,
        // Note ends and following onset reported here at 66
        1370.12, 1343.61, 1294.6, 1259.04,
        1247.73, 1246.39, 1248.16, 1247.62, 1247.06,  // 70
        1243.54, 1238.87, 1237.7, 1234.87, 1232.28
    };

    CoreFeatures::OnsetOffsetMap onsetOffsets;
    // (the type is irrelevant)
    onsetOffsets[17] = { 66, CoreFeatures::OffsetType::PowerDrop };

    testVibratoClassification
        ("Huberman 0.812s", pitch_Hz, onsetOffsets, "4Fn>");
}

BOOST_AUTO_TEST_CASE(huberman_11400)
{
    vector<double> pitch_Hz {
        453.957, 453.792, 453.944, 454.135, 453.919, // 0
        454.023, 454.041, 453.711, 454.811, 456.085,
        456.598, 460.746, 464.067, 466.808, 476.18, // 10
        480.268, 487.184, 531.22, 533.772, 533.986,
        534.529, 535.037, 535.198, 535.394, 536.199, // 20
        536.761, 537.431, 537.11, 537.515, 537.466,
        537.489, 538.417, 538.614, 539.703, 541.066, // 30
        542.348,
        // Onset is reported here at 36
        542.865, 542.786, 542.334, 541.468,
        540.392, 539.573, 539.062, 538.734, 538.8, // 40
        538.596, 538.058, 537.425, 536.551, 535.244,
        534.497, 533.642, 533.97, 534.463, 535.035, // 50
        536.071, 536.838, 537.745, 538.828, 539.792,
        540.573, 541.443, 542.074, 542.848, 543.342, // 60
        543.994, 544.68, 545.368, 545.653, 545.379,
        543.826, 542.397, 540.569, 539.168, 537.925, // 70
        536.407, 535.623, 535.019, 534.341, 534.229,
        534.048, 533.956, 534.905, 535.84, 536.732, // 80
        537.61, 538.324, 538.767, 539.467, 539.886,
        540.404, 540.549, 540.564, 540.426, 539.996, // 90
        539.735, 539.369, 539.25, 539.111, 538.67,
        538.588, 538.219, 538.276, 538.58, 538.804, // 100
        539.292, 539.438, 540.005, 540.11, 539.898,
        539.623, 539.165, 538.734, 539.144, 539.402, // 110
        539.286, 539.523, 539.339, 539.421, 539.819,
        539.726, 539.505, 539.137, 538.922, 538.803, // 120
        538.771, 538.496, 537.698, 536.735, 535.95,
        535.638, 534.894, 534.584, 534.15, 533.627, // 130
        533.741, 533.384, 533.174, 532.866, 532.36,
        532.029, 531.337, 531.029, 529.836, 529.441, // 140
        // Note ends here at 145; there is no following onset in this range
        528.468, 526.526, 525.568, 522.919, 520.013,
        516.213, 514.148, 513.009, 507.976, 506.725, // 150
        504.916, 502.865, 503.376, 499.496, 497.435,
        493.929, 492.291, 490.818, 488.565, 486.672, // 160
        483.669, 483.882, 484.405, 479.597, 478.993,
        476.937, 474.967, 475.143, 472.271, 470.696, // 170
        470.62, 468.476, 469.216
    };

    CoreFeatures::OnsetOffsetMap onsetOffsets;
    // (the type is irrelevant)
    onsetOffsets[36] = { 145, CoreFeatures::OffsetType::PowerDrop };

    //!!! check this!
    testVibratoClassification
        ("Huberman 11.4s", pitch_Hz, onsetOffsets, "4Sn:");
}

BOOST_AUTO_TEST_SUITE_END()

