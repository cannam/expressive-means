
/*
    Expressive Means 

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef BOOST_TEST_DYN_LINK
#define BOOST_TEST_DYN_LINK
#endif
#include <boost/test/unit_test.hpp>

#include "../src/Glide.h"

#include <iostream>
using std::cerr;
using std::endl;

BOOST_AUTO_TEST_SUITE(TestGlide)

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
    BOOST_TEST(extents.begin()->second.start == 50);
    BOOST_TEST(extents.begin()->second.end == 64);
}

BOOST_AUTO_TEST_SUITE_END()

