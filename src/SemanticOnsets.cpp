
/*
    Expressive Means Semantic Onsets

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "SemanticOnsets.h"

SemanticOnsets::SemanticOnsets(float inputSampleRate) :
    SemanticAdapter<Onsets>
    (inputSampleRate,
     // Output selection (to be passed through)
     { "onsets", "durations" },
     // Parameter selection (passed through, or new)
     { "clef", "instrumentType", "noteDurations", "normaliseAudio", "pyin-precisetime"
     },
     // Parameter metadata (map)
     { { "clef",
         { "Clef",
           "Clef which is closest to the instrument's pitch range."
         } },
       { "instrumentType",
         { "Signal type",
           "General family of signal / instrument."
         } },
       { "noteDurations",
         { "Note durations",
           "Indication of the shortest durations found in the recording."
         } }
     },
     // Named options parameters (map)
     { { "clef",
         { { "Treble",
             { { "spectralFrequencyMin", 100.f },
               { "spectralFrequencyMax", 4000.f }
             } },
           { "Alto",
             { { "spectralFrequencyMin", 100.f },
               { "spectralFrequencyMax", 3000.f }
             } },
           { "Bass",
             { { "spectralFrequencyMin", 50.f },
               { "spectralFrequencyMax", 2000.f }
             } }
         }
        },
       { "instrumentType",
         { { "Bowed Strings",
             { { "onsetSensitivityPitch", 15.f },
               { "onsetSensitivityNoise", 17.f },
               { "onsetSensitivityLevel", 8.f },
               { "onsetSensitivityNoiseTimeWindow", 100.f },
               { "onsetSensitivityRawPowerThreshold", 6.f }
             } },
           { "Vocal (Classical)",
             { { "onsetSensitivityPitch", 20.f },
               { "onsetSensitivityNoise", 35.f },
               { "onsetSensitivityLevel", 8.f },
               { "onsetSensitivityNoiseTimeWindow", 100.f },
               { "onsetSensitivityRawPowerThreshold", 10.f }, 
               { "sustainBeginThreshold", 150.f }, 
               { "spectralDropOffsetRatio", 30.f }
             } },
           { "Vocal (Jazz & Pop)",
             { { "onsetSensitivityPitch", 20.f },
               { "onsetSensitivityNoise", 25.f },
               { "onsetSensitivityLevel", 8.f },
               { "onsetSensitivityNoiseTimeWindow", 100.f },
               { "onsetSensitivityRawPowerThreshold", 15.f },
               { "sustainBeginThreshold", 100.f }, 
               { "spectralDropOffsetRatio", 20.f }
             } },
           { "Piano / Plugged Strings",
             { { "pyin-threshdistr", 1.f },
               { "pyin-lowampsuppression", 1.f },
               { "usePitchOnsetDetector", 0.f },
               { "onsetSensitivityNoise", 5.f },
               { "onsetSensitivityLevel", 9.f },
               { "onsetSensitivityNoiseTimeWindow", 100.f },
               { "onsetSensitivityRawPowerThreshold", 15.f }, 
               { "noteDurationThreshold", 15.f },               
               { "spectralDropFloor", -70.f },
               { "spectralDropOffsetRatio", 20.f }
             } },
           { "Piano / Plugged Strings (historical)",
             { { "pyin-threshdistr", 1.f },
               { "pyin-lowampsuppression", 1.f },
               { "usePitchOnsetDetector", 0.f },
               { "onsetSensitivityNoise", 8.f },
               { "onsetSensitivityLevel", 10.f },
               { "onsetSensitivityNoiseTimeWindow", 60.f },
               { "onsetSensitivityRawPowerThreshold", 15.f },
               { "noteDurationThreshold", 15.f },               
               { "spectralDropFloor", -70.f },
               { "spectralDropOffsetRatio", 20.f }
             } },
           { "Wind / Organ",
             { { "onsetSensitivityPitch", 10.f },
               { "onsetSensitivityNoise", 6.f },
               { "onsetSensitivityLevel", 8.f },
               { "onsetSensitivityNoiseTimeWindow", 100.f },
               { "onsetSensitivityRawPowerThreshold", 12.f }
             } },
           { "Percussion",
             { { "pyin-threshdistr", 1.f },
               { "pyin-lowampsuppression", 1.f },
               { "usePitchOnsetDetector", 0.f },
               { "onsetSensitivityNoise", 4.f },
               { "onsetSensitivityLevel", 8.f },
               { "onsetSensitivityNoiseTimeWindow", 50.f },
               { "onsetSensitivityRawPowerThreshold", 80.f },
               { "sustainBeginThreshold", 0.f },
               { "spectralDropFloor", -70.f },
               { "spectralDropOffsetRatio", 20.f }
             } }                  
         }
        },
       { "noteDurations",
         { { "Long (> 300 ms)",
             { { "minimumOnsetInterval", 280.f },
               { "pitchAverageWindow", 200.f }
             } },
           { "Moderate (150-300 ms)",
             { { "minimumOnsetInterval", 150.f },
               { "pitchAverageWindow", 150.f }
             } },
           { "Short (< 150 ms)",
             { { "minimumOnsetInterval", 50.f },
               { "pitchAverageWindow", 50.f }
             } }
          }
       }
     },
     // Numbered options parameters (map)
     {
     },
     // Toggled parameters (map)
     {
     },
     // Default values. If not specified, named parameters default to
     // the first label, i.e. 0.f, numbered ones default to 1.f, and
     // toggles default to off.
     {
         { "noteDurations", 1.f }
     })
    {}
    
string
SemanticOnsets::getIdentifier() const {
    return TAGGED_ID("onsets-semantic");
}

string
SemanticOnsets::getName() const {
    return TAGGED_NAME("Expressive Means");
}

string
SemanticOnsets::getDescription() const {
    return "finds note onsets and durations in monophonic recordings based on changes in spectral content, power, and pitch";
}

string
SemanticOnsets::getMaker() const {
    return m_adapted.getMaker();
}

int
SemanticOnsets::getPluginVersion() const {
    return m_adapted.getPluginVersion();
}

string
SemanticOnsets::getCopyright() const {
    return m_adapted.getCopyright();
}

