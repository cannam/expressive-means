
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
#include <array>
#include <map>

/** Calculate and return the fraction of spectral bins in a given
 *  frequency range whose magnitudes have risen by more than the given
 *  ratio within the given number of steps.
 */
class SpectralLevelRise
{
public:
    SpectralLevelRise() : m_initialised(false) {}
    ~SpectralLevelRise() {}

    struct Parameters {
        double sampleRate;
        int blockSize;
        double frequencyMin_Hz;
        double frequencyMax_Hz;
        double rise_dB;
        double noiseFloor_dB;
        double offset_dB;
        int historyLength;
        Parameters() :
            sampleRate(48000.0),
            blockSize(2048),
            frequencyMin_Hz(100.0),
            frequencyMax_Hz(4000.0),
            rise_dB(20.0),
            noiseFloor_dB(-70.0),
            offset_dB(-70.0),
            historyLength(20)
        {}
    };
    
    void initialise(Parameters parameters) {

        if (!parameters.sampleRate) {
            throw std::logic_error("SpectralLevelRise::initialise: sampleRate must be non-zero");
        }
        if (!parameters.blockSize) {
            throw std::logic_error("SpectralLevelRise::initialise: blockSize must be non-zero");
        }
        if (parameters.frequencyMin_Hz < 0.0 ||
            parameters.frequencyMax_Hz >= parameters.sampleRate / 2.0) {
            std::cerr << "SpectralLevelRise::initialise: min frequency ("
                      << parameters.frequencyMin_Hz
                      << ") is outside range 0.0 - "
                      << parameters.sampleRate / 2.0
                      << " (for sample rate "
                      << parameters.sampleRate << ")" << std::endl;
            throw std::logic_error("SpectralLevelRise::initialise: min frequency is out of range");
        }
        if (parameters.frequencyMax_Hz < 0.0 ||
            parameters.frequencyMax_Hz >= parameters.sampleRate / 2.0 ||
            parameters.frequencyMax_Hz < parameters.frequencyMin_Hz) {
            if (parameters.frequencyMax_Hz < parameters.frequencyMin_Hz) {
                std::cerr << "SpectralLevelRise::initialise: fmax ("
                          << parameters.frequencyMax_Hz
                          << ") is less than fmin ("
                          << parameters.frequencyMin_Hz << ")"
                          << std::endl;
            } else {
                std::cerr << "SpectralLevelRise::initialise: max frequency ("
                          << parameters.frequencyMax_Hz
                          << ") is outside range 0.0 - "
                          << parameters.sampleRate / 2.0
                          << " (for sample rate "
                          << parameters.sampleRate << ")" << std::endl;
            }
            throw std::logic_error("SpectralLevelRise::initialise: max frequency is out of range");
        }
        if (parameters.rise_dB <= 0.0) {
            std::cerr << "SpectralLevelRise::initialise: rise dB ("
                      << parameters.rise_dB
                      << ") should be positive" << std::endl;
            throw std::logic_error("SpectralLevelRise::initialise: rise dB should be positive (it is a gain ratio)");
        }
        if (parameters.noiseFloor_dB > 0.0) {
            std::cerr << "SpectralLevelRise::initialise: noise floor dB ("
                      << parameters.noiseFloor_dB
                      << ") is expected to be negative" << std::endl;
            throw std::logic_error("SpectralLevelRise::initialise: noise floor dB is expected to be negative (it is a signal level)");
        }
        if (parameters.offset_dB > 0.0) {
            std::cerr << "SpectralLevelRise::initialise: offset dB ("
                      << parameters.offset_dB
                      << ") is expected to be negative" << std::endl;
            throw std::logic_error("SpectralLevelRise::initialise: offset dB is expected to be negative (it is a signal level)");
        }
        if (parameters.historyLength < 2) {
            std::cerr << "SpectralLevelRise::initialise: historyLength ("
                      << parameters.historyLength
                      << ") must be at least 2" << std::endl;
            throw std::logic_error("SpectralLevelRise::initialise: historyLength must be at least 2");
        }

        m_parameters = parameters;

        m_binmin =
            (double(m_parameters.blockSize) * m_parameters.frequencyMin_Hz) /
            m_parameters.sampleRate;
        m_binmax =
            (double(m_parameters.blockSize) * m_parameters.frequencyMax_Hz) /
            m_parameters.sampleRate;
        m_rise_ratio = pow(10.0, m_parameters.rise_dB / 10.0);
        m_noiseFloor_mag = pow(10.0, m_parameters.noiseFloor_dB / 20.0);
        m_offset_mag = pow(10.0, m_parameters.offset_dB / 20.0);

        // Hann window
        m_window.reserve(m_parameters.blockSize);
        for (int i = 0; i < m_parameters.blockSize; ++i) {
            m_window.push_back(0.5 - 0.5 * cos((2.0 * M_PI * i) /
                                               m_parameters.blockSize));
        }

        m_initialised = true;
    }

    void reset() {
        if (!m_initialised) {
            throw std::logic_error("SpectralLevelRise::reset: Never initialised");
        }

        m_magHistory.clear();
        m_fractions.clear();
    }
    
    void process(const float *timeDomain) {
        if (!m_initialised) {
            throw std::logic_error("SpectralLevelRise::process: Not initialised");
        }
        
        std::vector<double> windowed;
        windowed.reserve(m_parameters.blockSize);
        for (int i = 0; i < m_parameters.blockSize; ++i) {
            windowed.push_back(m_window[i] * timeDomain[i]);
        }

        // No fftshift; we don't use phase
        std::vector<double> ro(m_parameters.blockSize, 0.0);
        std::vector<double> io(m_parameters.blockSize, 0.0);
        Vamp::FFT::forward(m_parameters.blockSize,
                           windowed.data(), nullptr,
                           ro.data(), io.data());

        std::vector<double> magnitudes;
        std::vector<int> aboveNoiseFloor, aboveOffset;
        for (int i = m_binmin; i <= m_binmax; ++i) {
            double mag = sqrt(ro[i] * ro[i] + io[i] * io[i]);
            mag /= double(m_parameters.blockSize);
            magnitudes.push_back(mag);
            if (mag > m_noiseFloor_mag) {
                aboveNoiseFloor.push_back(i);
            }
            if (mag > m_offset_mag) {
                aboveOffset.push_back(i);
            }
        }

        m_binsAboveNoiseFloor.push_back(aboveNoiseFloor);
        m_binsAboveOffset.push_back(aboveOffset);
        m_magHistory.push_back(magnitudes);

        if (int(m_magHistory.size()) >= m_parameters.historyLength) {
            double fraction = extractFraction();
            m_fractions.push_back(fraction);
            m_magHistory.pop_front();
        }
    }

    int getHistoryLength() const {
        return m_parameters.historyLength;
    }

    int getBinCount() const {
        return m_binmax - m_binmin + 1;
    }
    
    std::vector<double> getFractions() const {
        return m_fractions;
    }
    
    std::vector<int> getBinsAboveNoiseFloorAt(int step) const {
        if (step < int(m_binsAboveNoiseFloor.size())) {
            return m_binsAboveNoiseFloor.at(step);
        } else {
            return {};
        }
    }
    
    std::vector<int> getBinsAboveOffsetAt(int step) const {
        if (step < int(m_binsAboveOffset.size())) {
            return m_binsAboveOffset.at(step);
        } else {
            return {};
        }
    }

private:
    Parameters m_parameters;
    int m_binmin;
    int m_binmax;
    double m_rise_ratio;
    double m_noiseFloor_mag;
    double m_offset_mag;
    bool m_initialised;
    std::vector<float> m_window;
    std::deque<std::vector<double>> m_magHistory;
    std::vector<double> m_fractions;
    std::vector<std::vector<int>> m_binsAboveNoiseFloor;
    std::vector<std::vector<int>> m_binsAboveOffset;

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
                if (m_magHistory[j][i] > m_magHistory[0][i] * m_rise_ratio) {
                    ++above;
                    break;
                }
            }
        }
        return double(above) / double(n);
    }
};

#endif

