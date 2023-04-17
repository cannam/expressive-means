
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

#include <iostream>
using std::cerr;
using std::endl;

BOOST_AUTO_TEST_SUITE(TestGlide)

static void testSingleGlide(const vector<double> &pitch_Hz,
                            int expectedStart,
                            int expectedEnd)
{
    // We don't care about onsets - we are not testing onset linking
    // here - but we must provide at least one onset otherwise no
    // glide will be returned. We actually add three so that we can
    // tell if spurious glides were detected
    CoreFeatures::OnsetOffsetMap onsets;
    onsets[0] = { 50, CoreFeatures::OffsetType::PowerDrop };
    onsets[100] = { 150, CoreFeatures::OffsetType::PowerDrop };
    onsets[200] = { 250, CoreFeatures::OffsetType::PowerDrop };
    
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
    BOOST_TEST(extents.begin()->second.start == expectedStart);
    BOOST_TEST(extents.begin()->second.end == expectedEnd);
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
        1271.45, 1279.99, 1263.85, 1233.46, 1219.25,  // hop 0
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
        1082.16, 1067.72, 1059.89, 1040.02, 1027.41,
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

    testSingleGlide(pitch_Hz, 50, 64);
}

BOOST_AUTO_TEST_CASE(huberman_11987)
{
    vector<double> pitch_Hz {
        542.786, 542.334, 541.468, 540.392, 539.573,
        539.062, 538.734, 538.8, 538.596, 538.058,
        537.425, 536.551, 535.244, 534.497, 533.642,
        533.97, 534.463, 535.035, 536.071, 536.838,
        537.745, 538.828, 539.792, 540.573, 541.443,
        542.074, 542.848, 543.342, 543.994, 544.68,
        545.368, 545.653, 545.379, 543.826, 542.397,
        540.569, 539.168, 537.925, 536.407, 535.623,
        535.019, 534.341, 534.229, 534.048, 533.956,
        534.905, 535.84, 536.732, 537.61, 538.324,
        538.767, 539.467, 539.886, 540.404, 540.549,
        540.564, 540.426, 539.996, 539.735, 539.369,
        539.25, 539.111, 538.67, 538.588, 538.219,
        538.276, 538.58, 538.804, 539.292, 539.438,
        540.005, 540.11, 539.898, 539.623, 539.165,
        538.734, 539.144, 539.402, 539.286, 539.523,
        539.339, 539.421, 539.819, 539.726, 539.505,
        539.137, 538.922, 538.803, 538.771, 538.496,
        537.698, 536.735, 535.95, 535.638, 534.894,
        534.584, 534.15, 533.627, 533.741, 533.384,
        533.174, 532.866, 532.36, 532.029, 531.337,
        531.029, 529.836, 529.441, 528.468, 526.526,
        525.568, 522.919, 520.013, 516.213, 514.148,
        513.009, 507.976, 506.725, 504.916, 502.865,
        503.376, 499.496, 497.435, 493.929, 492.291,
        490.818, 488.565, 486.672, 483.669, 483.882,
        484.405, 479.597, 478.993, 476.937, 474.967,
        475.143, 472.271, 470.696, 470.62, 468.476,
        469.216, 469.49, 467.612, 469.146, 467.137,
        465.891, 465.174, 462.583, 462.679, 461.523,
        461.94, 463.137, 461.494, 460.993, 460.614,
        459.318, 459.926, 458.213, 458.617, 458.696,
        377.788, 376.763, 375.449, 375.17, 375.706,
        376.561, 377.751, 378.103, 377.579, 377.543,
        377.908, 378.476, 379.591, 379.831, 380.738,
        381.761, 381.948, 382.608, 383.059, 383.043,
        383.402, 383.685, 383.467, 383.043, 382.766,
        381.826, 381.019, 380.215, 379.173, 378.749,
        378.268, 378.253, 376.939, 377.905, 376.673,
        376.45, 376.567, 376.377, 376.731, 377.189,
        378.275, 379.154, 380.303, 381.346, 382.217,
        383.259, 384.018, 385.111, 385.804, 386.092,
        385.92, 385.583, 384.851, 384.203, 383.18,
        382.314, 381.999, 381.535, 381.182, 380.774,
        380.22, 379.544, 379.373, 379.015, 378.389,
        378.217, 377.841, 377.451, 377.244, 376.896,
        376.754, 376.904, 377.593, 379.17, 382.755,
        383.604, 383.935, 384.783, 385.119, 385.287,
        385.855, 385.863, 385.952, 385.956, 385.171,
        384.041, 383.251, 383.117, 382.455, 382.536,
        381.349, 380.294, 379.677, 379.56, 379.497,
        379.011
    };

    testSingleGlide(pitch_Hz, 84, 159);
}

BOOST_AUTO_TEST_SUITE_END()

