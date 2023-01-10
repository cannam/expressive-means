
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

// Filtered power calculation, somewhat like Mazurka MzPowerCurve's
// smoothedpower output

class Power
{
public:
    Power() :
        m_stepSize(0),
        m_blockSize(0),
        m_filterLength(0),
        m_threshold(0.0) {}
    ~Power() {}

    void initialise(size_t stepSize, size_t blockSize, size_t filterLength,
                    double threshold_dB) {
        m_stepSize = stepSize;
        m_blockSize = blockSize;
        m_filterLength = filterLength;
        m_threshold = pow(10.0, threshold_dB / 10.0);
    }

    void process(const float *input) {
        if (!m_stepSize || !m_blockSize || m_threshold == 0.0) {
            throw std::logic_error("Not properly initialised");
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
    size_t m_stepSize;
    size_t m_blockSize;
    size_t m_filterLength;
    double m_threshold;
    std::vector<double> m_rawPower;
};

#endif
