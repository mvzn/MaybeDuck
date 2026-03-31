#pragma once
#include <JuceHeader.h>

enum class BiquadFilterType
{
  kLPF, kHPF
};

struct AudioFilterParameters
{
    AudioFilterParameters()= default;
    AudioFilterParameters& operator=(const AudioFilterParameters& params)
    {
        if (this != &params)
        {
          algorithm = params.algorithm;
          fc = params.fc;
          Q = params.Q;
        }
        return *this;
    }
    BiquadFilterType algorithm = BiquadFilterType::kLPF;
    double fc = 700.;
    double Q = 0.70710678118;
};

class FilterBiquad
{
public:
  FilterBiquad() {reset();};
    ~FilterBiquad() {}
    void setParameters(const AudioFilterParameters& params) {
        m_params = params;
        calculateCoefficients();
        return;
    }

    AudioFilterParameters getParameters() const
    {
        return m_params;
    }

    bool reset()
    {
        state.fill(0.);
        return true;
    }

    double process2order(const double& input)
    {
      return processDF2T(input);
    }
    
    double process4order(const double& input)
    {
      double xn = processDF2T(input);
      return processDF2T(xn);
    }

    double processDF2T(double input)
    {
      double output = input * coeffs[0] + state[0];
      state[0] = input * coeffs[1] - output * coeffs[3] + state[1];
      state[1] = input * coeffs[2] - output * coeffs[4];
      return output;
    }

    void prepare(double _sampleRate)
    {
      fs = (_sampleRate > 0.) ? _sampleRate : 44100.;
        calculateCoefficients();
        return;
    }

    void calculateCoefficients()
    {
      coeffs.fill(0.);
      double w0 = 2. * juce::MathConstants<double>::pi * m_params.fc / fs;
      double alpha = sin(w0) / (2. * std::max(m_params.Q, 0.5));
      double cos_w0 = cos(w0);
      const double d = 1. / m_params.Q;
      const double beta = 0.5 * ((1. - (d / 2 * std::sin(w0))) / (1. + (d / 2 * std::sin(w0))));
      const double g = (0.5 + beta) * cos_w0;

      switch (m_params.algorithm)
      {
        // 2nd order LPF
        case BiquadFilterType::kLPF:
        {
          coeffs[0] = ((0.5 + beta - g ) / 2.0);
          coeffs[1] = (0.5 + beta - g);
          coeffs[2] = coeffs[0];
          coeffs[3] = -2. * g;
          coeffs[4] = 2. * beta;
          break;
        }
        // 2nd order HPF
        case BiquadFilterType::kHPF:
        {
          coeffs[0] = ((0.5 + beta + g ) / 2.0);
          coeffs[1] = -(0.5 + beta + g);
          coeffs[2] = coeffs[0];
          coeffs[3] = (-2. * g);
          coeffs[4] = (2. * beta);
          break;
        }
      }
    }
private:
    AudioFilterParameters m_params;
    std::array<double, 5> coeffs {};
    std::array<double, 2> state {};
    double fs = 44100.0;
    size_t order = 0;
};