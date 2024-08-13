class Biquad {
public:
	Biquad() {}
	~Biquad() {}

	void prepare(double a0, double b1)
	{
		this->a0 = a0;
		a1 = 0.0f;
		this->b1 = b1;

		reset();
	}

	void reset()
	{
		z1 = 0.0f;
		z2 = 0.0f;
	}

	float process(float in)
	{
		float out = a0 * in + z1;

		// Check for underflow
		JUCE_SNAP_TO_ZERO(out);

		z1 = a1 * in - b1 * out;

		return out;
	}

private:
	double a0, a1, b1;
	float z1, z2 = 0.0f;
};

//===========================================================

struct GloriousParams {
	GloriousParams() {}

	GloriousParams(float rate, float depth, float fdbk, float mix)
	{
		this->rate = rate;
		this->depth = depth;
		this->fdbk = fdbk;
		this->mix = mix;
	}

	float rate = 0.25f;
	float depth = 0.5f;
	float fdbk = 0.0f;
	float mix = 0.5f;
};

class Glorious {
public:
	Glorious() {}
	~Glorious() {}

	void prepare(juce::dsp::ProcessSpec spec, int maxDelayInSamples) 
	{
		this->spec = spec;
		delay.prepare(spec);
		delay.setMaximumDelayInSamples(maxDelayInSamples);
		delay.reset();
	}

	std::array<float, 2> process(std::array<float, 2> in)
	{
		lfoIncrement = juce::MathConstants<float>::twoPi * params.rate / spec.sampleRate;

		// Calculate lfo values
		for (int i = 0; i < 4; ++i)
		{
			float phaseShift = (float)i * juce::MathConstants<float>::halfPi;
			lfos[i] = (std::sin(lfoX + phaseShift) + 1.0f) / 2.0f;
			lfos[i] = 35.0f * params.depth * lfos[i] + 5.0f;
		}
		
		// Tap the delays
		for (int i = 0; i < 4; ++i)
		{
			int channel = i / 2;
			taps[i] = delay.popSample(channel, msToSamples(lfos[i]), i % 2 != 0); // Update read pointer on the last read
		}

		float f = 0.9 * params.fdbk;
		float toWriteL = in[0] + (taps[0] + taps[1]) * 0.5 * f;
		float toWriteR = in[1] + (taps[2] + taps[3]) * 0.5 * f;
		
		delay.pushSample(0, toWriteL);
		delay.pushSample(1, toWriteR);

		std::array<float, 2> out;

		out[0] = in[0] * (1.0f - params.mix) + (taps[0] * 0.55f + taps[1] * 0.33f + taps[2] * 0.22f) * params.mix * gainMakeup(params.mix);
		out[1] = in[1] * (1.0f - params.mix) + (taps[3] * 0.55f + taps[2] * 0.33f + taps[1] * 0.22f) * params.mix * gainMakeup(params.mix);

		lfoX += lfoIncrement;

		return out;
	}

	void setParams(GloriousParams params) 
	{
		this->params = params;
	}

	GloriousParams getParams() 
	{
		return params;
	}

private:
	juce::dsp::ProcessSpec spec;
	juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delay;

	GloriousParams params;

	double lfoX{ 0.0 };
	double lfoIncrement;

	std::array<float, 4> lfos;
	std::array<float, 4> taps;

	float msToSamples(float ms)
	{
		return ms * spec.sampleRate / 1000.0f;
	}

	float gainMakeup(float x)
	{
		return -std::powf((1.5 * x), 2) + 2.25 * x + 1;
	}
};

//=================================================================================

struct JuneParams {
	JuneParams() {}

	JuneParams(float rate, float depth, float dup, float mix)
	{
		this->rate = rate;
		this->depth = depth;
		this->dup = dup;
		this->mix = mix;
	}

	float rate = 0.4f;
	float depth = 0.5f;
	float dup = 0.0f;
	float mix = 0.5f;
};

class June {
public:
	June() {}
	~June() {}

	void prepare(juce::dsp::ProcessSpec spec, int maxDelayInSamples)
	{
		this->spec = spec;
		for (int i = 0; i < 2; i++) 
		{
			delays[i].prepare(spec);
			delays[i].setMaximumDelayInSamples(maxDelayInSamples);
			delays[i].reset();
			lpfs[i].prepare(0.6311373775480995, -0.36886262245190043);
		}
	}

	std::array<float, 2> process(std::array<float, 2> in)
	{
		lfoIncs[0] = 4.0 * params.rate / spec.sampleRate;
		lfoIncs[1] = lfoIncs[0] * 1.6;

		std::array<float, 2> lfos;
		std::array<float, 2> ilfos; // Inverse lfos

		// Calculate lfo values
		for (int i = 0; i < 2; ++i)
		{
			lfos[i] = (triangle(lfoXs[i]) + 1.0f) / 2.0f;
			ilfos[i] = -lfos[i] + 1.0f;

			lfos[i] = 10.7f * params.depth * lfos[i] + 1.66f;
			ilfos[i] = 10.7f * params.depth * ilfos[i] + 1.66f;
		}

		std::array<float, 2> delayLOuts, delayROuts;

		// Tap the delays
		for (int i = 0; i < 2; ++i)
		{
			delayLOuts[i] = delays[i].popSample(0, msToSamples(lfos[i])); // L
			delayROuts[i] = delays[i].popSample(1, msToSamples(ilfos[i])); // R
		}

		for (int i = 0; i < 2; i++) 
		{
			delays[i].pushSample(0, in[0]); // L
			delays[i].pushSample(1, in[1]); // R
		}

		std::array<float, 2> wet;
		float d = 0.5 * params.dup;
		wet[0] = delayLOuts[0] * (1.0f - d) + delayLOuts[1] * d * (1.2 * params.dup); // L
		wet[1] = delayROuts[0] * (1.0f - d) + delayROuts[1] * d * (1.2 * params.dup); // R

		// Filter the wet signal
		for (int i = 0; i < 2; i++) 
		{
			wet[i] = lpfs[i].process(wet[i]); // L + R lpf
		}
		
		std::array<float, 2> out;
		out[0] = in[0] * (1.0f - params.mix) + wet[0] * params.mix * gainMakeup(params.mix); // L
		out[1] = in[1] * (1.0f - params.mix) + wet[1] * params.mix * gainMakeup(params.mix); // R

		for (int i = 0; i < 2; i++)
		{
			lfoXs[i] += lfoIncs[i];
			if (lfoXs[i] > 4.0) { lfoXs[i] -= 4.0; }
		}

		return out;
	}

	void setParams(JuneParams params)
	{
		this->params = params;
	}

	JuneParams getParams()
	{
		return params;
	}

private:
	juce::dsp::ProcessSpec spec;
	std::array<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>, 2> delays;

	JuneParams params;

	std::array<float, 2> lfoXs{ 0.0, 0.0 };
	std::array<float, 2> lfoIncs;

	std::array<Biquad, 2> lpfs{ Biquad(), Biquad() };

	float msToSamples(float ms)
	{
		return ms * spec.sampleRate / 1000.0f;
	}

	float triangle(float x) {
		if (x <= 2.0) { return -x + 1; }
		else { return x - 3; }
	}

	float gainMakeup(float x) 
	{
		return -std::powf((1.5 * x), 2) + 2.25 * x + 1;
	}
};