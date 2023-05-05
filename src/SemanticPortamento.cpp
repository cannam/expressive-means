
/*
    Expressive Means Semantic Portamento

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "SemanticPortamento.h"

SemanticPortamento::SemanticPortamento(float inputSampleRate) :
    SemanticAdapter<Portamento>
    (inputSampleRate,
     // Output selection (to be passed through)
     { "summary", "portamentoType", "portamentoIndex" },
     // Parameter selection (passed through, or new)
     { "clef", "instrumentType", "noteDurations", "normaliseAudio"
     },
     // Parameter metadata (map)
     { { "clef",
         { "Clef",
           "Clef which is closest to the instrument's pitch range."
         } },
       { "instrumentType",
         { "Instrument type",
           "General family of instrument."
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
         { { "Instrumental",
             { { "onsetSensitivityPitch", 15.f },
               { "onsetSensitivityNoise", 24.f },
               { "onsetSensitivityLevel", 8.f },
               { "onsetSensitivityNoiseTimeWindow", 100.f },
               { "onsetSensitivityRawPowerThreshold", 6.f },
               { "glideThresholdPitch", 60.f },
               { "glideThresholdHopMinimum", 10.f },
               { "glideThresholdHopMaximum", 50.f },
               { "glideThresholdDuration", 50.f },
               { "glideThresholdProximity", 350.f },
               { "linkThreshold", 70.f }
             } },
           { "Voice",
             { { "onsetSensitivityPitch", 15.f },
               { "onsetSensitivityNoise", 40.f },
               { "onsetSensitivityLevel", 8.f },
               { "onsetSensitivityNoiseTimeWindow", 100.f },
               { "onsetSensitivityRawPowerThreshold", 8.f },
               { "glideThresholdPitch", 60.f },
               { "glideThresholdHopMinimum", 10.f },
               { "glideThresholdHopMaximum", 70.f },
               { "glideThresholdDuration", 50.f },
               { "glideThresholdProximity", 500.f },
               { "linkThreshold", 100.f }
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
             { { "minimumOnsetInterval", 60.f },
               { "pitchAverageWindow", 60.f }
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
SemanticPortamento::getIdentifier() const {
    return "portamento-semantic";
}

string
SemanticPortamento::getName() const {
    return "Expressive Means: Portamento";
}

string
SemanticPortamento::getDescription() const {
    return ""; //!!! todo
}

string
SemanticPortamento::getMaker() const {
    return m_adapted.getMaker();
}

int
SemanticPortamento::getPluginVersion() const {
    return m_adapted.getPluginVersion();
}

string
SemanticPortamento::getCopyright() const {
    return m_adapted.getCopyright();
}

