
/*
    Expressive Means Articulation

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <boost/test/unit_test.hpp>

#include "../src/Articulation.h"

#include <iostream>

std::ostream &operator<<(std::ostream &os, Articulation::LevelDevelopment ld)
{
    os << Articulation::developmentToString(ld);
    return os;
}

BOOST_AUTO_TEST_SUITE(TestArticulation)

BOOST_AUTO_TEST_CASE(volumeDevelopment)
{
    BOOST_CHECK(Articulation::classifyLevelDevelopment
                (-20.0, // begin
                 -20.0, // end
                 -20.0, // max
                 -20.0, // min
                 2.0) // threshold
                == Articulation::LevelDevelopment::Constant);
    
    BOOST_CHECK(Articulation::classifyLevelDevelopment
                (-20.0, // begin
                 -21.0, // end
                 -21.5, // max
                 -20.0, // min
                 2.0) // threshold
                == Articulation::LevelDevelopment::Constant);
    
    BOOST_CHECK(Articulation::classifyLevelDevelopment
                (-20.0, // begin
                 -21.5, // end
                 -21.0, // max
                 -20.0, // min
                 2.0) // threshold
                == Articulation::LevelDevelopment::Constant);

    BOOST_CHECK(Articulation::classifyLevelDevelopment
                (-23.0, // begin
                 -20.0, // end
                 -16.5, // max
                 -20.0, // min
                 2.0) // threshold
                 == Articulation::LevelDevelopment::InAndDecreasing);

    BOOST_CHECK(Articulation::classifyLevelDevelopment
                (-23.0, // begin
                 -18.0, // end
                 -16.5, // max
                 -20.0, // min
                 2.0) // threshold
                == Articulation::LevelDevelopment::Increasing);
    
    BOOST_CHECK(Articulation::classifyLevelDevelopment
                (-22.0, // begin
                 -27.2, // end
                 -22.0, // max
                 -31.0, // min
                 2.0) // threshold
                == Articulation::LevelDevelopment::DeAndIncreasing);
    
    BOOST_CHECK(Articulation::classifyLevelDevelopment
                (-22.0, // begin
                 -27.2, // end
                 -22.0, // max
                 -29.0, // min
                 2.0) // threshold
                == Articulation::LevelDevelopment::Decreasing);
    
    BOOST_CHECK(Articulation::classifyLevelDevelopment
                (-22.0, // begin
                 -27.2, // end
                 -19.0, // max
                 -31.0, // min
                 2.0) // threshold
                == Articulation::LevelDevelopment::Other);
}

BOOST_AUTO_TEST_SUITE_END()
