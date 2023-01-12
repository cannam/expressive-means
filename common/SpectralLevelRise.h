
/*
    Expressive Means

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef EXPRESSIVE_MEANS_SPECTRAL_LEVEL_RISE_H
#define EXPRESSIVE_MEANS_SPECTRAL_LEVEL_RISE_H

#include <vamp-sdk/FFT.h>

#include <vector>
#include <cmath>
#include <iostream>
#include <deque>

// Calculate and return the fraction of spectral bins in a given
// frequency range whose magnitudes have risen by more than the given
// ratio within the given number of steps.

class SpectralLevelRise
{
public:
    SpectralLevelRise() : m_initialised(false) {}
    ~SpectralLevelRise() {}

    void initialise(double sampleRate, int blockSize,
                    double fmin, double fmax,
                    double dB, int historyLength) {

        if (!m_sampleRate) {
            throw std::logic_error("SpectralLevelRise::initialise: sampleRate must be non-zero");
        }
        if (!blockSize) {
            throw std::logic_error("SpectralLevelRise::initialise: blockSize must be non-zero");
        }
        if (fmin < 0.0 || fmin >= sampleRate / 2.0) {
            throw std::logic_error("SpectralLevelRise::initialise: fmin is out of range");
        }
        if (fmax < 0.0 || fmax >= sampleRate / 2.0 || fmax < fmin) {
            throw std::logic_error("SpectralLevelRise::initialise: fmax is out of range");
        }
        if (dB <= 0.0) {
            throw std::logic_error("SpectralLevelRise::initialise: dB should be positive (it is a gain ratio)");
        }
        if (historyLength < 2) {
            throw std::logic_error("SpectralLevelRise::initialise: historyLength must be at least 2");
        }
        
        m_sampleRate = sampleRate;
        m_blockSize = blockSize;
        m_fmin = fmin;
        m_fmax = fmax;
        m_dB = dB;
        m_historyLength = historyLength;

        m_binmin = (double(m_blockSize) * m_fmin) / m_sampleRate;
        m_binmax = (double(m_blockSize) * m_fmax) / m_sampleRate;
        m_ratio = pow(10.0, m_dB / 10.0);

        std::cerr << "SpectralLevelRise::initialise: "
                  << "sampleRate " << m_sampleRate
                  << ", blockSize " << m_blockSize
                  << ", fmin " << m_fmin
                  << ", fmax " << m_fmax
                  << ", dB " << m_dB
                  << ", historyLength " << m_historyLength
                  << ", binmin " << m_binmin
                  << ", binmax " << m_binmax
                  << ", ratio " << m_ratio
                  << std::endl;
        
        // Hann window
        m_window.reserve(m_blockSize);
        for (int i = 0; i < m_blockSize; ++i) {
            m_window.push_back(0.5 - 0.5 * cos((2.0 * M_PI * i) / m_blockSize));
        }

        m_initialised = true;
    }

    void process(const float *timeDomain) {
        if (!m_initialised) {
            throw std::logic_error("SpectralLevelRise::process: Not initialised");
        }
        
        std::vector<double> windowed;
        windowed.reserve(m_blockSize);
        for (int i = 0; i < m_blockSize; ++i) {
            windowed.push_back(m_window[i] * timeDomain[i]);
        }

        // No fftshift; we don't use phase
        std::vector<double> ro(m_blockSize, 0.0);
        std::vector<double> io(m_blockSize, 0.0);
        Vamp::FFT::forward(m_blockSize,
                           windowed.data(), nullptr,
                           ro.data(), io.data());

        std::vector<double> magnitudes;
        for (int i = m_binmin; i <= m_binmax; ++i) {
            magnitudes.push_back(sqrt(ro[i] * ro[i] + io[i] * io[i]));
        }

        m_magHistory.push_back(magnitudes);

        if (m_magHistory.size() >= m_historyLength) {
            double fraction = extractFraction();
            m_fractions.push_back(fraction);
            m_magHistory.pop_front();
        }
    }

    int getHistoryLength() const {
        return m_historyLength;
    }
    
    std::vector<double> getFractions() const {
        return m_fractions;
    }

private:
    double m_sampleRate;
    int m_blockSize;
    double m_fmin;
    double m_fmax;
    int m_binmin;
    int m_binmax;
    double m_dB;
    double m_ratio;
    int m_historyLength;
    bool m_initialised;
    std::vector<float> m_window;
    std::deque<std::vector<double>> m_magHistory;
    std::vector<double> m_fractions;

    double extractFraction() const {
        // If, for a given bin i, there is a value anywhere in the
        // magnitude history (m_magHistory[j][i] for some j > 0) that
        // exceeds that at the start of the magnitude history
        // (m_magHistory[0][i]) by the required ratio, then we count
        // that bin toward the total. This may be open to adjustment
        int n = m_binmax - m_binmin + 1;
        int m = m_magHistory.size() - 1;
        if (m < 2) return 0.0;
        int above = 0;
        for (int i = 0; i < n; ++i) {
            for (int j = 1; j < m; ++j) {
                if (m_magHistory[j][i] > m_magHistory[0][i] * m_ratio) {
                    ++above;
                    break;
                }
            }
        }
        return double(above) / double(n);
    }
};

#endif
