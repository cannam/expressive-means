
/*
    Expressive Means Semantic Portamento

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef EXPRESSIVE_MEANS_SEMANTIC_PORTAMENTO_H
#define EXPRESSIVE_MEANS_SEMANTIC_PORTAMENTO_H

#include "SemanticAdapter.h"
#include "Portamento.h"
#include "CoreFeatures.h"

class SemanticPortamento : public SemanticAdapter<Portamento>
{
public:
    SemanticPortamento(float inputSampleRate);
    virtual ~SemanticPortamento() {}

    string getIdentifier() const;
    string getName() const;
    string getDescription() const;
    string getMaker() const;
    int getPluginVersion() const;
    string getCopyright() const;
};

#endif
