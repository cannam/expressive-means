
/*
    Expressive Means 

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <boost/test/unit_test.hpp>

#include "../src/Glide.h"
#include "../src/Portamento.h"

#include <iostream>
using std::cerr;
using std::endl;

std::ostream &operator<<(std::ostream &os, Portamento::GlideDirection x) {
    return (os << Portamento::glideDirectionToString(x));
}
std::ostream &operator<<(std::ostream &os, Portamento::GlideRange x) {
    return (os << Portamento::glideRangeToString(x));
}
std::ostream &operator<<(std::ostream &os, Portamento::GlideDuration x) {
    return (os << Portamento::glideDurationToString(x));
}
std::ostream &operator<<(std::ostream &os, Portamento::GlideLink x) {
    return (os << Portamento::glideLinkToString(x));
}
std::ostream &operator<<(std::ostream &os, Portamento::GlideDynamic x) {
    return (os << Portamento::glideDynamicToString(x));
}

BOOST_AUTO_TEST_SUITE(TestGlide)

static void testSingleGlide(const vector<double> &pitch_Hz,
                            const CoreFeatures::OnsetOffsetMap &onsets,
                            int expectedOnset,
                            int expectedStart,
                            int expectedEnd)
{
    Glide::Parameters parameters;
    Glide glide(parameters);
    auto extents = glide.extract_Hz(pitch_Hz, onsets);
/*
    cerr << "Glide returned the following:" << endl;
    for (auto e : extents) {
        cerr << "(onset " << e.first << "): " << e.second.start
             << " -> " << e.second.end << endl;
    }
*/    
    BOOST_TEST(extents.size() == 1);
    BOOST_TEST(extents.begin()->first == expectedOnset);
    BOOST_TEST(extents.begin()->second.start == expectedStart);
    BOOST_TEST(extents.begin()->second.end == expectedEnd);
}

static void testGlideClassification(const pair<int, Glide::Extent> &glide,
                                    const CoreFeatures::OnsetOffsetMap &onsetOffsets,
                                    const vector<double> &pyinPitch,
                                    const vector<double> &smoothedPower,
                                    Portamento::GlideDirection expectedDirection,
                                    Portamento::GlideRange expectedRange,
                                    Portamento::GlideDuration expectedDuration,
                                    Portamento::GlideLink expectedLink,
                                    Portamento::GlideDynamic expectedDynamic)
{
    Portamento portamento(44100.f);
    portamento.initialise(1,
                          portamento.getPreferredStepSize(),
                          portamento.getPreferredBlockSize());

    auto classification = portamento.classifyGlide(glide,
                                                   onsetOffsets,
                                                   pyinPitch,
                                                   smoothedPower);

    BOOST_TEST(classification.direction == expectedDirection);
    BOOST_TEST(classification.range == expectedRange);
    BOOST_TEST(classification.duration == expectedDuration);
    BOOST_TEST(classification.link == expectedLink);
    BOOST_TEST(classification.dynamic == expectedDynamic);
}

// Test case name is file id + approx time in msec of the glide or
// non-glide we are testing (for reference against SV plot). Pitch
// values are taken direct from the actual pitch track.
//
// Note that the default glide parameters (a Parameters object
// constructed without anything being assigned to it) may differ from
// the default parameters within the plugin - this is inevitable in
// some cases, e.g. the default glide parameters use fixed hop counts
// while the plugin derives them from time in ms.

BOOST_AUTO_TEST_CASE(roehn_2180)
{
    vector<double> pitch_Hz {
        1271.45,                                      // hop 0
        // Onset here at 1
        1279.99, 1263.85, 1233.46, 1219.25,
        1208.38, 1201.89, 1198.98, 1195.61, 1196.6, 
        1199.59, 1202.85, 1205.11, 1205.09, 1205.48,  // 10
        1204.18, 1203.38, 1206, 1210.53, 1218.96,   
        1223.04, 1226.25, 1225.84, 1223.68, 1207.18,  // 20
        1194.13, 1188.47, 1183.71, 1179.13, 1174.95,
        1172.85, 1173.67, 1175.69, 1179.19, 1184.33,  // 30
        1190.16, 1197.5, 1202.4, 1203.23, 1203.09,
        1203.43, 1201.07, 1199.6, 1196.63, 1192.79,   // 40
        1190.64, 1190.08, 1189.3, 1189.37, 1189.26,
        // Glide starts here at 50
        1187.98, 1186.45, 1184.93, 1182.86, 1180.24,  // 50
        1175.09, 1170.84, 1167.75, 1162.22, 1153.47,
        1138.81, 1130, 1124.44, 1111.68,     // 60
        // Glide ends here at 64 if we end it when pitch returns to
        // median (the variation in the following few steps is then
        // taken as vibrato)
        1096.36,
        1082.16, 1067.72, 1059.89,
        // Offset and following onset here at 68
        1040.02, 1027.41,
        // Glide ends here at 70 if we end it only when the drift
        // stops
        1026.17, 1031.77, 1052.97, 1095.3, 1112.98,   // 70
        1113.55, 1103.35, 1092.06, 1082.63, 1081.31,
        1079.66, 1079.68, 1080.79, 1082.17, 1083.31,  // 80
        1083.76, 1082.92, 1080.84, 1081.8, 1079.2,
        1077.74, 1074.94, 1070.01, 1066.9, 1062.26,   // 90
        1058.83, 1058.69, 1060.08, 1061.36, 1063.58,
        1065.49, 1068.49, 1071.2, 1074.5, 1075.13,    // 100
        1076.01, 1077.15, 1079.22, 1079.61, 1079.29,
        1078.18, 1074.76, 1072.07, 1067.96, 1063.04,  // 110
        1059.15, 1060.41, 1060.33, 1059.34, 1059.59,
        1059.47, 1060.61, 1063.98, 1069.59, 1075.2,   // 120
        1078.57, 1080.33, 1079.93, 1079.35, 1076.19,
        1074.98, 1073.87, 1071.6, 1070.98, 1069.06,   // 130
        1067.5, 1063.48, 1054.21, 1041.29, 1031.94
    };

    CoreFeatures::OnsetOffsetMap onsets;
    onsets[1] = { 68, CoreFeatures::OffsetType::FollowingOnsetReached };
    // Offset here is arbitrary
    onsets[68] = { 140, CoreFeatures::OffsetType::FollowingOnsetReached };

    testSingleGlide(pitch_Hz, onsets, 68, 50, 64);
}

BOOST_AUTO_TEST_CASE(huberman_11987)
{
    vector<double> pitch_Hz {
        542.786, 542.334, 541.468, 540.392, 539.573, // 0
        539.062, 538.734, 538.8, 538.596, 538.058,
        537.425, 536.551, 535.244, 534.497, 533.642, // 10
        533.97, 534.463, 535.035, 536.071, 536.838,
        537.745, 538.828, 539.792, 540.573, 541.443, // 20
        542.074, 542.848, 543.342, 543.994, 544.68,
        545.368, 545.653, 545.379, 543.826, 542.397, // 30
        540.569, 539.168, 537.925, 536.407, 535.623,
        535.019, 534.341, 534.229, 534.048, 533.956, // 40
        534.905, 535.84, 536.732, 537.61, 538.324,
        538.767, 539.467, 539.886, 540.404, 540.549, // 50
        540.564, 540.426, 539.996, 539.735, 539.369,
        539.25, 539.111, 538.67, 538.588, 538.219, // 60
        538.276, 538.58, 538.804, 539.292, 539.438,
        540.005, 540.11, 539.898, 539.623, 539.165, // 70
        538.734, 539.144, 539.402, 539.286, 539.523,
        539.339, 539.421, 539.819, 539.726, 539.505, // 80
        539.137, 538.922, 538.803, 538.771, 538.496,
        537.698, 536.735, 535.95, 535.638, 534.894, // 90
        534.584, 534.15, 533.627, 533.741, 533.384,
        533.174, 532.866, 532.36, 532.029, 531.337, // 100
        531.029, 529.836, 529.441,
        // Offset here at 108 (cause: spectral drop)
        528.468, 526.526,
        525.568, 522.919, 520.013, 516.213, 514.148, // 110
        513.009, 507.976, 506.725, 504.916, 502.865,
        503.376, 499.496, 497.435, 493.929, 492.291, // 120
        490.818, 488.565, 486.672, 483.669, 483.882,
        484.405, 479.597, 478.993, 476.937, 474.967, // 130
        475.143, 472.271, 470.696, 470.62, 468.476,
        469.216, 469.49, 467.612, 469.146, 467.137, // 140
        465.891, 465.174, 462.583, 462.679, 461.523,
        461.94, 463.137, 461.494, 460.993, 460.614, // 150
        459.318, 459.926, 458.213, 458.617, 458.696,
        0.0, 0.0, 0.0, 0.0, 0.0,                    // 160
        0.0, 0.0, 0.0, 0.0,
        // Next onset here at 169
        0.0,
        0.0, 0.0, 0.0, 377.788, 376.763,            // 170
        375.449, 375.17, 375.706, 376.561, 377.751,
        378.103, 377.579, 377.543, 377.908, 378.476, // 180
        379.591, 379.831, 380.738, 381.761, 381.948,
        382.608, 383.059, 383.043, 383.402, 383.685, // 190
        // Offset here at 195 (cause: spectral drop)
        383.467, 383.043, 382.766, 381.826, 381.019,
        380.215, 379.173, 378.749, 378.268, 378.253, // 200
        376.939, 377.905, 376.673, 376.45, 376.567,
        376.377, 376.731, 377.189, 378.275, 379.154, // 210
        380.303, 381.346, 382.217, 383.259, 384.018,
        385.111, 385.804, 386.092, 385.92, 385.583 // 220
    };

    CoreFeatures::OnsetOffsetMap onsets;
    onsets[0] = { 108, CoreFeatures::OffsetType::SpectralLevelDrop };
    onsets[169] = { 195, CoreFeatures::OffsetType::SpectralLevelDrop };

    testSingleGlide(pitch_Hz, onsets, 169, 84, 146);
}

BOOST_AUTO_TEST_CASE(zimmermann_458)
{
    vector<double> pitch_Hz {
        1193.03, 1195.28, 1193.9, 1190.7, 1179.52,   // 0
        1158.76,
        // Onset here at 6
        1140.48, 1132.1, 1123.98, 1120.26,
        1116.74, 1116.66, 1111.85, 1107.73, 1107.94, // 10
        1105.93, 1105.93, 1107.63, 1107.46, 1107.98,
        1112.12, 1107.29, 1110.66, 1110.29, 1108.02, // 20
        1111.07, 1108.74, 1110.73, 1111.13, 1109.22,
        1109.71, 1107.45, 1107.07, 1107.57,          // 30
        1106.97, 1108.26,
        // Glide starts here at 36
        1109.68, 1116.74, 1125.74, 1135.48,
        1145.17, 1152.23, 1157.46, 1161.03, 1165.24, // 40
        1170.11, 1178.42, 1189.5, 1197.42, 1203.97,
        1210.11, 1219.56, 1225.37, 1229.51, 1238.27, // 50
        1248.98, 1256.12,
        // Offset and next onset here at 57
        1260.84, 1266.82, 1273.07,
        1284.13,                                     // 60
        // Glide ends here at 61
        1296.87, 1302.65, 1306.08, 1307.45,
        1304.98, 1301.33, 1306.64, 1309.31, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0,                     // 70
        0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0,                     // 80
        0.0, 0.0, 0.0, 0.0, 0.0,
        // Offset and next onset here at 90
        0.0, 0.0, 0.0, 1154.54, 1156.17,             // 90
        1162.05, 1162.57, 1162.76, 1164.06, 1164.19,
        1165.78, 1167.79, 1168.85, 1170.8, 1172.8,   // 100
        1174.03, 1176.41, 1179.47
    };

    vector<double> smoothedPower_dB {
        -27.0302, -26.8771, -26.6809, -26.4685, -26.2683,
        -26.0781, -25.8742, -25.6611, -25.4132, -25.145,
        -24.8399, -24.5279, -24.1825, -23.8385, -23.5379,
        -23.3203, -23.138, -22.9621, -22.837, -22.711,
        -22.615, -22.5339, -22.4494, -22.3453, -22.2353,
        -22.1338, -22.0244, -21.9286, -21.8387, -21.765,
        -21.6858, -21.6298, -21.5899, -21.5632, -21.5689,
        -21.6049, -21.6501, -21.7058, -21.8127, -21.9455,
        -22.0652, -22.1681, -22.235, -22.2936, -22.3462,
        -22.3934, -22.4313, -22.4603, -22.5176, -22.6303,
        -22.79, -22.9723, -23.2093, -23.4947, -23.8265,
        -24.2068, -24.622, -25.0381, -25.4652, -25.9503,
        -26.4867, -27.0482, -27.6041, -28.1578, -28.7005,
        -29.235, -29.7529, -30.238, -30.6575, -31.0132,
        -31.3275, -31.5747, -31.7452, -31.8312, -31.862,
        -31.8285, -31.7402, -31.6041, -31.4, -31.1733,
        -30.955, -30.7558, -30.5606, -30.3793, -30.2075,
        -30.0333, -29.8302, -29.5787, -29.2913, -28.9757,
        -28.6294, -28.2559, -27.87, -27.4767, -27.1044,
        -26.7678, -26.4708, -26.2232, -26.0129, -25.8498,
        -25.7593, -25.7475, -25.7789, -25.815, -25.8455,
        -25.8968, -25.9818, -26.0933
    };

    CoreFeatures::OnsetOffsetMap onsetOffsets;
    onsetOffsets[6] = { 57, CoreFeatures::OffsetType::FollowingOnsetReached };
    onsetOffsets[57] = { 90, CoreFeatures::OffsetType::FollowingOnsetReached };

    int begin = 36;
    int end = 61;
    int associatedOnset = 57;
    
    testSingleGlide(pitch_Hz, onsetOffsets, associatedOnset, begin, end);

    pair<int, Glide::Extent> glide { associatedOnset, { begin, end }};
    
    testGlideClassification(glide, onsetOffsets, pitch_Hz, smoothedPower_dB,
                            Portamento::GlideDirection::Ascending,
                            Portamento::GlideRange::Medium,
                            Portamento::GlideDuration::Medium,
                            Portamento::GlideLink::Interconnecting,
                            Portamento::GlideDynamic::Stable);
}


BOOST_AUTO_TEST_SUITE_END()

