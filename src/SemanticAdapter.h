/*
    Expressive Means

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef EXPRESSIVE_MEANS_SEMANTIC_ADAPTER_H
#define EXPRESSIVE_MEANS_SEMANTIC_ADAPTER_H

#include <vamp-sdk/Plugin.h>

#include <set>
#include <map>
#include <vector>
#include <cmath>

//#define DEBUG_SEMANTIC_ADAPTER 1

using std::string;

template <class Adapted>
class SemanticAdapter : public Vamp::Plugin
{
protected:
    struct ParameterRec {
        string name;
        string description;
    };

    typedef std::map<string, ParameterRec>
    ParameterMetadata;
    
    // e.g. "Instrument Type" -> "Strings" -> "onsetSensitivityPitch" -> 15.f
    // NB vector<pair> instead of <map> for value labels because order matters
    typedef std::map<string, std::vector<std::pair<string, std::map<string, float>>>>
    NamedOptionsParameters;

    typedef std::map<string, std::vector<std::map<string, float>>>
    NumberedOptionsParameters;

    typedef std::map<string, std::map<string, std::pair<float, float>>>
    ToggleParameters;

    typedef std::vector<string> // in returned order
    IdSelection;

    typedef std::set<string> // for lookup
    IdSet;

    typedef std::map<string, float>
    ValueMap;
    
    SemanticAdapter(float inputSampleRate,
                    IdSelection outputSelection,
                    IdSelection parameterSelection,
                    ParameterMetadata parameterMetadata,
                    NamedOptionsParameters namedOptionsParameters,
                    NumberedOptionsParameters numberedOptionsParameters,
                    ToggleParameters toggleParameters,
                    ValueMap parameterDefaults) :
        Plugin(inputSampleRate),
        m_adapted(inputSampleRate),
        m_outputSelection(outputSelection),
        m_outputSet(outputSelection.begin(), outputSelection.end()),
        m_parameterSelection(parameterSelection),
        m_parameterMetadata(parameterMetadata),
        m_namedOptionsParameters(namedOptionsParameters),
        m_numberedOptionsParameters(numberedOptionsParameters),
        m_toggleParameters(toggleParameters),
        m_semanticParameterDefaults(parameterDefaults),
        m_semanticParameterValues(parameterDefaults)
    {
        for (auto pm : m_parameterMetadata) {
            if (m_semanticParameterValues.find(pm.first) ==
                m_semanticParameterValues.end()) {
                if (m_numberedOptionsParameters.find(pm.first) !=
                    m_numberedOptionsParameters.end()) {
                    m_semanticParameterValues[pm.first] = 1.f;
                } else {
                    m_semanticParameterValues[pm.first] = 0.f;
                }
            }
        }
    }
    
public:
    virtual ~SemanticAdapter() { } 

    InputDomain getInputDomain() const { return m_adapted.getInputDomain(); }
    size_t getPreferredBlockSize() const { return m_adapted.getPreferredBlockSize(); }
    size_t getPreferredStepSize() const { return m_adapted.getPreferredStepSize(); }
    size_t getMinChannelCount() const { return m_adapted.getMinChannelCount(); }
    size_t getMaxChannelCount() const { return m_adapted.getMaxChannelCount(); }

    ParameterList getParameterDescriptors() const {
        ParameterList upstream = m_adapted.getParameterDescriptors();
        std::map<string, int> upmap;
        for (int i = 0; i < int(upstream.size()); ++i) {
            upmap[upstream.at(i).identifier] = i;
        }
        ParameterList list;
        for (auto id : m_parameterSelection) {
            // If a parameter is named in the selection but does not
            // appear in the metadata set as a semantic parameter,
            // that means we should pass it through directly from the
            // wrapped plugin
            if (m_parameterMetadata.find(id) == m_parameterMetadata.end()) {
                if (upmap.find(id) == upmap.end()) {
                    throw std::logic_error("Parameter in selection is not found in metadata or upstream: " + id);
                }
                list.push_back(upstream.at(upmap.at(id)));
            } else {
                // Parameter appears in the metadata set: it's a
                // semantic parameter
                bool named =
                    (m_namedOptionsParameters.find(id) !=
                     m_namedOptionsParameters.end());
                bool numbered = 
                    (m_numberedOptionsParameters.find(id) !=
                     m_numberedOptionsParameters.end());
                bool toggled = 
                    (m_toggleParameters.find(id) !=
                     m_toggleParameters.end());
                if (!named && !numbered && !toggled) {
                    throw std::logic_error("Parameter in metadata is not found in named, numbered, or toggled: " + id);
                }
                if ((named && numbered) || (named && toggled) || (numbered && toggled)) {
                    throw std::logic_error("Parameter in metadata appears in more than one type map: " + id);
                }
                ParameterDescriptor d;
                d.identifier = id;
                d.name = m_parameterMetadata.at(id).name;
                d.description = m_parameterMetadata.at(id).description;
                d.unit = "";
                d.isQuantized = true;
                d.quantizeStep = 1.f;
                std::set<string> upstreamParamsUsed;
                if (named) {
                    d.minValue = 0.f;
                    d.defaultValue = 0.f;
                    d.maxValue = int(m_namedOptionsParameters.at(id).size()) - 1;
                    for (const auto &v : m_namedOptionsParameters.at(id)) {
                        d.valueNames.push_back(v.first);
                        for (const auto &ppv : v.second) {
                            upstreamParamsUsed.insert(ppv.first);
                        }
                    }
                } else if (numbered) {
                    d.minValue = 1.f;
                    d.defaultValue = 1.f;
                    d.maxValue = int(m_numberedOptionsParameters.at(id).size());
                    for (const auto &v : m_numberedOptionsParameters.at(id)) {
                        for (const auto &ppv : v) {
                            upstreamParamsUsed.insert(ppv.first);
                        }
                    }
                    d.valueNames.clear();
                } else if (toggled) {
                    d.minValue = 0.f;
                    d.defaultValue = 1.f;
                    d.maxValue = 1.f;
                    for (const auto &ppv : m_toggleParameters.at(id)) {
                        upstreamParamsUsed.insert(ppv.first);
                    }
                    d.valueNames.clear();
                }                    
                if (m_semanticParameterDefaults.find(id) !=
                    m_semanticParameterDefaults.end()) {
                    d.defaultValue = m_semanticParameterDefaults.at(id);
                }
                for (auto p : upstreamParamsUsed) {
                    if (upmap.find(p) == upmap.end()) {
                        throw std::logic_error("Parameter: " + id + " refers to nonexistent upstream parameter: " + p);
                    }
                }
                list.push_back(d);
            }
        }
        return list;
    }
    
    float getParameter(string id) const {
        if (m_parameterMetadata.find(id) != m_parameterMetadata.end()) {
            // It's a semantic parameter
            return m_semanticParameterValues.at(id);
        } else {
            // It's just passed through
            return m_adapted.getParameter(id);
        }
    }
    
    void setParameter(string id, float value) {
        if (m_parameterMetadata.find(id) != m_parameterMetadata.end()) {
            // It's a semantic parameter
            m_semanticParameterValues[id] = value;
        } else {
            // It's just passed through
            m_adapted.setParameter(id, value);
        }
    }

    OutputList getOutputDescriptors() const {
        OutputList upstream = m_adapted.getOutputDescriptors();
        OutputList list;
        IdSet found;
        for (int i = 0; i < int(upstream.size()); ++i) {
            const auto &out = upstream.at(i);
            auto id = out.identifier;
            found.insert(id);
            if (m_outputSet.find(id) == m_outputSet.end()) {
                continue;
            }
            m_outputIndicesHere[id] = int(list.size());
            m_outputIndicesThere[id] = i;
            list.push_back(out);
        }
        for (auto out : m_outputSelection) {
            if (found.find(out) == found.end()) {
                throw std::logic_error("Output not found upstream: " + out);
            }
        }
        return list;
    }

    bool initialise(size_t channels, size_t stepSize, size_t blockSize) {

        for (auto pv : m_semanticParameterValues) {
            string id = pv.first;
            int v = int(roundf(pv.second));
            if (m_namedOptionsParameters.find(id) !=
                m_namedOptionsParameters.end()) {
                int n = int(m_namedOptionsParameters.at(id).size());
                if (v < 0 || v >= n) {
                    std::cerr << "WARNING: parameter " << id << " value " << v
                              << " is out of name range 0-" << n-1 << std::endl;
                } else {
                    for (auto ppv : m_namedOptionsParameters.at(id).at(v).second) {
#ifdef DEBUG_SEMANTIC_ADAPTER
                        std::cerr << "[named] " << ppv.first << " -> "
                                  << ppv.second << std::endl;
#endif
                        m_adapted.setParameter(ppv.first, ppv.second);
                    }
                }
            } else if (m_numberedOptionsParameters.find(id) !=
                       m_numberedOptionsParameters.end()) {
                int n = int(m_numberedOptionsParameters.at(id).size());
                if (v < 1 || v > n) {
                    std::cerr << "WARNING: parameter " << id << " value " << v
                              << " is out of range 1-" << n << std::endl;
                } else {
                    for (auto ppv : m_numberedOptionsParameters.at(id).at(v-1)) {
#ifdef DEBUG_SEMANTIC_ADAPTER
                        std::cerr << "[numbered] " << ppv.first << " -> "
                                  << ppv.second << std::endl;
#endif
                        m_adapted.setParameter(ppv.first, ppv.second);
                    }
                }
            } else if (m_toggleParameters.find(id) !=
                       m_toggleParameters.end()) {
                for (auto ppv : m_toggleParameters.at(id)) {
                    float value =
                        (v < 0.5 ? ppv.second.first : ppv.second.second);
#ifdef DEBUG_SEMANTIC_ADAPTER
                    std::cerr << "[toggled] " << ppv.first << " -> "
                              << value << std::endl;
#endif
                    m_adapted.setParameter(ppv.first, value);
                }                    
            } else {
                throw std::logic_error("Parameter in semantic parameter values not found in named, numbered, or toggled: " + id);
            }
        }
        
        if (m_adapted.initialise(channels, stepSize, blockSize)) {
            (void)getOutputDescriptors();
            return true;
        } else {
            return false;
        }
    }
    
    void reset() {
        m_adapted.reset();
    }

    FeatureSet process(const float *const *inputBuffers,
                       Vamp::RealTime timestamp) {
        FeatureSet upstream = m_adapted.process(inputBuffers, timestamp);
        return selectFeatures(upstream);
    }

    FeatureSet getRemainingFeatures() {
        return selectFeatures(m_adapted.getRemainingFeatures());
    }

protected:
    Adapted m_adapted;

    const IdSelection m_outputSelection;
    const IdSet m_outputSet;
    const IdSelection m_parameterSelection;
    const ParameterMetadata m_parameterMetadata;
    const NamedOptionsParameters m_namedOptionsParameters;
    const NumberedOptionsParameters m_numberedOptionsParameters;
    const ToggleParameters m_toggleParameters;
    mutable std::map<string, int> m_outputIndicesHere;
    mutable std::map<string, int> m_outputIndicesThere;
    const std::map<string, float> m_semanticParameterDefaults;
    std::map<string, float> m_semanticParameterValues;

    FeatureSet selectFeatures(const FeatureSet &upstream) {
        FeatureSet selection;
        for (auto id : m_outputSelection) {
            if (upstream.find(m_outputIndicesThere.at(id)) != upstream.end()) {
                selection[m_outputIndicesHere.at(id)] =
                    upstream.at(m_outputIndicesThere.at(id));
            }
        }
        return selection;
    }
};

#endif
