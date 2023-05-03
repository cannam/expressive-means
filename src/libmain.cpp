/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Expressive Means Vamp Plugins

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <vamp/vamp.h>
#include <vamp-sdk/PluginAdapter.h>

#include "Onsets.h"
#include "Articulation.h"
#include "PitchVibrato.h"
#include "Portamento.h"

#include "SemanticArticulation.h"
#include "SemanticPitchVibrato.h"
#include "SemanticPortamento.h"

static Vamp::PluginAdapter<Onsets> onsetsPluginAdapter;
static Vamp::PluginAdapter<Articulation> articulationPluginAdapter;
static Vamp::PluginAdapter<PitchVibrato> pitchVibratoPluginAdapter;
static Vamp::PluginAdapter<Portamento> portamentoPluginAdapter;

static Vamp::PluginAdapter<SemanticArticulation> semanticArticulationPluginAdapter;
static Vamp::PluginAdapter<SemanticPitchVibrato> semanticPitchVibratoPluginAdapter;
static Vamp::PluginAdapter<SemanticPortamento> semanticPortamentoPluginAdapter;

const VampPluginDescriptor *
vampGetPluginDescriptor(unsigned int version, unsigned int index)
{
    if (version < 1) return 0;

    switch (index) {
    case  0: return onsetsPluginAdapter.getDescriptor();
    case  1: return articulationPluginAdapter.getDescriptor();
    case  2: return pitchVibratoPluginAdapter.getDescriptor();
    case  3: return portamentoPluginAdapter.getDescriptor();
    case  4: return semanticArticulationPluginAdapter.getDescriptor();
    case  5: return semanticPitchVibratoPluginAdapter.getDescriptor();
    case  6: return semanticPortamentoPluginAdapter.getDescriptor();
    default: return 0;
    }
}


