struct GloriousParams {
	GloriousParams() {}

	GloriousParams(float rate, float depth, float fdbk, float mix)
	{
		this->rate = rate;
		this->depth = depth;
		this->fdbk = fdbk;
		this->mix = mix;
	}

	float rate = 0.5f;
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

		out[0] = in[0] * (1 - params.mix) + (taps[0] * 0.5f + taps[1] * 0.3f + taps[2] * 0.2f) * params.mix;
		out[1] = in[1] * (1 - params.mix) + (taps[3] * 0.5f + taps[2] * 0.3f + taps[1] * 0.2f) * params.mix;

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

	float rate = 0.5f;
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
		}
	}

	std::array<float, 2> process(std::array<float, 2> in)
	{
		lfoIncs[0] = 4.0 * params.rate / spec.sampleRate;
		lfoIncs[1] = lfoIncs[0] * 1.6;

		std::array<float, 2> lfos;

		// Calculate lfo values
		for (int i = 0; i < 2; ++i)
		{
			lfos[i] = (triangle(lfoXs[i]) + 1.0f) / 2.0f;
			lfos[i] = 10.7f * params.depth * lfos[i] + 1.66f;
		}

		std::array<float, 2> delayLOuts, delayROuts;

		// Tap the delays
		for (int i = 0; i < 2; ++i)
		{
			delayLOuts[i] = delays[i].popSample(0, msToSamples(lfos[i])); // L
			delayROuts[i] = delays[i].popSample(1, msToSamples(-lfos[i])); // R
		}

		for (int i = 0; i < 2; i++) 
		{
			delays[i].pushSample(0, in[0]); // L
			delays[i].pushSample(1, in[1]); // R
		}

		std::array<float, 2> out;

		float d = 0.5 * params.dup;
		out[0] = in[0] * (1 - params.mix) + (delayLOuts[0] * (1 - d) + delayLOuts[1] * d) * params.mix; // L
		out[1] = in[1] * (1 - params.mix) + (delayROuts[0] * (1 - d) + delayROuts[1] * d) * params.mix; // R

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

	float msToSamples(float ms)
	{
		return ms * spec.sampleRate / 1000.0f;
	}

	float triangle(float x) {
		if (x <= 2.0) { return -x + 1; }
		else { return x - 3; }
	}
};