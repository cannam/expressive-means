
/*
    Expressive Means

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef EXPRESSIVE_MEANS_POWER_H
#define EXPRESSIVE_MEANS_POWER_H

#include "../ext/pyin/MeanFilter.h"

#include <vector>
#include <cmath>
#include <iostream>

/** Filtered power calculation, somewhat like Mazurka MzPowerCurve's
 *  smoothedpower output
 */
class Power
{
public:
    Power() : m_initialised(false) {}
    ~Power() {}

    struct Parameters {
        int blockSize;
        int filterLength;
        double threshold_dB;
        Parameters() :
            blockSize(2048),
            filterLength(18),
            threshold_dB(-120.0) { }
    };

    void initialise(Parameters parameters) {
        if (parameters.blockSize < 1) {
            std::cerr << "Power::initialise: invalid blockSize "
                      << parameters.blockSize << std::endl;
            throw std::logic_error("Power::initialise: blockSize must be > 0");
        }            
        if (parameters.filterLength < 1) {
            std::cerr << "Power::initialise: invalid filterLength "
                      << parameters.filterLength << std::endl;
            throw std::logic_error("Power::initialise: filterLength must be > 0");
        }
        m_blockSize = parameters.blockSize;
        m_filterLength = parameters.filterLength;
        m_threshold = pow(10.0, parameters.threshold_dB / 10.0);
        m_initialised = true;
    }

    void reset() {
        if (!m_initialised) {
            throw std::logic_error("Power::reset: Never initialised");
        }
        m_rawPower.clear();
    }
    
    void process(const float *input) {
        if (!m_initialised) {
            throw std::logic_error("Power::process: Not initialised");
        }
        
        double sum = 0.0;
        for (size_t i = 0; i < m_blockSize; ++i) {
            sum += input[i] * input[i];
        }
        if (sum < m_threshold) {
            sum = m_threshold;
        }
        double dB = 10.0 * log10(sum / double(m_blockSize));
        m_rawPower.push_back(dB);
    }

    std::vector<double> getRawPower() const {
        return m_rawPower;
    }

    std::vector<double> getSmoothedPower() const {
        size_t n = m_rawPower.size();
        MeanFilter filter(m_filterLength);
        std::vector<double> smoothed(n, 0.0);
        filter.filter(m_rawPower.data(), smoothed.data(), int(n));
        return smoothed;
    }
    
private:
    size_t m_blockSize;
    int m_filterLength;
    double m_threshold;
    bool m_initialised;
    std::vector<double> m_rawPower;
};

#endif
