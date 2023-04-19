
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
    
    vector<int> rawPeaks;
    auto elements = pv.extractElements(pitch_Hz, rawPeaks);

    cerr << endl << testName << " test: extractElements finished" << endl;
    
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
        453.711, 454.811, 456.085, 456.598, 460.746,  // 0
        464.067, 466.808, 476.18, 480.268, 487.184, 
        0.0, 0.0, 0.0, 0.0, 0.0,                      // 10
        0.0, 0.0, 531.22, 533.772, 533.986,
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

    testVibratoClassification
        ("Huberman 11.4s", pitch_Hz, onsetOffsets, "4Sn:");
}

BOOST_AUTO_TEST_CASE(szeryng_300)
{
    vector<double> pitch_Hz {
        1042.44, 1042.12, 1041.36, 1041.3, 1040.22, // 0
        1039.5, 1039.5, 1039.67, 1039.65, 1040.16,
        1041.03, 1043, 1045.64, 1046.72, 1048.54, // 10
        1047.65, 1046.97, 1052.75, 1061, 1072.36,
        1086.83, 1099.05, 1107.7, 1116.35, 1122.76, // 20
        1125.25, 1129.27, 1132.29, 1134.83, 1139.9,
        // Onset here
        1140.81, 1141.85, 1143.69, 1143.72, 1145.41, // 30
        1149.34, 1150.65, 1153.2, 1156.54, 1156.07,
        1158.61, 1161.19, 1163.45, 1166.97, 1169.94, // 40
        1169.94, 1168.29, 1169.82, 1170.06, 1173.96,
        1175.49, 1172.6, 1171.64, 1166.72, 1165.58, // 50
        1164.5, 1160.21, 1157.5, 1155.45, 1154.17,
        1154.75, 1156.15, 1157.82, 1158.98, 1160.11, // 60
        1161.94, 1163.13, 1166.69, 1173.3, 1175.81,
        1179.02, 1182.38, 1185.1,                    // 70
        // Tilo's app finds two vibrato segments, the first beginning here
        1188.22, 1184.66,
        1183.52, 1180.14, 1177.07, 1172.64, 1167.13,
        1162.94, 1158.36, 1155.6, 1154.88, 1156.32, // 80
        1156.48, 1156.84, 1158.26, 1160.25, 1163.19,
        1169.94, 1176.61, 1180.72, 1184.01, 1184.81, // 90
        1189.05,
        // and ending here, with the second beginning here
        1190.99, 1192.29, 1192.92, 1184.32,
        1174.23, 1167.2, 1163.45, 1159.32, 1155.82, // 100
        1154.86, 1154.04, 1155.23, 1156.65, 1161.18,
        1164.18, 1168.28, 1171.88, 1174.07, 1175.15, // 110
        1176.16, 1177.16, 1181.92,
        // and ending here
        1187.58, 1187.23,
        1188.57, 1185.76, 1181.6, 1175.24, 1170.94, // 120
        1166.95, 1164.39, 1163.71, 1162.78, 1163.76,
        1165.57, 1168.97, 1171.68, 1173.85, 1175.26, // 130
        1176.32, 1176.48, 1174.24,
        // Following onset here
        1170.06, 1159.83,
        1149.37, 1140.84, 1134.27, 1133.56, 1131.56, // 140
        1131.4, 1134.78, 1136.26, 1137.59, 1136.67,
        1132.91, 1128.45, 1128.24, 1127.68, 1126.74, // 150
        1126.59, 1125.77, 1126.17, 1125.84, 1126.82,
        1127.13, 1128.71, 1128.94, 1128.73, 1129.55, // 160
        1127.68, 1128.57, 1127.94, 1126.51, 1124.67,
        1123.15, 1122.81, 1121.5, 1122.82, 1123.07 // 170
    };

    CoreFeatures::OnsetOffsetMap onsetOffsets;
    // (the type is irrelevant)
    onsetOffsets[30] = { 138, CoreFeatures::OffsetType::PowerDrop };

    //!!! I just made this code up, but at this point the important
    //!!! thing is that vibrato is detected at all!
    testVibratoClassification
        ("Szeryng 0.3s", pitch_Hz, onsetOffsets, "4Mm=");
}

BOOST_AUTO_TEST_CASE(szeryng_6400)
{
    vector<double> pitch_Hz {
        587.111, 587.014, 586.82, 587.092, 587.282, // 0
        587.414, 587.295, 586.973, 586.974, 587.11,
        587.545, 587.631, 587.586, 587.304, 586.978, // 10
        586.961, 586.816, 586.991, 586.84, 586.221,
        585.4, 584.354,
        // Onset here
        583.366, 583.16, 586.316,                    // 20
        590.326, 594.173, 595.388, 595.104, 594.305,
        592.906, 591.232, 588.822, 587.073, 586.017, // 30
        584.873, 583.68, 582.911, 582.8, 583.332,
        584.545, 586.042, 587.634, 590.34, 593.501, // 40
        596.267, 597.783, 598.677, 598.803, 599.204,
        599.07, 597.949, 596.905, 594.805, 591.499, // 50
        587.243, 584.031, 583.211, 582.524, 581.744,
        581.664, 581.482, 582.634, 583.269, 585.278, // 60
        587.879, 590.284, 592.755, 594.87, 595.988,
        596.955, 597.307, 597.567, 597.579, 596.163, // 70
        595.386, 593.781, 591.677, 589.075, 586.73,
        585.871, 585.455, 584.662, 584.792, 585.086, // 80
        586.285, 588.009, 590.077, 592.998, 595.735,
        598.235, 598.115, 598.084, 597.301, 595.567, // 90
        593.612, 590.762,
        // Following onset here
        588.551, 585.765, 521.861,
        521.112, 523.25, 521.475, 521.435, 521.26, // 100
        521.837, 522.174, 521.934, 521.997, 521.858,
        521.877, 521.453, 520.756, 520.64, 520.783, // 110
        521.358, 522.019, 522.48, 522.39, 521.943,
        521.788, 521.497, 521.519, 521.571, 521.811, // 120
        521.992, 522.099, 521.945, 521.396, 521.03,
        520.7                                       // 130
    };

    CoreFeatures::OnsetOffsetMap onsetOffsets;
    // (the type is irrelevant)
    onsetOffsets[22] = { 97, CoreFeatures::OffsetType::PowerDrop };

    //!!! I just made this code up, but at this point the important
    //!!! thing is that vibrato is detected at all!

    // (Actually we really want to test that it is detected throughout
    // the note, since in visual checks just now it appears we have
    // two segments that are not joined up)
    
    testVibratoClassification
        ("Szeryng 6.4s", pitch_Hz, onsetOffsets, "4Mn=");
}

BOOST_AUTO_TEST_SUITE_END()

