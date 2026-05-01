#pragma once
#include <JuceHeader.h>
#include <FXFunctions.h>

enum class vaFilterAlgorithm {
	kLPF1, kHPF1, kAPF1, kSVF_LP, kSVF_HP, kSVF_BP, kSVF_BS
}; // --- you will add more here...


/**
\struct ZVAFilterParameters
\ingroup FX-Objects
\brief
Custom parameter structure for the ZVAFilter object.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
struct ZVAFilterParameters
{
	ZVAFilterParameters() {}
	/** all FXObjects parameter objects require overloaded= operator so remember to add new entries if you add new variables. */
	ZVAFilterParameters& operator=(const ZVAFilterParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;

		filterAlgorithm = params.filterAlgorithm;
		fc = params.fc;
		Q = params.Q;
		filterOutputGain_dB = params.filterOutputGain_dB;
		enableGainComp = params.enableGainComp;
		matchAnalogNyquistLPF = params.matchAnalogNyquistLPF;
		selfOscillate = params.selfOscillate;
		enableNLP = params.enableNLP;
		return *this;
	}

	// --- individual parameters
	vaFilterAlgorithm filterAlgorithm = vaFilterAlgorithm::kSVF_LP;	///< va filter algorithm
	double fc = 1000.0;						///< va filter fc
	double Q = 0.707;						///< va filter Q
	double filterOutputGain_dB = 0.0;		///< va filter gain (normally unused)
	bool enableGainComp = false;			///< enable gain compensation (see book)
	bool matchAnalogNyquistLPF = false;		///< match analog gain at Nyquist
	bool selfOscillate = false;				///< enable selfOscillation
	bool enableNLP = false;					///< enable non linear processing (use oversampling for best results)
};


/**
\class ZVAFilter
\ingroup FX-Objects
\brief
The ZVAFilter object implements multpile Zavalishin VA Filters.
Audio I/O:
- Processes mono input to mono output.

Control I/F:
- Use BiquadParameters structure to get/set object params.

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class ZVAFilter
{
public:
	ZVAFilter() {}		/* C-TOR */
	~ZVAFilter() {}		/* D-TOR */

	/** reset members to initialized state */
	virtual bool reset(double _sampleRate)
	{
		sampleRate = _sampleRate;
		integrator_z[0] = 0.0;
		integrator_z[1] = 0.0;

		return true;
	}

	/** get parameters: note use of custom structure for passing param data */
	/**
	\return ZVAFilterParameters custom data structure
	*/
	ZVAFilterParameters getParameters()
	{
		return zvaFilterParameters;
	}

	/** set parameters: note use of custom structure for passing param data */
	/**
	\param ZVAFilterParameters custom data structure
	*/
	void setParameters(const ZVAFilterParameters& params)
	{
		if (params.fc != zvaFilterParameters.fc ||
			params.Q != zvaFilterParameters.Q ||
			params.selfOscillate != zvaFilterParameters.selfOscillate ||
			params.matchAnalogNyquistLPF != zvaFilterParameters.matchAnalogNyquistLPF)
		{
				zvaFilterParameters = params;
				calculateFilterCoeffs();
		}
		else
			zvaFilterParameters = params;
	}

	/** return false: this object only processes samples */
	virtual bool canProcessAudioFrame() { return false; }

	/** process input x(n) through the VA filter to produce return value y(n) */
	/**
	\param xn input
	\return the processed sample
	*/
	virtual double processAudioSample(double xn)
	{
		// --- with gain comp enabled, we reduce the input by
		//     half the gain in dB at resonant peak
		//     NOTE: you can change that logic here!
		vaFilterAlgorithm filterAlgorithm = zvaFilterParameters.filterAlgorithm;
		bool matchAnalogNyquistLPF = zvaFilterParameters.matchAnalogNyquistLPF;

		if (zvaFilterParameters.enableGainComp)
		{
			double peak_dB = dBPeakGainFor_Q(zvaFilterParameters.Q);
			if (peak_dB > 0.0)
			{
				double halfPeak_dBGain = dB2Raw(-peak_dB / 2.0);
				xn *= halfPeak_dBGain;
			}
		}

		// --- for 1st order filters:
		if (filterAlgorithm == vaFilterAlgorithm::kLPF1 ||
			filterAlgorithm == vaFilterAlgorithm::kHPF1 ||
			filterAlgorithm == vaFilterAlgorithm::kAPF1)
		{
			// --- create vn node
			double vn = (xn - integrator_z[0])*alpha;

			// --- form LP output
			double lpf = ((xn - integrator_z[0])*alpha) + integrator_z[0];

			// double sn = integrator_z[0];

			// --- update memory
			integrator_z[0] = vn + lpf;

			// --- form the HPF = INPUT = LPF
			double hpf = xn - lpf;

			// --- form the APF = LPF - HPF
			double apf = lpf - hpf;

			// --- set the outputs
			if (filterAlgorithm == vaFilterAlgorithm::kLPF1)
			{
				// --- this is a very close match as-is at Nyquist!
				if (matchAnalogNyquistLPF)
					return lpf + alpha*hpf;
				else
					return lpf;
			}
			else if (filterAlgorithm == vaFilterAlgorithm::kHPF1)
				return hpf;
			else if (filterAlgorithm == vaFilterAlgorithm::kAPF1)
				return apf;

			// --- unknown filter
			return xn;
		}

		// --- form the HP output first
		double hpf = alpha0*(xn - rho*integrator_z[0] - integrator_z[1]);

		// --- BPF Out
		double bpf = alpha*hpf + integrator_z[0];
		if (zvaFilterParameters.enableNLP)
			bpf = softClipWaveShaper(bpf, 1.0);

		// --- LPF Out
		double lpf = alpha*bpf + integrator_z[1];

		// --- BSF Out
		double bsf = hpf + lpf;

		// --- finite gain at Nyquist; slight error at VHF
		double sn = integrator_z[0];

		// update memory
		integrator_z[0] = alpha*hpf + bpf;
		integrator_z[1] = alpha*bpf + lpf;

		double filterOutputGain = pow(10.0, zvaFilterParameters.filterOutputGain_dB / 20.0);

		// return our selected type
		if (filterAlgorithm == vaFilterAlgorithm::kSVF_LP)
		{
			if (matchAnalogNyquistLPF)
				lpf += analogMatchSigma*(sn);
			return filterOutputGain*lpf;
		}
		else if (filterAlgorithm == vaFilterAlgorithm::kSVF_HP)
			return filterOutputGain*hpf;
		else if (filterAlgorithm == vaFilterAlgorithm::kSVF_BP)
			return filterOutputGain*bpf;
		else if (filterAlgorithm == vaFilterAlgorithm::kSVF_BS)
			return filterOutputGain*bsf;

		// --- unknown filter
		return filterOutputGain*lpf;
	}

	/** recalculate the filter coefficients*/
	void calculateFilterCoeffs()
	{
		double fc = zvaFilterParameters.fc;
		double Q = zvaFilterParameters.Q;
		vaFilterAlgorithm filterAlgorithm = zvaFilterParameters.filterAlgorithm;

		// --- normal Zavalishin SVF calculations here
		//     prewarp the cutoff- these are bilinear-transform filters
		double wd = kTwoPi*fc;
		double T = 1.0 / sampleRate;
		double wa = (2.0 / T)*tan(wd*T / 2.0);
		double g = wa*T / 2.0;

		// --- for 1st order filters:
		if (filterAlgorithm == vaFilterAlgorithm::kLPF1 ||
			filterAlgorithm == vaFilterAlgorithm::kHPF1 ||
			filterAlgorithm == vaFilterAlgorithm::kAPF1)
		{
			// --- calculate alpha
			alpha = g / (1.0 + g);
		}
		else // state variable variety
		{
			// --- note R is the traditional analog damping factor zeta
			double R = zvaFilterParameters.selfOscillate ? 0.0 : 1.0 / (2.0*Q);
			alpha0 = 1.0 / (1.0 + 2.0*R*g + g*g);
			alpha = g;
			rho = 2.0*R + g;

			// --- sigma for analog matching version
			double f_o = (sampleRate / 2.0) / fc;
			analogMatchSigma = 1.0 / (alpha*f_o*f_o);
		}
	}

	/** set beta value, for filters that aggregate 1st order VA sections*/
	void setBeta(double _beta) { beta = _beta; }

	/** get beta value,not used in book projects; for future use*/
	double getBeta() { return beta; }

protected:
	ZVAFilterParameters zvaFilterParameters;	///< object parameters
	double sampleRate = 44100.0;				///< current sample rate

	// --- state storage
	double integrator_z[2];						///< state variables

	// --- filter coefficients
	double alpha0 = 0.0;		///< input scalar, correct delay-free loop
	double alpha = 0.0;			///< alpha is (wcT/2)
	double rho = 0.0;			///< p = 2R + g (feedback)

	double beta = 0.0;			///< beta value, not used

	// --- for analog Nyquist matching
	double analogMatchSigma = 0.0; ///< analog matching Sigma value (see book)

};