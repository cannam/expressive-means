
/*
    Expressive Means Semantic Articulation

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef EXPRESSIVE_MEANS_SEMANTIC_ARTICULATION_H
#define EXPRESSIVE_MEANS_SEMANTIC_ARTICULATION_H

#include "SemanticAdapter.h"
#include "Articulation.h"
#include "CoreFeatures.h"

class SemanticArticulation : public SemanticAdapter<Articulation>
{
public:
    SemanticArticulation(float inputSampleRate) :
        SemanticAdapter<Articulation>
        (inputSampleRate,
         { "summary", "noiseType", "volumeDevelopment",
           "articulationType", "articulationIndex" },
         { "clef", "instrumentType", "noteDurations",
           "soundQuality"
//           , "reverbDuration", "overlapCompensation"
         },
         { { "clef", { "Clef (pitch range)", "" } }, //!!! todo: descriptions
            { "instrumentType", { "Instrument type", "" } },
            { "noteDurations", { "Note durations (shortest)", "" } },
            { "soundQuality", { "Sound quality", "" } }
//            ,
//            { "reverbDuration", { "Reverb duration", "" } },
//            { "overlapCompensation", { "Overlap compensation", "" } }
         },
         { { "clef",
                { { "Treble",
                        { { "spectralMin", 196.f },
                            { "spectralMax", 4000.f },
                            { "sustainBeginThreshold", 50.f },
                            { "volumeDevelopmentThreshold", 2.f }
                        } },
                  { "Alto",
                      { { "spectralMin", 130.f },
                          { "spectralMax", 3000.f },
                          { "sustainBeginThreshold", 60.f },
                          { "volumeDevelopmentThreshold", 2.f }
                        } },
                  { "Bass",
                      { { "spectralMin", 65.f },
                          { "spectralMax", 2000.f },
                          { "sustainBeginThreshold", 70.f },
                          { "volumeDevelopmentThreshold", 3.f }
                      } }
                }
            },
            { "instrumentType",
              { { "Strings",
                  { { "onsetSensitivityPitch", 15.f },
                    { "onsetSensitivityNoise", 24.f },
                    { "onsetSensitivityLevel", 8.f },
                    { "onsetSensitivityNoiseTimeWindow", 100.f },
                    { "onsetSensitivityRawPowerThreshold", 6.f }
                  } },
                { "Voice",
                  { { "onsetSensitivityPitch", 15.f },
                    { "onsetSensitivityNoise", 40.f },
                    { "onsetSensitivityLevel", 8.f },
                    { "onsetSensitivityNoiseTimeWindow", 100.f },
                    { "onsetSensitivityRawPowerThreshold", 8.f }
                  } },
                { "Keys",
                  { { "onsetSensitivityPitch", 90.f },
                    { "onsetSensitivityNoise", 18.f },
                    { "onsetSensitivityLevel", 8.f },
                    { "onsetSensitivityNoiseTimeWindow", 100.f },
                    { "onsetSensitivityRawPowerThreshold", 15.f }
                  } },
                { "Wind",
                  { { "onsetSensitivityPitch", 10.f },
                    { "onsetSensitivityNoise", 30.f },
                    { "onsetSensitivityLevel", 8.f },
                    { "onsetSensitivityNoiseTimeWindow", 100.f },
                    { "onsetSensitivityRawPowerThreshold", 12.f }
                  } },
                { "Percussion",
                  { { "onsetSensitivityPitch", 100.f },
                    { "onsetSensitivityNoise", 8.f },
                    { "onsetSensitivityLevel", 8.f },
                    { "onsetSensitivityNoiseTimeWindow", 50.f },
                    { "onsetSensitivityRawPowerThreshold", 50.f }
                  } }                  
              }
            },
            { "noteDurations",
                { { "Long (> 300 ms)",
                    { { "minimumOnsetInterval", 250.f },
                      { "pitchAverageWindow", 200.f }
                    } },
                    { "Moderate (150-300 ms)",
                      { { "minimumOnsetInterval", 100.f },
                        { "pitchAverageWindow", 150.f }
                      } },
                    { "Short (< 150 ms)",
                      { { "minimumOnsetInterval", 60.f },
                        { "pitchAverageWindow", 60.f }
                      } }
                }
            }
         },
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
                    { { "impulseNoiseRatioPlosive", 34.f },
                        { "impulseNoiseRatioFricative", 20.f }
                    },
                    { { "impulseNoiseRatioPlosive", 36.f },
                        { "impulseNoiseRatioFricative", 27.f }
                    },
                    { { "impulseNoiseRatioPlosive", 53.f },
                        { "impulseNoiseRatioFricative", 47.f }
                    }
                }
            }
         },
         // Defaults, where non-zero:
         {
             { "noteDurations", 1.f },
             { "soundQuality", 2.f }
         })
    {}
        
    virtual ~SemanticArticulation() {
    }

    string getIdentifier() const {
        return "articulation-semantic";
    }
    string getName() const {
        return "Expressive Means (semantic presets): Articulation";
    }
    string getDescription() const {
        return ""; //!!! todo
    }
    string getMaker() const {
        return m_adapted.getMaker();
    }
    int getPluginVersion() const {
        return m_adapted.getPluginVersion();
    }
    string getCopyright() const {
        return m_adapted.getCopyright();
    }
};

#endif
