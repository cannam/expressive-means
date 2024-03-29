
/*
    Expressive Means Semantic Articulation

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "SemanticArticulation.h"

SemanticArticulation::SemanticArticulation(float inputSampleRate) :
    SemanticAdapter<Articulation>
    (inputSampleRate,
     // Output selection (to be passed through)
     { "summary", "noiseType", "volumeDevelopment",
       "articulationType", "articulationIndex" },
     // Parameter selection (passed through, or new)
     { "clef", "instrumentType", "noteDurations",
       "soundQuality", "reverb", "overlap", "normaliseAudio", "pyin-precisetime"
     },
     // Parameter metadata (map)
     { { "clef",
         { "Clef",
           "Clef which is closest to the instrument's pitch range."
         } },
       { "instrumentType",
         { "Signal type",
           "General family of the signal / instrument."
         } },
       { "noteDurations",
         { "Note durations",
           "Indication of the shortest durations found in the recording."
         } },
       { "soundQuality",
         { "Sound quality",
           "Indication of the degree of surface noise in the recording, from 1 (clean, SNR > 50dB) to 6 (extremely noisy, SNR < 6dB)."
         } },
       { "reverb",
         { "Reverb duration",
           "Indication of the reverb duration of the recording space."
         } },
       { "overlap",
         { "Overlap compensation",
           "Whether to compensate automatically for likely overlapping notes."
         } }
     },
     // Named options parameters (map)
     { { "clef",
         { { "Treble",
             { { "spectralFrequencyMin", 100.f },
               { "spectralFrequencyMax", 4000.f },
               { "volumeDevelopmentThreshold", 2.f }
             } },
           { "Alto",
             { { "spectralFrequencyMin", 100.f },
               { "spectralFrequencyMax", 3000.f },
               { "volumeDevelopmentThreshold", 2.f }
             } },
           { "Bass",
             { { "spectralFrequencyMin", 50.f },
               { "spectralFrequencyMax", 2000.f },
               { "volumeDevelopmentThreshold", 3.f }
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
             { { "onsetSensitivityPitch", 25.f },
               { "onsetSensitivityNoise", 35.f },
               { "onsetSensitivityLevel", 8.f },
               { "onsetSensitivityNoiseTimeWindow", 100.f },
               { "onsetSensitivityRawPowerThreshold", 10.f }, 
               { "sustainBeginThreshold", 150.f }, 
               { "spectralDropOffsetRatio", 30.f }
             } },
           { "Vocal (Jazz & Pop)",
             { { "onsetSensitivityPitch", 25.f },
               { "onsetSensitivityNoise", 30.f },
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
       },
       { "reverb",
         { { "Small studio (< 150 ms)",
             { { "noteDurationThreshold", 12.f },
               { "spectralDropFloor", -70.f },
               { "reverbDurationFactor", 1.f }
             } },
           { "Large studio (c. 150-600 ms)",
             { { "noteDurationThreshold", 12.f },
               { "spectralDropFloor", -70.f },
               { "reverbDurationFactor", 1.5f }
             } },
           { "Concert hall (c. 600-1500 ms)",
             { { "noteDurationThreshold", 12.f },
               { "spectralDropFloor", -70.f },
               { "reverbDurationFactor", 2.25f }
             } },
           { "Church (> 1500 ms)",
             { { "noteDurationThreshold", 12.f },
               { "spectralDropFloor", -70.f },
               { "reverbDurationFactor", 3.375f }
             } }
         }
       }
     },
     // Numbered options parameters (map)
     { { "soundQuality",
         {
             { { "impulseNoiseRatioPlosive", 22.f },
               { "impulseNoiseRatioFricative", 11.f }
             },
             { { "impulseNoiseRatioPlosive", 26.f },
               { "impulseNoiseRatioFricative", 13.f }
             },
             { { "impulseNoiseRatioPlosive", 32.f },
               { "impulseNoiseRatioFricative", 16.f }
             },
             { { "impulseNoiseRatioPlosive", 36.f },
               { "impulseNoiseRatioFricative", 27.f }
             },
             { { "impulseNoiseRatioPlosive", 53.f },
               { "impulseNoiseRatioFricative", 47.f }
             },
             { { "impulseNoiseRatioPlosive", 80.f },
               { "impulseNoiseRatioFricative", 80.f }
             }
         } }
     },
     // Toggled parameters (map)
     {
       { "overlap",
         { { "overlapCompensationFactor",
             { 1.f, 1.6f } // off, on
             }
         }
       }
     },
     // Default values. If not specified, named parameters default to
     // the first label, i.e. 0.f, numbered ones default to 1.f, and
     // toggles default to off.
     {
         { "noteDurations", 1.f },
         { "reverb", 1.f },
         { "soundQuality", 2.f },
         { "overlap", 1.f }
     })
    {}
    
string
SemanticArticulation::getIdentifier() const {
    return TAGGED_ID("articulation-semantic");
}

string
SemanticArticulation::getName() const {
    return TAGGED_NAME("Expressive Means: Articulation");
}

string
SemanticArticulation::getDescription() const {
    return "identifies types and intensity of articulation in monophonic recordings"; 
}

string
SemanticArticulation::getMaker() const {
    return m_adapted.getMaker();
}

int
SemanticArticulation::getPluginVersion() const {
    return m_adapted.getPluginVersion();
}

string
SemanticArticulation::getCopyright() const {
    return m_adapted.getCopyright();
}

