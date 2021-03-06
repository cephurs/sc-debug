/*
	SuperCollider real time audio synthesis system
    Copyright (c) 2002 James McCartney. All rights reserved.
	http://www.audiosynth.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/


#include "SC_PlugIn.h"

const int kMAXMEDIANSIZE = 32;

static InterfaceTable *ft;

struct ScopeOut : public Unit
{
	SndBuf *m_buf;
	SndBufUpdates *m_bufupdates;
	float m_fbufnum;
	uint32 m_framepos, m_framecount;
	float **mIn;
};

struct PlayBuf : public Unit
{
	double m_phase;
	float m_prevtrig;
	float m_fbufnum;
	SndBuf *m_buf;
	float **mOut;
};


struct Grain
{
	double phase, rate;
	double b1, y1, y2; // envelope
	float pan1, pan2;
	int counter;
	int bufnum;
	int chan;
	int interp;
};

const int kMaxGrains = 64;

struct TGrains : public Unit
{
	float mPrevTrig;
	int mNumActive;
	Grain mGrains[kMaxGrains];
};


#if NOTYET
struct SimpleLoopBuf : public Unit
{
	int m_phase;
	float m_prevtrig;
	float m_fbufnum;
	SndBuf *m_buf;
	float **mOut;
};
#endif

struct BufRd : public Unit
{
	float m_fbufnum;
	SndBuf *m_buf;
	float **mOut;
};

struct BufWr : public Unit
{
	float m_fbufnum;
	SndBuf *m_buf;
	float **mIn;
};

struct RecordBuf : public Unit
{
	float m_fbufnum;
	SndBuf *m_buf;
	int32 m_writepos;
	float m_recLevel, m_preLevel;
	float m_prevtrig;
	float **mIn;
};

struct Pitch : public Unit
{
	float m_values[kMAXMEDIANSIZE];
	int m_ages[kMAXMEDIANSIZE];
	float *m_buffer;

	float m_freq, m_minfreq, m_maxfreq, m_hasfreq, m_srate, m_ampthresh, m_peakthresh;
	int m_minperiod, m_maxperiod, m_execPeriod, m_index, m_readp, m_size;
	int m_downsamp, m_maxlog2bins, m_medianSize;
	int m_state;
};

struct BufDelayUnit : public Unit
{
	float m_fbufnum;
	SndBuf *m_buf;
	float m_dsamp;
	float m_delaytime;
	int m_iwrphase;
	uint32 m_numoutput;
};

struct BufDelayN : public BufDelayUnit
{
};

struct BufDelayL : public BufDelayUnit
{
};

struct BufDelayC : public BufDelayUnit
{
};

struct BufFeedbackDelay : public BufDelayUnit
{
	float m_feedbk, m_decaytime;
};

struct BufCombN : public BufFeedbackDelay
{
};

struct BufCombL : public BufFeedbackDelay
{
};

struct BufCombC : public BufFeedbackDelay
{
};

struct BufAllpassN : public BufFeedbackDelay
{
};

struct BufAllpassL : public BufFeedbackDelay
{
};

struct BufAllpassC : public BufFeedbackDelay
{
};


struct DelayUnit : public Unit
{
	float *m_dlybuf;

	float m_dsamp, m_fdelaylen;
	float m_delaytime, m_maxdelaytime;
	long m_iwrphase, m_idelaylen, m_mask;
	long m_numoutput;
};

struct DelayN : public DelayUnit
{
};

struct DelayL : public DelayUnit
{
};

struct DelayC : public DelayUnit
{
};

struct FeedbackDelay : public DelayUnit
{
	float m_feedbk, m_decaytime;
};

struct CombN : public FeedbackDelay
{
};

struct CombL : public FeedbackDelay
{
};

struct CombC : public FeedbackDelay
{
};

struct AllpassN : public FeedbackDelay
{
};

struct AllpassL : public FeedbackDelay
{
};

struct AllpassC : public FeedbackDelay
{
};

struct BufInfoUnit : public Unit
{
	float m_fbufnum;
	SndBuf *m_buf;
};

struct Pluck : public FeedbackDelay
{	float m_lastsamp, m_prevtrig, m_coef;
	long m_inputsamps;
};

struct LocalBuf : public Unit
{
	float m_fbufnum;
	SndBuf *m_buf;
};

struct MaxLocalBufs : public Unit
{

};

struct SetBuf : public Unit
{
	float m_fbufnum;
	SndBuf *m_buf;
};

struct ClearBuf : public Unit
{
	float m_fbufnum;
	SndBuf *m_buf;
};



//////////////////////////////////////////////////////////////////////////////////////////////////

extern "C"
{
	void load(InterfaceTable *inTable);

	void SampleRate_Ctor(Unit *unit, int inNumSamples);
	void ControlRate_Ctor(Unit *unit, int inNumSamples);
	void SampleDur_Ctor(Unit *unit, int inNumSamples);
	void ControlDur_Ctor(Unit *unit, int inNumSamples);
	void SubsampleOffset_Ctor(Unit *unit, int inNumSamples);
	void RadiansPerSample_Ctor(Unit *unit, int inNumSamples);
	void NumInputBuses_Ctor(Unit *unit, int inNumSamples);
	void NumOutputBuses_Ctor(Unit *unit, int inNumSamples);
	void NumAudioBuses_Ctor(Unit *unit, int inNumSamples);
	void NumControlBuses_Ctor(Unit *unit, int inNumSamples);
	void NumBuffers_Ctor(Unit *unit, int inNumSamples);
	void NumRunningSynths_Ctor(Unit *unit, int inNumSamples);
	void NumRunningSynths_next(Unit *unit, int inNumSamples);

	void BufSampleRate_next(BufInfoUnit *unit, int inNumSamples);
	void BufSampleRate_Ctor(BufInfoUnit *unit, int inNumSamples);

	void BufFrames_next(BufInfoUnit *unit, int inNumSamples);
	void BufFrames_Ctor(BufInfoUnit *unit, int inNumSamples);

	void BufDur_next(BufInfoUnit *unit, int inNumSamples);
	void BufDur_Ctor(BufInfoUnit *unit, int inNumSamples);

	void BufChannels_next(BufInfoUnit *unit, int inNumSamples);
	void BufChannels_Ctor(BufInfoUnit *unit, int inNumSamples);

	void BufSamples_next(BufInfoUnit *unit, int inNumSamples);
	void BufSamples_Ctor(BufInfoUnit *unit, int inNumSamples);

	void BufRateScale_next(BufInfoUnit *unit, int inNumSamples);
	void BufRateScale_Ctor(BufInfoUnit *unit, int inNumSamples);

	void PlayBuf_next_aa(PlayBuf *unit, int inNumSamples);
	void PlayBuf_next_ak(PlayBuf *unit, int inNumSamples);
	void PlayBuf_next_ka(PlayBuf *unit, int inNumSamples);
	void PlayBuf_next_kk(PlayBuf *unit, int inNumSamples);
	void PlayBuf_Ctor(PlayBuf* unit);
	void PlayBuf_Dtor(PlayBuf* unit);

	void TGrains_next(TGrains *unit, int inNumSamples);
	void TGrains_Ctor(TGrains* unit);

#if NOTYET
	void SimpleLoopBuf_next_kk(SimpleLoopBuf *unit, int inNumSamples);
	void SimpleLoopBuf_Ctor(SimpleLoopBuf* unit);
	void SimpleLoopBuf_Dtor(SimpleLoopBuf* unit);
#endif

	void BufRd_Ctor(BufRd *unit);
	void BufRd_Dtor(BufRd *unit);
	void BufRd_next_4(BufRd *unit, int inNumSamples);
	void BufRd_next_2(BufRd *unit, int inNumSamples);
	void BufRd_next_1(BufRd *unit, int inNumSamples);

	void BufWr_Ctor(BufWr *unit);
	void BufWr_Dtor(BufWr *unit);
	void BufWr_next(BufWr *unit, int inNumSamples);

	void RecordBuf_Ctor(RecordBuf *unit);
	void RecordBuf_Dtor(RecordBuf *unit);
	void RecordBuf_next(RecordBuf *unit, int inNumSamples);
	void RecordBuf_next_10(RecordBuf *unit, int inNumSamples);

	void Pitch_Ctor(Pitch *unit);
	void Pitch_next_a(Pitch *unit, int inNumSamples);
	void Pitch_next_k(Pitch *unit, int inNumSamples);

	void LocalBuf_Ctor(LocalBuf *unit);
	void LocalBuf_Dtor(LocalBuf *unit);
	void LocalBuf_next(LocalBuf *unit, int inNumSamples);

	void MaxLocalBufs_Ctor(MaxLocalBufs *unit);

	void SetBuf_Ctor(SetBuf *unit);
	void SetBuf_next(SetBuf *unit, int inNumSamples);
	void ClearBuf_Ctor(ClearBuf *unit);
	void ClearBuf_next(ClearBuf *unit, int inNumSamples);

	void BufDelayUnit_Reset(BufDelayUnit *unit);
	void BufFeedbackDelay_Reset(BufFeedbackDelay *unit);

	void BufDelayN_Ctor(BufDelayN *unit);
	void BufDelayN_next(BufDelayN *unit, int inNumSamples);
	void BufDelayN_next_z(BufDelayN *unit, int inNumSamples);

	void BufDelayL_Ctor(BufDelayL *unit);
	void BufDelayL_next(BufDelayL *unit, int inNumSamples);
	void BufDelayL_next_z(BufDelayL *unit, int inNumSamples);

	void BufDelayC_Ctor(BufDelayC *unit);
	void BufDelayC_next(BufDelayC *unit, int inNumSamples);
	void BufDelayC_next_z(BufDelayC *unit, int inNumSamples);

	void BufCombN_Ctor(BufCombN *unit);
	void BufCombN_next(BufCombN *unit, int inNumSamples);
	void BufCombN_next_z(BufCombN *unit, int inNumSamples);

	void BufCombL_Ctor(BufCombL *unit);
	void BufCombL_next(BufCombL *unit, int inNumSamples);
	void BufCombL_next_z(BufCombL *unit, int inNumSamples);

	void BufCombC_Ctor(BufCombC *unit);
	void BufCombC_next(BufCombC *unit, int inNumSamples);
	void BufCombC_next_z(BufCombC *unit, int inNumSamples);

	void BufAllpassN_Ctor(BufAllpassN *unit);
	void BufAllpassN_next(BufAllpassN *unit, int inNumSamples);
	void BufAllpassN_next_z(BufAllpassN *unit, int inNumSamples);

	void BufAllpassL_Ctor(BufAllpassL *unit);
	void BufAllpassL_next(BufAllpassL *unit, int inNumSamples);
	void BufAllpassL_next_z(BufAllpassL *unit, int inNumSamples);

	void BufAllpassC_Ctor(BufAllpassC *unit);
	void BufAllpassC_next(BufAllpassC *unit, int inNumSamples);
	void BufAllpassC_next_z(BufAllpassC *unit, int inNumSamples);

	void DelayUnit_Reset(DelayUnit *unit);
	void DelayUnit_Dtor(DelayUnit *unit);
	void DelayUnit_AllocDelayLine(DelayUnit *unit);
	void FeedbackDelay_Reset(FeedbackDelay *unit);

	void DelayN_Ctor(DelayN *unit);
	void DelayN_next(DelayN *unit, int inNumSamples);
	void DelayN_next_z(DelayN *unit, int inNumSamples);

	void DelayL_Ctor(DelayL *unit);
	void DelayL_next(DelayL *unit, int inNumSamples);
	void DelayL_next_z(DelayL *unit, int inNumSamples);

	void DelayC_Ctor(DelayC *unit);
	void DelayC_next(DelayC *unit, int inNumSamples);
	void DelayC_next_z(DelayC *unit, int inNumSamples);

	void CombN_Ctor(CombN *unit);
	void CombN_next(CombN *unit, int inNumSamples);
	void CombN_next_z(CombN *unit, int inNumSamples);

	void CombL_Ctor(CombL *unit);
	void CombL_next(CombL *unit, int inNumSamples);
	void CombL_next_z(CombL *unit, int inNumSamples);

	void CombC_Ctor(CombC *unit);
	void CombC_next(CombC *unit, int inNumSamples);
	void CombC_next_z(CombC *unit, int inNumSamples);

	void AllpassN_Ctor(AllpassN *unit);
	void AllpassN_next(AllpassN *unit, int inNumSamples);
	void AllpassN_next_z(AllpassN *unit, int inNumSamples);

	void AllpassL_Ctor(AllpassL *unit);
	void AllpassL_next(AllpassL *unit, int inNumSamples);
	void AllpassL_next_z(AllpassL *unit, int inNumSamples);

	void AllpassC_Ctor(AllpassC *unit);
	void AllpassC_next(AllpassC *unit, int inNumSamples);
	void AllpassC_next_z(AllpassC *unit, int inNumSamples);

	void ScopeOut_next(ScopeOut *unit, int inNumSamples);
	void ScopeOut_Ctor(ScopeOut *unit);
	void ScopeOut_Dtor(ScopeOut *unit);

	void Pluck_Ctor(Pluck* unit);
	void Pluck_next_aa(Pluck *unit, int inNumSamples);
	void Pluck_next_aa_z(Pluck *unit, int inNumSamples);
	void Pluck_next_kk(Pluck *unit, int inNumSamples);
	void Pluck_next_kk_z(Pluck *unit, int inNumSamples);
	void Pluck_next_ka(Pluck *unit, int inNumSamples);
	void Pluck_next_ka_z(Pluck *unit, int inNumSamples);
	void Pluck_next_ak(Pluck *unit, int inNumSamples);
	void Pluck_next_ak_z(Pluck *unit, int inNumSamples);

}


//////////////////////////////////////////////////////////////////////////////////////////////////

void SampleRate_Ctor(Unit *unit, int inNumSamples)
{
	ZOUT0(0) = unit->mWorld->mSampleRate;
}


void ControlRate_Ctor(Unit *unit, int inNumSamples)
{
	ZOUT0(0) = unit->mWorld->mBufRate.mSampleRate;
}


void SampleDur_Ctor(Unit *unit, int inNumSamples)
{
	ZOUT0(0) = unit->mWorld->mFullRate.mSampleDur;
}

void ControlDur_Ctor(Unit *unit, int inNumSamples)
{
	ZOUT0(0) = unit->mWorld->mFullRate.mBufDuration;
}

void RadiansPerSample_Ctor(Unit *unit, int inNumSamples)
{
	ZOUT0(0) = unit->mWorld->mFullRate.mRadiansPerSample;
}

void SubsampleOffset_Ctor(Unit *unit, int inNumSamples)
{
	ZOUT0(0) = unit->mParent->mSubsampleOffset;
}


void NumInputBuses_Ctor(Unit *unit, int inNumSamples)
{
	ZOUT0(0) = unit->mWorld->mNumInputs;
}

void NumOutputBuses_Ctor(Unit *unit, int inNumSamples)
{
	ZOUT0(0) = unit->mWorld->mNumOutputs;
}

void NumAudioBuses_Ctor(Unit *unit, int inNumSamples)
{
	ZOUT0(0) = unit->mWorld->mNumAudioBusChannels;
}

void NumControlBuses_Ctor(Unit *unit, int inNumSamples)
{
	ZOUT0(0) = unit->mWorld->mNumControlBusChannels;
}

void NumBuffers_Ctor(Unit *unit, int inNumSamples)
{
	ZOUT0(0) = unit->mWorld->mNumSndBufs;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NumRunningSynths_Ctor(Unit *unit, int inNumSamples)
{
	if(INRATE(0) != calc_ScalarRate) { SETCALC(NumRunningSynths_next); }
	ZOUT0(0) = unit->mWorld->mNumGraphs;
}

void NumRunningSynths_next(Unit *unit, int inNumSamples)
{
	ZOUT0(0) = unit->mWorld->mNumGraphs;
}


//////////////////////////////////////////////////////////////////////////////////////////////////

void BufSampleRate_next(BufInfoUnit *unit, int inNumSamples)
{
	SIMPLE_GET_BUF
	ZOUT0(0) = buf->samplerate;
}

void BufSampleRate_Ctor(BufInfoUnit *unit, int inNumSamples)
{
	SETCALC(BufSampleRate_next);
	unit->m_fbufnum = -1e9f;
	SIMPLE_GET_BUF
	ZOUT0(0) = buf->samplerate;
}


void BufFrames_next(BufInfoUnit *unit, int inNumSamples)
{
	SIMPLE_GET_BUF
	ZOUT0(0) = buf->frames;
}

void BufFrames_Ctor(BufInfoUnit *unit, int inNumSamples)
{
	SETCALC(BufFrames_next);
	unit->m_fbufnum = -1.f;
	SIMPLE_GET_BUF
	ZOUT0(0) = buf->frames;
}


void BufDur_next(BufInfoUnit *unit, int inNumSamples)
{
	SIMPLE_GET_BUF
	ZOUT0(0) = buf->frames * buf->sampledur;
}

void BufDur_Ctor(BufInfoUnit *unit, int inNumSamples)
{
	SETCALC(BufDur_next);
	unit->m_fbufnum = -1e9f;
	SIMPLE_GET_BUF
	ZOUT0(0) = buf->frames * buf->sampledur;
}


void BufChannels_next(BufInfoUnit *unit, int inNumSamples)
{
	SIMPLE_GET_BUF
	ZOUT0(0) = buf->channels;
}

void BufChannels_Ctor(BufInfoUnit *unit, int inNumSamples)
{
	SETCALC(BufChannels_next);
	unit->m_fbufnum = -1e9f;
	SIMPLE_GET_BUF
	ZOUT0(0) = buf->channels;
}


void BufSamples_next(BufInfoUnit *unit, int inNumSamples)
{
	SIMPLE_GET_BUF
	ZOUT0(0) = buf->samples;
}

void BufSamples_Ctor(BufInfoUnit *unit, int inNumSamples)
{
	SETCALC(BufSamples_next);
	unit->m_fbufnum = -1e9f;
	SIMPLE_GET_BUF
	ZOUT0(0) = buf->samples;
}


void BufRateScale_next(BufInfoUnit *unit, int inNumSamples)
{
	SIMPLE_GET_BUF
	ZOUT0(0) = buf->samplerate * unit->mWorld->mFullRate.mSampleDur;
}

void BufRateScale_Ctor(BufInfoUnit *unit, int inNumSamples)
{
	SETCALC(BufRateScale_next);
	unit->m_fbufnum = -1e9f;
	SIMPLE_GET_BUF
	ZOUT0(0) = buf->samplerate * unit->mWorld->mFullRate.mSampleDur;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline int32 BUFMASK(int32 x)
{
	return (1 << (31 - CLZ(x))) - 1;
}


void LocalBuf_allocBuffer(LocalBuf *unit, SndBuf *buf, int numChannels, int numFrames) {

	int numSamples = numFrames * numChannels;
	// Print("bufnum: %i, allocating %i channels and %i frames. memsize: %i\n", (int)unit->m_fbufnum, numChannels, numFrames, numSamples * sizeof(float));
	buf->data = (float*)RTAlloc(unit->mWorld, numSamples * sizeof(float));

	if (!buf->data) {
		if(unit->mWorld->mVerbosity > -2){
			Print("failed to allocate memory for LocalBuffer\n");
		}
		return;
	}

	buf->channels = numChannels;
	buf->frames   = numFrames;
	buf->samples  = numSamples;
	buf->mask     = BUFMASK(numSamples); // for delay lines
	buf->mask1    = buf->mask - 1;	// for oscillators
	buf->samplerate = unit->mWorld->mSampleRate;
	buf->sampledur = 1. / buf->samplerate;

}




void LocalBuf_Ctor(LocalBuf *unit)
{
	Graph *parent = unit->mParent;

	int offset =  unit->mWorld->mNumSndBufs;
	int bufnum =  parent->localBufNum;

	if (parent->localBufNum >= parent->localMaxBufNum) {
		unit->m_fbufnum = -1.f;
		if(unit->mWorld->mVerbosity > -2){
			printf("warning: LocalBuf tried to allocate too many local buffers.\n");
		}

	} else {

		unit->m_fbufnum = (float) (bufnum + offset);
		unit->m_buf =  parent->mLocalSndBufs + bufnum;
		parent->localBufNum = parent->localBufNum + 1;

		LocalBuf_allocBuffer(unit, unit->m_buf, (int)IN0(0), (int)IN0(1));
	}

	OUT0(0) = unit->m_fbufnum;

}

void LocalBuf_Dtor(LocalBuf *unit)
{
	RTFree(unit->mWorld, unit->m_buf->data);
	if(unit->mParent->localBufNum <= 1) { // only the last time.
		RTFree(unit->mWorld, unit->mParent->mLocalSndBufs);
		unit->mParent->localMaxBufNum = 0;
	} else {
		unit->mParent->localBufNum =  unit->mParent->localBufNum - 1;
	}
}

// dummy for unit size.
void LocalBuf_next(LocalBuf *unit, int inNumSamples) {}


//////////////////////////////////////////////////////////////////////////////////////////////////

void MaxLocalBufs_Ctor(MaxLocalBufs *unit)
{
	Graph *parent = unit->mParent;

	int offset =  unit->mWorld->mNumSndBufs;
	int bufnum =  parent->localBufNum;
	int maxBufNum = (int)(IN0(0) + .5f);
	if(!parent->localMaxBufNum) {
		parent->mLocalSndBufs = (SndBuf*)RTAlloc(unit->mWorld, maxBufNum * sizeof(SndBuf));
		parent->localMaxBufNum = maxBufNum;
	} else {
		printf("warning: MaxLocalBufs - maximum number of local buffers is already declared (%i) and must remain unchanged.\n", parent->localMaxBufNum);
	}

}

//////////////////////////////////////////////////////////////////////////////////////////////////


void SetBuf_next(SetBuf *unit, int inNumSamples) {

	GET_BUF
	if (!bufData) {
		if(unit->mWorld->mVerbosity > -2){
			Print("SetBuf: no valid buffer\n");
		}
		return;
	}

	int offset = (int)IN0(1);
	int numArgs = (int)IN0(2);
	int end = sc_min(buf->samples, numArgs + offset);

	int j = 3;
	for(int i=offset; i<end; ++j, ++i) {
		bufData[i] = (float)IN0(j);
	}

}

void SetBuf_Ctor(SetBuf *unit)
{
	unit->m_fbufnum = -1.f;
	SETCALC(SetBuf_next);
	OUT0(0) = 0.f;
	SetBuf_next(unit, 0);
}


//////////////////////////////////////////////////////////////////////////////////////////////////


void ClearBuf_next(ClearBuf *unit, int inNumSamples) {

	GET_BUF
	if (!bufData) {
		if(unit->mWorld->mVerbosity > -2){
			Print("ClearBuf: no valid buffer\n");
		}
		return;
	}
	int n = unit->m_buf->samples;

	//bzero(unit->m_buf->data, unit->m_buf->samples * sizeof(float));
	for (int i=0; i<n; ++i) {
		bufData[i] = 0.f;
	}
}

void ClearBuf_Ctor(ClearBuf *unit)
{
	unit->m_fbufnum = -1.f;
	SETCALC(ClearBuf_next);
	OUT0(0) = 0.f;
	ClearBuf_next(unit, 0);
}




//////////////////////////////////////////////////////////////////////////////////////////////////

static float cubicinterp(float x, float y0, float y1, float y2, float y3)
{
	// 4-point, 3rd-order Hermite (x-form)
	float c0 = y1;
	float c1 = 0.5f * (y2 - y0);
	float c2 = y0 - 2.5f * y1 + 2.f * y2 - 0.5f * y3;
	float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

	return ((c3 * x + c2) * x + c1) * x + c0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////

inline float CalcFeedback(float delaytime, float decaytime)
{
	if (delaytime == 0.f) {
		return 0.f;
	} else if (decaytime > 0.f) {
		return exp(log001 * delaytime / decaytime);
	} else if (decaytime < 0.f) {
		return -exp(log001 * delaytime / -decaytime);
	} else {
		return 0.f;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

inline double sc_loop(Unit *unit, double in, double hi, int loop)
{
	// avoid the divide if possible
	if (in >= hi) {
		if (!loop) {
			unit->mDone = true;
			return hi;
		}
		in -= hi;
		if (in < hi) return in;
	} else if (in < 0.) {
		if (!loop) {
			unit->mDone = true;
			return 0.;
		}
		in += hi;
		if (in >= 0.) return in;
	} else return in;

	return in - hi * floor(in/hi);
}

#define CHECK_BUF \
	if (!bufData) { \
                unit->mDone = true; \
		ClearUnitOutputs(unit, inNumSamples); \
		return; \
	}

#define SETUP_OUT \
	uint32 numOutputs = unit->mNumOutputs; \
	if (numOutputs > bufChannels) { \
		if(unit->mWorld->mVerbosity > -1 && !unit->mDone){ \
			Print("buffer-reading UGen channel mismatch: numOutputs %i, yet buffer has %i channels\n", numOutputs, bufChannels); \
		} \
                unit->mDone = true; \
		ClearUnitOutputs(unit, inNumSamples); \
		return; \
	} \
	if(!unit->mOut){ \
		unit->mOut = (float**)RTAlloc(unit->mWorld, numOutputs * sizeof(float*)); \
	} \
	float **out = unit->mOut; \
	for (uint32 i=0; i<numOutputs; ++i){ \
		out[i] = ZOUT(i); \
	}

#define TAKEDOWN_OUT \
	if(unit->mOut){ \
		RTFree(unit->mWorld, unit->mOut); \
	}

#define SETUP_IN(offset) \
	uint32 numInputs = unit->mNumInputs - (uint32)offset; \
	if (numInputs != bufChannels) { \
		if(unit->mWorld->mVerbosity > -1 && !unit->mDone){ \
			Print("buffer-writing UGen channel mismatch: numInputs %i, yet buffer has %i channels\n", numInputs, bufChannels); \
		} \
                unit->mDone = true; \
		ClearUnitOutputs(unit, inNumSamples); \
		return; \
	} \
	if(!unit->mIn){ \
		unit->mIn = (float**)RTAlloc(unit->mWorld, numInputs * sizeof(float*)); \
	} \
	float **in = unit->mIn; \
	for (uint32 i=0; i<numInputs; ++i) { \
		in[i] = ZIN(i+offset); \
	}

#define TAKEDOWN_IN \
	if(unit->mIn){ \
		RTFree(unit->mWorld, unit->mIn); \
	}


#define LOOP_BODY_4 \
		phase = sc_loop((Unit*)unit, phase, loopMax, loop); \
		int32 iphase = (int32)phase; \
		float* table1 = bufData + iphase * bufChannels; \
		float* table0 = table1 - bufChannels; \
		float* table2 = table1 + bufChannels; \
		float* table3 = table2 + bufChannels; \
		if (iphase == 0) { \
			if (loop) { \
				table0 += bufSamples; \
			} else { \
				table0 += bufChannels; \
			} \
		} else if (iphase >= guardFrame) { \
			if (iphase == guardFrame) { \
				if (loop) { \
					table3 -= bufSamples; \
				} else { \
					table3 -= bufChannels; \
				} \
			} else { \
				if (loop) { \
					table2 -= bufSamples; \
					table3 -= bufSamples; \
				} else { \
					table2 -= bufChannels; \
					table3 -= 2 * bufChannels; \
				} \
			} \
		} \
		int32 index = 0; \
		float fracphase = phase - (double)iphase; \
		for (uint32 i=0; i<numOutputs; ++i) { \
			float a = table0[index]; \
			float b = table1[index]; \
			float c = table2[index]; \
			float d = table3[index]; \
			*++(out[i]) = cubicinterp(fracphase, a, b, c, d); \
			index++; \
		}

#define LOOP_BODY_2 \
		phase = sc_loop((Unit*)unit, phase, loopMax, loop); \
		int32 iphase = (int32)phase; \
		float* table1 = bufData + iphase * bufChannels; \
		float* table2 = table1 + bufChannels; \
		if (iphase > guardFrame) { \
			if (loop) { \
				table2 -= bufSamples; \
			} else { \
				table2 -= bufChannels; \
			} \
		} \
		int32 index = 0; \
		float fracphase = phase - (double)iphase; \
		for (uint32 i=0; i<numOutputs; ++i) { \
			float b = table1[index]; \
			float c = table2[index]; \
			*++(out[i]) = b + fracphase * (c - b); \
			index++; \
		}

#define LOOP_BODY_1 \
        phase = sc_loop((Unit*)unit, phase, loopMax, loop); \
		int32 iphase = (int32)phase; \
		float* table1 = bufData + iphase * bufChannels; \
		int32 index = 0; \
		for (uint32 i=0; i<numOutputs; ++i) { \
			*++(out[i]) = table1[index++]; \
		}


void PlayBuf_Ctor(PlayBuf *unit)
{
	if (INRATE(1) == calc_FullRate) {
		if (INRATE(2) == calc_FullRate) {
			SETCALC(PlayBuf_next_aa);
		} else {
			SETCALC(PlayBuf_next_ak);
		}
	} else {
		if (INRATE(2) == calc_FullRate) {
			SETCALC(PlayBuf_next_ka);
		} else {
			SETCALC(PlayBuf_next_kk);
		}
	}

	unit->m_fbufnum = -1e9f;
	unit->m_prevtrig = 0.;
	unit->mOut = 0;
	unit->m_phase = ZIN0(3);

	ClearUnitOutputs(unit, 1);
}

void PlayBuf_Dtor(PlayBuf *unit)
{
	TAKEDOWN_OUT
}

void PlayBuf_next_aa(PlayBuf *unit, int inNumSamples)
{
	float *ratein  = ZIN(1);
	float *trigin  = ZIN(2);
	int32 loop     = (int32)ZIN0(4);

	float fbufnum  = ZIN0(0);
	if (fbufnum != unit->m_fbufnum) {
		uint32 bufnum = (int)fbufnum;
		World *world = unit->mWorld;
		if (bufnum >= world->mNumSndBufs) bufnum = 0;
		unit->m_fbufnum = fbufnum;
		unit->m_buf = world->mSndBufs + bufnum;
	}
	SndBuf *buf = unit->m_buf;
	float *bufData __attribute__((__unused__)) = buf->data;
	uint32 bufChannels __attribute__((__unused__)) = buf->channels;
	uint32 bufSamples __attribute__((__unused__)) = buf->samples;
	uint32 bufFrames = buf->frames;
	int mask __attribute__((__unused__)) = buf->mask;
	int guardFrame __attribute__((__unused__)) = bufFrames - 2;

	CHECK_BUF
	SETUP_OUT

	double loopMax = (double)(loop ? bufFrames : bufFrames - 1);
	double phase = unit->m_phase;
	float prevtrig = unit->m_prevtrig;

	for (int i=0; i<inNumSamples; ++i) {
		float trig = ZXP(trigin);
		if (trig > 0.f && prevtrig <= 0.f) {
			unit->mDone = false;
			phase = ZIN0(3);
		}
		prevtrig = trig;

		LOOP_BODY_4

		phase += ZXP(ratein);
	}
	if(unit->mDone) { DoneAction((int)ZIN0(5), unit); }
	unit->m_phase = phase;
	unit->m_prevtrig = prevtrig;
}

void PlayBuf_next_ak(PlayBuf *unit, int inNumSamples)
{
	float *ratein  = ZIN(1);
	float trig     = ZIN0(2);
	int32 loop     = (int32)ZIN0(4);

	float fbufnum  = ZIN0(0);
	if (fbufnum != unit->m_fbufnum) {
		uint32 bufnum = (int)fbufnum;
		World *world = unit->mWorld;
		if (bufnum >= world->mNumSndBufs) bufnum = 0;
		unit->m_fbufnum = fbufnum;
		unit->m_buf = world->mSndBufs + bufnum;
	}
	SndBuf *buf = unit->m_buf;
	float *bufData __attribute__((__unused__)) = buf->data;
	uint32 bufChannels __attribute__((__unused__)) = buf->channels;
	uint32 bufSamples __attribute__((__unused__)) = buf->samples;
	uint32 bufFrames = buf->frames;
	int mask __attribute__((__unused__)) = buf->mask;
	int guardFrame __attribute__((__unused__)) = bufFrames - 2;

	CHECK_BUF
	SETUP_OUT

	double loopMax = (double)(loop ? bufFrames : bufFrames - 1);
	double phase = unit->m_phase;
    if(phase == -1.) phase = bufFrames;
	if (trig > 0.f && unit->m_prevtrig <= 0.f) {
		unit->mDone = false;
		phase = ZIN0(3);
	}
	unit->m_prevtrig = trig;
	for (int i=0; i<inNumSamples; ++i) {

		LOOP_BODY_4

		phase += ZXP(ratein);
	}
	if(unit->mDone) { DoneAction((int)ZIN0(5), unit); }
	unit->m_phase = phase;
}

void PlayBuf_next_kk(PlayBuf *unit, int inNumSamples)
{
	float rate     = ZIN0(1);
	float trig     = ZIN0(2);
	int32 loop     = (int32)ZIN0(4);

	GET_BUF
	CHECK_BUF
	SETUP_OUT

	double loopMax = (double)(loop ? bufFrames : bufFrames - 1);
	double phase = unit->m_phase;
	if (trig > 0.f && unit->m_prevtrig <= 0.f) {
		unit->mDone = false;
		phase = ZIN0(3);
	}
	unit->m_prevtrig = trig;
	for (int i=0; i<inNumSamples; ++i) {

		LOOP_BODY_4

		phase += rate;
	}
	if(unit->mDone) { DoneAction((int)ZIN0(5), unit); }
	unit->m_phase = phase;
}

void PlayBuf_next_ka(PlayBuf *unit, int inNumSamples)
{
	float rate     = ZIN0(1);
	float *trigin  = ZIN(2);
	int32 loop     = (int32)ZIN0(4);

	GET_BUF
	CHECK_BUF
	SETUP_OUT

	double loopMax = (double)(loop ? bufFrames : bufFrames - 1);
	double phase = unit->m_phase;
	float prevtrig = unit->m_prevtrig;
	for (int i=0; i<inNumSamples; ++i) {
		float trig = ZXP(trigin);
		if (trig > 0.f && prevtrig <= 0.f) {
			unit->mDone = false;
			if (INRATE(3) == calc_FullRate) phase = IN(3)[i];
			else phase = ZIN0(3);
		}
		prevtrig = trig;

		LOOP_BODY_4

		phase += rate;
	}
	if(unit->mDone) { DoneAction((int)ZIN0(5), unit); }
	unit->m_phase = phase;
	unit->m_prevtrig = prevtrig;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////

void BufRd_Ctor(BufRd *unit)
{
	int interp = (int)ZIN0(3);
	switch (interp) {
		case 1 : SETCALC(BufRd_next_1); break;
		case 2 : SETCALC(BufRd_next_2); break;
		default : SETCALC(BufRd_next_4); break;
	}

	unit->m_fbufnum = -1e9f;
	unit->mOut = 0;

	ClearUnitOutputs(unit, 1);
}

void BufRd_Dtor(BufRd *unit)
{
	TAKEDOWN_OUT
}

void BufRd_next_4(BufRd *unit, int inNumSamples)
{
	float *phasein = ZIN(1);
	int32 loop     = (int32)ZIN0(2);

	GET_BUF
	CHECK_BUF
	SETUP_OUT

	double loopMax = (double)(loop ? bufFrames : bufFrames - 1);

	for (int i=0; i<inNumSamples; ++i) {
		double phase = ZXP(phasein);

		LOOP_BODY_4

	}
}

void BufRd_next_2(BufRd *unit, int inNumSamples)
{
	float *phasein = ZIN(1);
	int32 loop     = (int32)ZIN0(2);

	GET_BUF
	CHECK_BUF
	SETUP_OUT

	double loopMax = (double)(loop ? bufFrames : bufFrames - 1);

	for (int i=0; i<inNumSamples; ++i) {
		double phase = ZXP(phasein);

		LOOP_BODY_2

	}
}

void BufRd_next_1(BufRd *unit, int inNumSamples)
{
	float *phasein = ZIN(1);
	int32 loop     = (int32)ZIN0(2);

	GET_BUF
	CHECK_BUF
	SETUP_OUT

	double loopMax = (double)(loop ? bufFrames : bufFrames - 1);

	for (int i=0; i<inNumSamples; ++i) {
		double phase = ZXP(phasein);

		LOOP_BODY_1

	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void BufWr_Ctor(BufWr *unit)
{
	SETCALC(BufWr_next);

	unit->m_fbufnum = -1e9f;
	unit->mIn = 0;

	ClearUnitOutputs(unit, 1);
}

void BufWr_Dtor(BufWr *unit)
{
	TAKEDOWN_IN
}

void BufWr_next(BufWr *unit, int inNumSamples)
{
	float *phasein  = ZIN(1);
	int32 loop     = (int32)ZIN0(2);

	GET_BUF
	CHECK_BUF
	SETUP_IN(3)
	double loopMax = (double)(bufFrames - (loop ? 0 : 1));

	for (int32 k=0; k<inNumSamples; ++k) {
		double phase = sc_loop((Unit*)unit, ZXP(phasein), loopMax, loop);
		int32 iphase = (int32)phase;
		float* table0 = bufData + iphase * bufChannels;
		for (uint32 i=0; i<numInputs; ++i) {
			table0[i] = *++(in[i]);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

//bufnum=0, offset=0.0, recLevel=1.0, preLevel=0.0, run=1.0, loop=1.0, trigger=1.0

void RecordBuf_Ctor(RecordBuf *unit)
{

	uint32 numInputs = unit->mNumInputs - 8;
	unit->m_fbufnum = -1e9f;
	unit->mIn = 0;
	unit->m_writepos = (int32)ZIN0(1) * numInputs;
	unit->m_recLevel = ZIN0(2);
	unit->m_preLevel = ZIN0(3);

	if (INRATE(2) == calc_ScalarRate && INRATE(3) == calc_ScalarRate
		&& unit->m_recLevel == 1.0 && unit->m_preLevel == 0.0)
	{
		SETCALC(RecordBuf_next_10);
	} else {
		SETCALC(RecordBuf_next);
	}

	ClearUnitOutputs(unit, 1);
}

void RecordBuf_Dtor(RecordBuf *unit)
{
	TAKEDOWN_IN
}

void RecordBuf_next(RecordBuf *unit, int inNumSamples)
{
	//printf("RecordBuf_next\n");
	GET_BUF
	CHECK_BUF
	SETUP_IN(8)

	float recLevel = ZIN0(2);
	float preLevel = ZIN0(3);
	float run      = ZIN0(4);
	int32 loop     = (int32)ZIN0(5);
	float trig     = ZIN0(6);
	//printf("loop %d  run %g\n", loop, run);

	int32 writepos = unit->m_writepos;

	float recLevel_slope = CALCSLOPE(recLevel, unit->m_recLevel);
	float preLevel_slope = CALCSLOPE(preLevel, unit->m_preLevel);

	/* reset recLevel and preLevel to use the previous value ... bug fix */
	recLevel = unit->m_recLevel;
	preLevel = unit->m_preLevel;

	if (loop) {
		if (trig > 0.f && unit->m_prevtrig <= 0.f) {
			unit->mDone = false;
			writepos = (int32)ZIN0(1) * bufChannels;
		}
		if (writepos < 0) writepos = bufSamples - bufChannels;
		else if (writepos >= (int32)bufSamples) writepos = 0;
		if (run > 0.f) {
			if (bufChannels == 1) {
				for (int32 k=0; k<inNumSamples; ++k) {
					float* table0 = bufData + writepos;
					table0[0] = *++(in[0]) * recLevel + table0[0] * preLevel;
					writepos += 1;
					if (writepos >= (int32)bufSamples) writepos = 0;

					recLevel += recLevel_slope;
					preLevel += preLevel_slope;
				}
			} else if (bufChannels == 2 && numInputs == 2) {
				for (int32 k=0; k<inNumSamples; ++k) {
					float* table0 = bufData + writepos;
					table0[0] = *++(in[0]) * recLevel + table0[0] * preLevel;
					table0[1] = *++(in[1]) * recLevel + table0[1] * preLevel;
					writepos += 2;
					if (writepos >= (int32)bufSamples) writepos = 0;

					recLevel += recLevel_slope;
					preLevel += preLevel_slope;
				}
			} else {
				for (int32 k=0; k<inNumSamples; ++k) {
					float* table0 = bufData + writepos;
					for (uint32 i=0; i<numInputs; ++i) {
						float *samp = table0 + i;
						*samp = *++(in[i]) * recLevel + *samp * preLevel;
					}
					writepos += bufChannels;
					if (writepos >= (int32)bufSamples) writepos = 0;

					recLevel += recLevel_slope;
					preLevel += preLevel_slope;
				}
			}
		} else if (run < 0.f) {
			if (bufChannels == 1) {
				for (int32 k=0; k<inNumSamples; ++k) {
					float* table0 = bufData + writepos;
					table0[0] = *++(in[0]) * recLevel + table0[0] * preLevel;
					writepos -= 1;
					if (writepos < 0) writepos = bufSamples - bufChannels;

					recLevel += recLevel_slope;
					preLevel += preLevel_slope;
				}
			} else if (bufChannels == 2 && numInputs == 2) {
				for (int32 k=0; k<inNumSamples; ++k) {
					float* table0 = bufData + writepos;
					table0[0] = *++(in[0]) * recLevel + table0[0] * preLevel;
					table0[1] = *++(in[1]) * recLevel + table0[1] * preLevel;
					writepos -= 2;
					if (writepos < 0) writepos = bufSamples - bufChannels;

					recLevel += recLevel_slope;
					preLevel += preLevel_slope;
				}
			} else {
				for (int32 k=0; k<inNumSamples; ++k) {
					float* table0 = bufData + writepos;
					for (uint32 i=0; i<numInputs; ++i) {
						float *samp = table0 + i;
						*samp = *++(in[i]) * recLevel + *samp * preLevel;
					}
					writepos -= bufChannels;
					if (writepos < 0) writepos = bufSamples - bufChannels;

					recLevel += recLevel_slope;
					preLevel += preLevel_slope;
				}
			}
		}
	} else {
		if (trig > 0.f && unit->m_prevtrig <= 0.f) {
			unit->mDone = false;
			writepos = (int32)ZIN0(1) * bufChannels;
		}
		if (run > 0.f) {
			int nsmps = bufSamples - writepos;
			nsmps = sc_clip(nsmps, 0, inNumSamples);
			if (bufChannels == 1) {
				for (int32 k=0; k<nsmps; ++k) {
					float* table0 = bufData + writepos;
					table0[0] = *++(in[0]) * recLevel + table0[0] * preLevel;
					writepos += 1;

					recLevel += recLevel_slope;
					preLevel += preLevel_slope;
				}
			} else if (bufChannels == 2 && numInputs == 2) {
				for (int32 k=0; k<nsmps; ++k) {
					float* table0 = bufData + writepos;
					table0[0] = *++(in[0]) * recLevel + table0[0] * preLevel;
					table0[1] = *++(in[1]) * recLevel + table0[1] * preLevel;
					writepos += 2;

					recLevel += recLevel_slope;
					preLevel += preLevel_slope;
				}
			} else {
				for (int32 k=0; k<nsmps; ++k) {
					float* table0 = bufData + writepos;
					for (uint32 i=0; i<numInputs; ++i) {
						float *samp = table0 + i;
						*samp = *++(in[i]) * recLevel + *samp * preLevel;
					}
					writepos += bufChannels;

					recLevel += recLevel_slope;
					preLevel += preLevel_slope;
				}
			}
		} else if (run < 0.f) {
			int nsmps = writepos;
			nsmps = sc_clip(nsmps, 0, inNumSamples);
			if (bufChannels == 1) {
				for (int32 k=0; k<inNumSamples; ++k) {
					float* table0 = bufData + writepos;
					table0[0] = *++(in[0]) * recLevel + table0[0] * preLevel;
					writepos -= bufChannels;

					recLevel += recLevel_slope;
					preLevel += preLevel_slope;
				}
			} else if (bufChannels == 2 && numInputs == 2) {
				for (int32 k=0; k<inNumSamples; ++k) {
					float* table0 = bufData + writepos;
					table0[0] = *++(in[0]) * recLevel + table0[0] * preLevel;
					table0[1] = *++(in[1]) * recLevel + table0[1] * preLevel;
					writepos -= bufChannels;

					recLevel += recLevel_slope;
					preLevel += preLevel_slope;
				}
			} else {
				for (int32 k=0; k<inNumSamples; ++k) {
					float* table0 = bufData + writepos;
					for (uint32 i=0; i<numInputs; ++i) {
						float *samp = table0 + i;
						*samp = *++(in[i]) * recLevel + *samp * preLevel;
					}
					writepos -= bufChannels;

					recLevel += recLevel_slope;
					preLevel += preLevel_slope;
				}
			}
		}
		if (writepos >= (int32)bufSamples){
			unit->mDone = true;
			DoneAction(IN0(7), unit);
		}
	}
	unit->m_prevtrig = trig;
	unit->m_writepos = writepos;
	unit->m_recLevel = recLevel;
	unit->m_preLevel = preLevel;
}

void RecordBuf_next_10(RecordBuf *unit, int inNumSamples)
{
	// printf("RecordBuf_next_10\n");
	GET_BUF
	CHECK_BUF
	SETUP_IN(8)

	float run      = ZIN0(4);
	int32 loop     = (int32)ZIN0(5);
	float trig     = ZIN0(6);
	//printf("loop %d  run %g\n", loop, run);

	int32 writepos = unit->m_writepos;

	if (loop) {
		if (trig > 0.f && unit->m_prevtrig <= 0.f) {
			unit->mDone = false;
			writepos = (int32)ZIN0(1) * bufChannels;
		}
		if (writepos < 0) writepos = bufSamples - bufChannels;
		else if (writepos >= (int32)bufSamples) writepos = 0;
		if (run > 0.f) {
			if (bufChannels == 1) {
				for (int32 k=0; k<inNumSamples; ++k) {
					float* table0 = bufData + writepos;
					table0[0] = *++(in[0]);
					writepos += 1;
					if (writepos >= (int32)bufSamples) writepos = 0;
				}
			} else if (bufChannels == 2) {
				for (int32 k=0; k<inNumSamples; ++k) {
					float* table0 = bufData + writepos;
					table0[0] = *++(in[0]);
					table0[1] = *++(in[1]);
					writepos += 2;
					if (writepos >= (int32)bufSamples) writepos = 0;
				}
			} else {
				for (int32 k=0; k<inNumSamples; ++k) {
					float* table0 = bufData + writepos;
					for (uint32 i=0; i<bufChannels; ++i) {
						float *samp = table0 + i;
						*samp = *++(in[i]);
					}
					writepos += bufChannels;
					if (writepos >= (int32)bufSamples) writepos = 0;
				}
			}
		} else if (run < 0.f) {
			if (bufChannels == 1) {
				for (int32 k=0; k<inNumSamples; ++k) {
					float* table0 = bufData + writepos;
					table0[0] = *++(in[0]);
					writepos -= 1;
					if (writepos < 0) writepos = bufSamples - bufChannels;
				}
			} else if (bufChannels == 2) {
				for (int32 k=0; k<inNumSamples; ++k) {
					float* table0 = bufData + writepos;
					table0[0] = *++(in[0]);
					table0[1] = *++(in[1]);
					writepos -= 2;
					if (writepos < 0) writepos = bufSamples - bufChannels;
				}
			} else {
				for (int32 k=0; k<inNumSamples; ++k) {
					float* table0 = bufData + writepos;
					for (uint32 i=0; i<bufChannels; ++i) {
						float *samp = table0 + i;
						*samp = *++(in[i]);
					}
					writepos -= bufChannels;
					if (writepos < 0) writepos = bufSamples - bufChannels;
				}
			}
		}
	} else {
		if (trig > 0.f && unit->m_prevtrig <= 0.f) {
			unit->mDone = false;
			writepos = (int32)ZIN0(1) * bufChannels;
		}
		if (run > 0.f) {
			int nsmps = bufSamples - writepos;
			nsmps = sc_clip(nsmps, 0, inNumSamples);
			if (bufChannels == 1) {
				for (int32 k=0; k<nsmps; ++k) {
					float* table0 = bufData + writepos;
					table0[0] = *++(in[0]);
					writepos += 1;
				}
			} else if (bufChannels == 2) {
				for (int32 k=0; k<nsmps; ++k) {
					float* table0 = bufData + writepos;
					table0[0] = *++(in[0]);
					table0[1] = *++(in[1]);
					writepos += 2;
					if (writepos >= (int32)bufSamples) writepos = (int32)bufSamples - 2; // added by jrhb
				}
			} else {
				for (int32 k=0; k<nsmps; ++k) {
					float* table0 = bufData + writepos;
					for (uint32 i=0; i<bufChannels; ++i) {
						float *samp = table0 + i;
						*samp = *++(in[i]);
					}
					writepos += bufChannels;
					if (writepos >= (int32)bufSamples) writepos = (int32)bufSamples - bufChannels; // added by jrhb
				}
			}
		} else if (run < 0.f) {
			int nsmps = writepos;
			nsmps = sc_clip(nsmps, 0, inNumSamples);
			if (bufChannels == 1) {
				for (int32 k=0; k<inNumSamples; ++k) {
					float* table0 = bufData + writepos;
					table0[0] = *++(in[0]);
					writepos -= 1;
				}
			} else if (bufChannels == 2) {
				for (int32 k=0; k<inNumSamples; ++k) {
					float* table0 = bufData + writepos;
					table0[0] = *++(in[0]);
					table0[1] = *++(in[1]);
					writepos -= 2;
				}
			} else {
				for (int32 k=0; k<inNumSamples; ++k) {
					float* table0 = bufData + writepos;
					for (uint32 i=0; i<bufChannels; ++i) {
						float *samp = table0 + i;
						*samp = *++(in[i]);
					}
					writepos -= bufChannels;
				}
			}
		}
		if (writepos >= (int32)bufSamples){
			unit->mDone = true;
			DoneAction(IN0(7), unit);
		}
	}
	unit->m_prevtrig = trig;
	unit->m_writepos = writepos;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////



float insertMedian(float* values, int* ages, int size, float value);
float insertMedian(float* values, int* ages, int size, float value)
{
	int i, last, pos=-1;

	// keeps a sorted list of the previous n=size values
	// the oldest is removed and the newest is inserted.
	// values between the oldest and the newest are shifted over by one.

	// values and ages are both arrays that are 'size' int.
	// the median value is always values[size>>1]

	last = size - 1;
	// find oldest bin and age the other bins.
	for (i=0; i<size; ++i) {
		if (ages[i] == last) { // is it the oldest bin ?
			pos = i;
		} else {
			ages[i]++;	// age the bin
		}
	}
	// move values to fill in place of the oldest and make a space for the newest
	// search lower if value is too small for the open space
	while (pos != 0 && value < values[pos-1]) {
		values[pos] = values[pos-1];
		ages[pos] = ages[pos-1];
		pos--;
	}
	// search higher if value is too big for the open space
	while (pos != last && value > values[pos+1]) {
		values[pos] = values[pos+1];
		ages[pos] = ages[pos+1];
		pos++;
	}
	values[pos] = value;
	ages[pos] = 0;		// this is the newest bin, age = 0
	return values[size>>1];
}

void initMedian(float* values, int* ages, int size, float value);
void initMedian(float* values, int* ages, int size, float value)
{
	int i;

	// initialize the arrays with the first value
	for (i=0; i<size; ++i) {
		values[i] = value;
		ages[i] = i;
	}
}




enum {
	kPitchIn,
	kPitchInitFreq,
	kPitchMinFreq,
	kPitchMaxFreq,
	kPitchExecFreq,
	kPitchMaxBins,
	kPitchMedian,
	kPitchAmpThreshold,
	kPitchPeakThreshold,
	kPitchDownsamp
};

void Pitch_Ctor(Pitch *unit)
{
	unit->m_freq = ZIN0(kPitchInitFreq);
	unit->m_minfreq = ZIN0(kPitchMinFreq);
	unit->m_maxfreq = ZIN0(kPitchMaxFreq);

	float execfreq = ZIN0(kPitchExecFreq);
	execfreq = sc_clip(execfreq, unit->m_minfreq, unit->m_maxfreq);

	int maxbins = (int)ZIN0(kPitchMaxBins);
	unit->m_maxlog2bins = LOG2CEIL(maxbins);

	unit->m_medianSize = sc_clip((int)ZIN0(0), 0, kMAXMEDIANSIZE);  // (int)ZIN0(kPitchMedian);
	unit->m_ampthresh = ZIN0(kPitchAmpThreshold);
	unit->m_peakthresh = ZIN0(kPitchPeakThreshold);

	int downsamp = (int)ZIN0(kPitchDownsamp);

	if (INRATE(kPitchIn) == calc_FullRate) {
		SETCALC(Pitch_next_a);
	 	unit->m_downsamp = sc_clip(downsamp, 1, unit->mWorld->mFullRate.mBufLength);
    	unit->m_srate = FULLRATE / (float)unit->m_downsamp;
	} else {
 		SETCALC(Pitch_next_k);
	 	unit->m_downsamp = sc_max(downsamp, 1);
		unit->m_srate = FULLRATE / (float) (unit->mWorld->mFullRate.mBufLength*unit->m_downsamp);
	}

    unit->m_minperiod = (long)(unit->m_srate / unit->m_maxfreq);
    unit->m_maxperiod = (long)(unit->m_srate / unit->m_minfreq);

	unit->m_execPeriod = (int)(unit->m_srate / execfreq);
	unit->m_execPeriod = sc_max(unit->m_execPeriod, unit->mWorld->mFullRate.mBufLength);

	unit->m_size = unit->m_maxperiod << 1;

	unit->m_buffer = (float*)RTAlloc(unit->mWorld, unit->m_size * sizeof(float));

	unit->m_index = 0;
	unit->m_readp = 0;
	unit->m_hasfreq = 0.f;

	initMedian(unit->m_values, unit->m_ages, unit->m_medianSize, unit->m_freq);

	ZOUT0(0) = 0.f;
	ZOUT0(1) = 0.f;
}

void Pitch_Dtor(Pitch *unit)
{
	RTFree(unit->mWorld, unit->m_buffer);
}

void Pitch_next_a(Pitch *unit, int inNumSamples)
{
	bool foundPeak;

	float* in = ZIN(kPitchIn);
	uint32 size = unit->m_size;
	uint32 index = unit->m_index;
	int downsamp = unit->m_downsamp;
	int readp = unit->m_readp;
	int ksamps = unit->mWorld->mFullRate.mBufLength;

	float *bufData = unit->m_buffer;

	float freq = unit->m_freq;
	float hasfreq = unit->m_hasfreq;
	//printf("> %d %d readp %d ksamps %d ds %d\n", index, size, readp, ksamps, downsamp);
	do {
		float z = in[readp];
		bufData[index++] = z;
		readp += downsamp;

		if (index >= size) {
			float ampthresh = unit->m_ampthresh;
			bool ampok = false;

			hasfreq = 0.f; // assume failure

			int minperiod = unit->m_minperiod;
			int maxperiod = unit->m_maxperiod;
			//float maxamp = 0.f;
			// check for amp threshold
			for (int j = 0; j < maxperiod; ++j) {
				if (fabs(bufData[j]) >= ampthresh) {
					ampok = true;
					break;
				}
				//if (fabs(bufData[j]) > maxamp) maxamp = fabs(bufData[j]);
			}
			//printf("ampok %d  maxperiod %d  maxamp %g\n", ampok, maxperiod,  maxamp);

			// if amplitude is too small then don't even look for pitch
			float ampsum;
			if (ampok) {
				int maxlog2bins = unit->m_maxlog2bins;
				int octave;
				// calculate the zero lag value and compute the threshold based on that
				float threshold = 0.f;
				for (int j = 0; j < maxperiod; ++j) {
					threshold += bufData[j] * bufData[j];
				}
				threshold *= unit->m_peakthresh;

				// skip until drop below threshold
				int binstep, peakbinstep = 0;
				int i;
				for (i = 1; i <= maxperiod; i += binstep) {
					// compute sum of one lag
					ampsum = 0.f;
					for (int j = 0; j < maxperiod; ++j) {
						ampsum += bufData[i+j] * bufData[j];
					}
					if (ampsum < threshold) break;

					octave = LOG2CEIL(i);
					if (octave <= maxlog2bins) {
						binstep = 1;
					} else {
						binstep = 1L << (octave - maxlog2bins);
					}
				}
				int startperiod = i;
				int period = startperiod;
				//printf("startperiod %d\n", startperiod);

				// find the first peak
				float maxsum = threshold;
				foundPeak = false;
				for (i = startperiod; i <= maxperiod; i += binstep) {
					if (i >= minperiod) {
						ampsum = 0.f;
						for (int j = 0; j < maxperiod; ++j) {
							ampsum += bufData[i+j] * bufData[j];
						}
						if (ampsum > threshold) {
							if (ampsum > maxsum) {
								foundPeak = true;
								maxsum = ampsum;
								peakbinstep = binstep;
								period = i;
							}
						} else if (foundPeak) break;
					}
					octave = LOG2CEIL(i);
					if (octave <= maxlog2bins) {
						binstep = 1;
					} else {
						binstep = 1L << (octave - maxlog2bins);
					}
				}

				//printf("found %d  thr %g  maxs %g  per %d  bs %d\n", foundPeak, threshold, maxsum, period, peakbinstep);
				if (foundPeak) {
					float prevampsum, nextampsum;

					// find amp sums immediately surrounding max
					prevampsum = 0.f;
					if (period > 0) {
						i = period - 1;
						for (int j = 0; j < maxperiod; ++j) {
							prevampsum += bufData[i+j] * bufData[j];
						}
					}

					nextampsum = 0.f;
					if (period < maxperiod) {
						i = period + 1;
						for (int j = 0; j < maxperiod; ++j) {
							nextampsum += bufData[i+j] * bufData[j];
						}
					}

					//printf("prevnext %g %g %g   %d\n", prevampsum, maxsum, nextampsum, period);
					// not on a peak yet. This can happen if binstep > 1
					while (prevampsum > maxsum && period > 0) {
						nextampsum = maxsum;
						maxsum = prevampsum;
						period--;
						i = period - 1;
						prevampsum = 0.f;
						for (int j = 0; j < maxperiod; ++j) {
							prevampsum += bufData[i+j] * bufData[j];
						}
						//printf("slide left %g %g %g   %d\n", prevampsum, maxsum, nextampsum, period);
					}
					while (nextampsum > maxsum && period < maxperiod) {
						prevampsum = maxsum;
						maxsum = nextampsum;
						period++;
						i = period + 1;
						nextampsum = 0.f;
						for (int j = 0; j < maxperiod; ++j) {
							nextampsum += bufData[i+j] * bufData[j];
						}
						//printf("slide right %g %g %g   %d\n", prevampsum, maxsum, nextampsum, period);
					}

					// make a fractional period
					float beta = 0.5 * (nextampsum - prevampsum);
					float gamma = 2.0  * maxsum - nextampsum - prevampsum;
					float fperiod = (float)period + (beta/gamma);

					// calculate frequency
					float tempfreq = unit->m_srate / fperiod;

					//printf("freq %g   %g / %g    %g %g  %d\n", tempfreq, unit->m_srate, fperiod,
					//	unit->m_minfreq, unit->m_maxfreq,
					//  tempfreq >= unit->m_minfreq && tempfreq <= unit->m_maxfreq);

					if (tempfreq >= unit->m_minfreq && tempfreq <= unit->m_maxfreq) {
						freq = tempfreq;

						// median filter
						if (unit->m_medianSize > 1) {
							freq = insertMedian(unit->m_values, unit->m_ages, unit->m_medianSize, freq);
						}
						hasfreq = 1.f;

						startperiod = (ksamps+downsamp-1)/downsamp;
					}
				}
			}/* else {
                printf("amp too low \n");
            }*/

			// shift buffer for next fill
			int execPeriod = unit->m_execPeriod;
			int interval = size - execPeriod;
			//printf("interval %d  sz %d ep %d\n", interval, size, execPeriod);
			for (int i = 0; i < interval; i++) {
				bufData[i] = bufData[i + execPeriod];
			}
			index = interval;
		}
	} while (readp < ksamps);

	ZOUT0(0) = freq;
	ZOUT0(1) = hasfreq;
	unit->m_readp = readp - ksamps;
	unit->m_index = index;
	unit->m_freq = freq;
	unit->m_hasfreq = hasfreq;
}


// control rate pitch tracking (nescivi 11/2008)
void Pitch_next_k(Pitch *unit, int inNumSamples)
{
	bool foundPeak;

	float in = ZIN0(kPitchIn); // one sample, current input
	uint32 size = unit->m_size;
	uint32 index = unit->m_index;
	int downsamp = unit->m_downsamp;
	int readp = unit->m_readp;
//  	int ksamps = unit->mWorld->mFullRate.mBufLength;

	float *bufData = unit->m_buffer;

	float freq = unit->m_freq;
	float hasfreq = unit->m_hasfreq;
// 	printf("> %d %d readp %d downsamp %d exec %d\n", index, size, readp, downsamp, unit->m_execPeriod);
	readp++;
	if ( readp == downsamp ){
// 	do {
// 		float z = in[readp];
		float z = in;
		bufData[index++] = z;
		readp = 0;
// 		readp += downsamp;

		if (index >= size) {
			float ampthresh = unit->m_ampthresh;
			bool ampok = false;

			hasfreq = 0.f; // assume failure

			int minperiod = unit->m_minperiod;
			int maxperiod = unit->m_maxperiod;
			//float maxamp = 0.f;
			// check for amp threshold
			for (int j = 0; j < maxperiod; ++j) {
				if (fabs(bufData[j]) >= ampthresh) {
					ampok = true;
					break;
				}
				//if (fabs(bufData[j]) > maxamp) maxamp = fabs(bufData[j]);
			}
			//printf("ampok %d  maxperiod %d  maxamp %g\n", ampok, maxperiod,  maxamp);

			// if amplitude is too small then don't even look for pitch
			float ampsum;
			if (ampok) {
				int maxlog2bins = unit->m_maxlog2bins;
				int octave;
				// calculate the zero lag value and compute the threshold based on that
				float threshold = 0.f;
				for (int j = 0; j < maxperiod; ++j) {
					threshold += bufData[j] * bufData[j];
				}
				threshold *= unit->m_peakthresh;

				// skip until drop below threshold
				int binstep, peakbinstep = 0;
				int i;
				for (i = 1; i <= maxperiod; i += binstep) {
					// compute sum of one lag
					ampsum = 0.f;
					for (int j = 0; j < maxperiod; ++j) {
						ampsum += bufData[i+j] * bufData[j];
					}
					if (ampsum < threshold) break;

					octave = LOG2CEIL(i);
					if (octave <= maxlog2bins) {
						binstep = 1;
					} else {
						binstep = 1L << (octave - maxlog2bins);
					}
				}
				int startperiod = i;
				int period = startperiod;
				//printf("startperiod %d\n", startperiod);

				// find the first peak
				float maxsum = threshold;
				foundPeak = false;
				for (i = startperiod; i <= maxperiod; i += binstep) {
					if (i >= minperiod) {
						ampsum = 0.f;
						for (int j = 0; j < maxperiod; ++j) {
							ampsum += bufData[i+j] * bufData[j];
						}
						if (ampsum > threshold) {
							if (ampsum > maxsum) {
								foundPeak = true;
								maxsum = ampsum;
								peakbinstep = binstep;
								period = i;
							}
						} else if (foundPeak) break;
					}
					octave = LOG2CEIL(i);
					if (octave <= maxlog2bins) {
						binstep = 1;
					} else {
						binstep = 1L << (octave - maxlog2bins);
					}
				}

				//printf("found %d  thr %g  maxs %g  per %d  bs %d\n", foundPeak, threshold, maxsum, period, peakbinstep);
				if (foundPeak) {
					float prevampsum, nextampsum;

					// find amp sums immediately surrounding max
					prevampsum = 0.f;
					if (period > 0) {
						i = period - 1;
						for (int j = 0; j < maxperiod; ++j) {
							prevampsum += bufData[i+j] * bufData[j];
						}
					}

					nextampsum = 0.f;
					if (period < maxperiod) {
						i = period + 1;
						for (int j = 0; j < maxperiod; ++j) {
							nextampsum += bufData[i+j] * bufData[j];
						}
					}

					//printf("prevnext %g %g %g   %d\n", prevampsum, maxsum, nextampsum, period);
					// not on a peak yet. This can happen if binstep > 1
					while (prevampsum > maxsum && period > 0) {
						nextampsum = maxsum;
						maxsum = prevampsum;
						period--;
						i = period - 1;
						prevampsum = 0.f;
						for (int j = 0; j < maxperiod; ++j) {
							prevampsum += bufData[i+j] * bufData[j];
						}
						//printf("slide left %g %g %g   %d\n", prevampsum, maxsum, nextampsum, period);
					}
					while (nextampsum > maxsum && period < maxperiod) {
						prevampsum = maxsum;
						maxsum = nextampsum;
						period++;
						i = period + 1;
						nextampsum = 0.f;
						for (int j = 0; j < maxperiod; ++j) {
							nextampsum += bufData[i+j] * bufData[j];
						}
						//printf("slide right %g %g %g   %d\n", prevampsum, maxsum, nextampsum, period);
					}

					// make a fractional period
					float beta = 0.5 * (nextampsum - prevampsum);
					float gamma = 2.0  * maxsum - nextampsum - prevampsum;
					float fperiod = (float)period + (beta/gamma);

					// calculate frequency
					float tempfreq = unit->m_srate / fperiod;

					//printf("freq %g   %g / %g    %g %g  %d\n", tempfreq, unit->m_srate, fperiod,
					//	unit->m_minfreq, unit->m_maxfreq,
					//  tempfreq >= unit->m_minfreq && tempfreq <= unit->m_maxfreq);

					if (tempfreq >= unit->m_minfreq && tempfreq <= unit->m_maxfreq) {
						freq = tempfreq;

						// median filter
						if (unit->m_medianSize > 1) {
							freq = insertMedian(unit->m_values, unit->m_ages, unit->m_medianSize, freq);
						}
						hasfreq = 1.f;

						// nescivi: not sure about this one?
						startperiod = 1; // (ksamps+downsamp-1)/downsamp;
					}
				}
			}/* else {
                printf("amp too low \n");
            }*/

			// shift buffer for next fill
			int execPeriod = unit->m_execPeriod;
			int interval = size - execPeriod;
			//printf("interval %d  sz %d ep %d\n", interval, size, execPeriod);
			for (int i = 0; i < interval; i++) {
				bufData[i] = bufData[i + execPeriod];
			}
			index = interval;
		}
	}
//while (readp < ksamps);

	ZOUT0(0) = freq;
	ZOUT0(1) = hasfreq;
// 	unit->m_readp = readp - ksamps;
	unit->m_readp = readp;
	unit->m_index = index;
	unit->m_freq = freq;
	unit->m_hasfreq = hasfreq;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////


#if 0
void DelayUnit_AllocDelayLine(DelayUnit *unit)
{
	long delaybufsize = (long)ceil(unit->m_maxdelaytime * SAMPLERATE + 1.f);
	delaybufsize = delaybufsize + BUFLENGTH;
	delaybufsize = NEXTPOWEROFTWO(delaybufsize);  // round up to next power of two
	unit->m_fdelaylen = unit->m_idelaylen = delaybufsize;

	RTFree(unit->mWorld, unit->m_dlybuf);
	int size = delaybufsize * sizeof(float);
	//Print("->RTAlloc %d\n", size);
	unit->m_dlybuf = (float*)RTAlloc(unit->mWorld, size);
	//Print("<-RTAlloc %08X\n", unit->m_dlybuf);
	unit->m_mask = delaybufsize - 1;
}
#endif

#define BufCalcDelay(delaytime) (sc_clip(delaytime * SAMPLERATE, 1.f, (float)bufSamples))

void BufDelayUnit_Reset(BufDelayUnit *unit)
{
	//Print("->DelayUnit_Reset\n");
	//unit->m_maxdelaytime = ZIN0(1);
	unit->m_delaytime = ZIN0(2);
	//Print("unit->m_delaytime %g\n", unit->m_delaytime);
	//unit->m_dlybuf = 0;
	unit->m_fbufnum = -1e9f;

	//DelayUnit_AllocDelayLine(unit);
	//Print("->GET_BUF\n");
	GET_BUF
	//Print("<-GET_BUF\n");
	unit->m_dsamp = BufCalcDelay(unit->m_delaytime);
	unit->m_numoutput = 0;
	unit->m_iwrphase = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void BufFeedbackDelay_Reset(BufFeedbackDelay *unit)
{
	BufDelayUnit_Reset(unit);

	unit->m_decaytime = ZIN0(3);
	unit->m_feedbk = CalcFeedback(unit->m_delaytime, unit->m_decaytime);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////

void BufDelayN_Ctor(BufDelayN *unit)
{
	SETCALC(BufDelayN_next_z);
	BufDelayUnit_Reset(unit);
	ZOUT0(0) = 0.f;
}

void BufDelayN_next(BufDelayN *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(1);
	float delaytime = ZIN0(2);

	GET_BUF
	CHECK_BUF
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;

	if (delaytime == unit->m_delaytime) {
		long irdphase = iwrphase - (long)dsamp;
		float* dlybuf1 = bufData - ZOFF;
		float* dlyrd   = dlybuf1 + (irdphase & mask);
		float* dlywr   = dlybuf1 + (iwrphase & mask);
		float* dlyN    = dlybuf1 + bufSamples;
		long remain = inNumSamples;
		while (remain) {
			long rdspace = dlyN - dlyrd;
			long wrspace = dlyN - dlywr;
			long nsmps = sc_min(rdspace, wrspace);
			if (nsmps == 0) NodeEnd(&unit->mParent->mNode); // buffer not allocated.
			nsmps = sc_min(remain, nsmps);
			remain -= nsmps;
			LOOP(nsmps,
				ZXP(dlywr) = ZXP(in);
				ZXP(out) = ZXP(dlyrd);
			);
			if (dlyrd == dlyN) dlyrd = dlybuf1;
			if (dlywr == dlyN) dlywr = dlybuf1;
		}
		iwrphase += inNumSamples;
	} else {
		float next_dsamp = BufCalcDelay(delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		LOOP(inNumSamples,
			bufData[iwrphase & mask] = ZXP(in);
			dsamp += dsamp_slope;
			++iwrphase;
			long irdphase = iwrphase - (long)dsamp;
			ZXP(out) = bufData[irdphase & mask];
		);
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
	}

	unit->m_iwrphase = iwrphase;

}


void BufDelayN_next_z(BufDelayN *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(1);
	float delaytime = ZIN0(2);

	GET_BUF
	CHECK_BUF
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;

	if (delaytime == unit->m_delaytime) {
		long irdphase = iwrphase - (long)dsamp;
		float* dlybuf1 = bufData - ZOFF;
		float* dlyN    = dlybuf1 + bufSamples;
		long remain = inNumSamples;
		while (remain) {
			float* dlywr = dlybuf1 + (iwrphase & mask);
			float* dlyrd = dlybuf1 + (irdphase & mask);
			long rdspace = dlyN - dlyrd;
			long wrspace = dlyN - dlywr;
			long nsmps = sc_min(rdspace, wrspace);
			if (nsmps == 0) NodeEnd(&unit->mParent->mNode); // buffer not allocated.
			nsmps = sc_min(remain, nsmps);
			remain -= nsmps;
			if (irdphase < 0) {
				LOOP(nsmps,
					ZXP(dlywr) = ZXP(in);
					ZXP(out) = 0.f;
				);
			} else {
				LOOP(nsmps,
					ZXP(dlywr) = ZXP(in);
					ZXP(out) = ZXP(dlyrd);
				);
			}
			iwrphase += nsmps;
			irdphase += nsmps;
		}
	} else {

		float next_dsamp = BufCalcDelay(delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long irdphase = iwrphase - (long)dsamp;

			if (irdphase < 0) {
				bufData[iwrphase & mask] = ZXP(in);
				ZXP(out) = 0.f;
			} else {
				bufData[iwrphase & mask] = ZXP(in);
				ZXP(out) = bufData[irdphase & mask];
			}
			iwrphase++;
		);
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
	}

	unit->m_iwrphase = iwrphase;

	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= bufSamples) {
		SETCALC(BufDelayN_next);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

void BufDelayL_Ctor(BufDelayL *unit)
{
	BufDelayUnit_Reset(unit);
	SETCALC(BufDelayL_next_z);
	ZOUT0(0) = 0.f;
}

void BufDelayL_next(BufDelayL *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(1);
	float delaytime = ZIN0(2);

	GET_BUF
	CHECK_BUF
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;

	if (delaytime == unit->m_delaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			bufData[iwrphase & mask] = ZXP(in);
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;
			float d1 = bufData[irdphase & mask];
			float d2 = bufData[irdphaseb & mask];
			ZXP(out) = lininterp(frac, d1, d2);
			iwrphase++;
		);
	} else {

		float next_dsamp = BufCalcDelay(delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		LOOP(inNumSamples,
			bufData[iwrphase & mask] = ZXP(in);
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;
			float d1 = bufData[irdphase & mask];
			float d2 = bufData[irdphaseb & mask];
			ZXP(out) = lininterp(frac, d1, d2);
			iwrphase++;
		);
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
	}

	unit->m_iwrphase = iwrphase;

}


void BufDelayL_next_z(BufDelayL *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(1);
	float delaytime = ZIN0(2);

	GET_BUF
	CHECK_BUF
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;

	if (delaytime == unit->m_delaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;

			bufData[iwrphase & mask] = ZXP(in);
			if (irdphase < 0) {
				ZXP(out) = 0.f;
			} else if (irdphaseb < 0) {
				float d1 = bufData[irdphase & mask];
				ZXP(out) = d1 - frac * d1;
			} else {
				float d1 = bufData[irdphase & mask];
				float d2 = bufData[irdphaseb & mask];
				ZXP(out) = lininterp(frac, d1, d2);
			}
			iwrphase++;
		);
	} else {

		float next_dsamp = BufCalcDelay(delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;

			bufData[iwrphase & mask] = ZXP(in);
			if (irdphase < 0) {
				ZXP(out) = 0.f;
			} else if (irdphaseb < 0) {
				float d1 = bufData[irdphase & mask];
				ZXP(out) = d1 - frac * d1;
			} else {
				float d1 = bufData[irdphase & mask];
				float d2 = bufData[irdphaseb & mask];
				ZXP(out) = lininterp(frac, d1, d2);
			}
			iwrphase++;
		);
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
	}

	unit->m_iwrphase = iwrphase;

	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= bufSamples) {
		SETCALC(BufDelayL_next);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void BufDelayC_Ctor(BufDelayC *unit)
{
	BufDelayUnit_Reset(unit);
	SETCALC(BufDelayC_next_z);
	ZOUT0(0) = 0.f;
}

void BufDelayC_next(BufDelayC *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(1);
	float delaytime = ZIN0(2);

	GET_BUF
	CHECK_BUF
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;

	if (delaytime == unit->m_delaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			bufData[iwrphase & mask] = ZXP(in);
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			float d0 = bufData[irdphase0 & mask];
			float d1 = bufData[irdphase1 & mask];
			float d2 = bufData[irdphase2 & mask];
			float d3 = bufData[irdphase3 & mask];
			ZXP(out) = cubicinterp(frac, d0, d1, d2, d3);
			iwrphase++;
		);
	} else {

		float next_dsamp = BufCalcDelay(delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		LOOP(inNumSamples,
			bufData[iwrphase & mask] = ZXP(in);
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			float d0 = bufData[irdphase0 & mask];
			float d1 = bufData[irdphase1 & mask];
			float d2 = bufData[irdphase2 & mask];
			float d3 = bufData[irdphase3 & mask];
			ZXP(out) = cubicinterp(frac, d0, d1, d2, d3);
			iwrphase++;
		);
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
	}

	unit->m_iwrphase = iwrphase;

}


void BufDelayC_next_z(BufDelayC *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(1);
	float delaytime = ZIN0(2);

	GET_BUF
	CHECK_BUF
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float d0, d1, d2, d3;

	if (delaytime == unit->m_delaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;

			bufData[iwrphase & mask] = ZXP(in);
			if (irdphase0 < 0) {
				ZXP(out) = 0.f;
			} else {
				if (irdphase1 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = bufData[irdphase0 & mask];
				} else if (irdphase2 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = bufData[irdphase0 & mask];
					d1 = bufData[irdphase1 & mask];
				} else if (irdphase3 < 0) {
					d3 = 0.f;
					d0 = bufData[irdphase0 & mask];
					d1 = bufData[irdphase1 & mask];
					d2 = bufData[irdphase2 & mask];
				} else {
					d0 = bufData[irdphase0 & mask];
					d1 = bufData[irdphase1 & mask];
					d2 = bufData[irdphase2 & mask];
					d3 = bufData[irdphase3 & mask];
				}
				ZXP(out) = cubicinterp(frac, d0, d1, d2, d3);
			}
			iwrphase++;
		);
	} else {

		float next_dsamp = BufCalcDelay(delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;

			bufData[iwrphase & mask] = ZXP(in);
			if (irdphase0 < 0) {
				ZXP(out) = 0.f;
			} else {
				if (irdphase1 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = bufData[irdphase0 & mask];
				} else if (irdphase2 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = bufData[irdphase0 & mask];
					d1 = bufData[irdphase1 & mask];
				} else if (irdphase3 < 0) {
					d3 = 0.f;
					d0 = bufData[irdphase0 & mask];
					d1 = bufData[irdphase1 & mask];
					d2 = bufData[irdphase2 & mask];
				} else {
					d0 = bufData[irdphase0 & mask];
					d1 = bufData[irdphase1 & mask];
					d2 = bufData[irdphase2 & mask];
					d3 = bufData[irdphase3 & mask];
				}
				ZXP(out) = cubicinterp(frac, d0, d1, d2, d3);
			}
			iwrphase++;
		);
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
	}

	unit->m_iwrphase = iwrphase;

	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= bufSamples) {
		SETCALC(BufDelayC_next);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

void BufCombN_Ctor(BufCombN *unit)
{
	SETCALC(BufCombN_next_z);
	BufFeedbackDelay_Reset(unit);
	ZOUT0(0) = 0.f;
}

void BufCombN_next(BufCombN *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(1);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	GET_BUF
	CHECK_BUF
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;

	//postbuf("BufCombN_next %g %g %g %g %d %d %d\n", delaytime, decaytime, feedbk, dsamp, mask, iwrphase, zorg);
	if (delaytime == unit->m_delaytime) {
		long irdphase = iwrphase - (long)dsamp;
		float* dlybuf1 = bufData - ZOFF;
		float* dlyrd   = dlybuf1 + (irdphase & mask);
		float* dlywr   = dlybuf1 + (iwrphase & mask);
		float* dlyN    = dlybuf1 + bufSamples;
		if (decaytime == unit->m_decaytime) {
			long remain = inNumSamples;
			while (remain) {
				long rdspace = dlyN - dlyrd;
				long wrspace = dlyN - dlywr;
				long nsmps = sc_min(rdspace, wrspace);
				if (nsmps == 0) NodeEnd(&unit->mParent->mNode); // buffer not allocated.
				nsmps = sc_min(remain, nsmps);
				remain -= nsmps;
				LOOP(nsmps,
					float value = ZXP(dlyrd);
					ZXP(dlywr) = value * feedbk + ZXP(in);
					ZXP(out) = value;
				);
				if (dlyrd == dlyN) dlyrd = dlybuf1;
				if (dlywr == dlyN) dlywr = dlybuf1;
			}
		} else {
			float next_feedbk = CalcFeedback(delaytime, decaytime);
			float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);
			long remain = inNumSamples;
			while (remain) {
				long rdspace = dlyN - dlyrd;
				long wrspace = dlyN - dlywr;
				long nsmps = sc_min(rdspace, wrspace);
				if (nsmps == 0) NodeEnd(&unit->mParent->mNode); // buffer not allocated.
				nsmps = sc_min(remain, nsmps);
				remain -= nsmps;

				LOOP(nsmps,
					float value = ZXP(dlyrd);
					ZXP(dlywr) = value * feedbk + ZXP(in);
					ZXP(out) = value;
					feedbk += feedbk_slope;
				);
				if (dlyrd == dlyN) dlyrd = dlybuf1;
				if (dlywr == dlyN) dlywr = dlybuf1;
			}
			unit->m_feedbk = feedbk;
			unit->m_decaytime = decaytime;
		}
		iwrphase += inNumSamples;
	} else {
		float next_dsamp = BufCalcDelay(delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);
		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			++iwrphase;
			long irdphase = iwrphase - (long)dsamp;
			float value = bufData[irdphase & mask];
			bufData[iwrphase & mask] = ZXP(in) + feedbk * value;
			ZXP(out) = value;
			feedbk += feedbk_slope;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

}


void BufCombN_next_z(BufCombN *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(1);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	GET_BUF
	CHECK_BUF
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;

	//Print("BufCombN_next_z %g %g %g %g %d %d %d\n", delaytime, decaytime, feedbk, dsamp, mask, iwrphase, zorg);
	if (delaytime == unit->m_delaytime) {
		long irdphase = iwrphase - (long)dsamp;
		float* dlybuf1 = bufData - ZOFF;
		float* dlyN    = dlybuf1 + bufSamples;
		if (decaytime == unit->m_decaytime) {
			long remain = inNumSamples;
			while (remain) {
				float* dlywr = dlybuf1 + (iwrphase & mask);
				float* dlyrd = dlybuf1 + (irdphase & mask);
				long rdspace = dlyN - dlyrd;
				long wrspace = dlyN - dlywr;
				long nsmps = sc_min(rdspace, wrspace);
				if (nsmps == 0) NodeEnd(&unit->mParent->mNode); // buffer not allocated.
				nsmps = sc_min(remain, nsmps);
				remain -= nsmps;
				if (irdphase < 0) {
					LOOP(nsmps,
						ZXP(dlywr) = ZXP(in);
						ZXP(out) = 0.f;
					);
				} else {
					LOOP(nsmps,
						float value = ZXP(dlyrd);
						ZXP(dlywr) = value * feedbk + ZXP(in);
						ZXP(out) = value;
					);
				}
				iwrphase += nsmps;
				irdphase += nsmps;
			}
		} else {
			float next_feedbk = CalcFeedback(delaytime, decaytime);
			float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);
			long remain = inNumSamples;
			while (remain) {
				float* dlyrd = dlybuf1 + (irdphase & mask);
				float* dlywr = dlybuf1 + (iwrphase & mask);
				long rdspace = dlyN - dlyrd;
				long wrspace = dlyN - dlywr;
				long nsmps = sc_min(rdspace, wrspace);
				if (nsmps == 0) NodeEnd(&unit->mParent->mNode); // buffer not allocated.
				nsmps = sc_min(remain, nsmps);
				remain -= nsmps;

				if (irdphase < 0) {
					feedbk += nsmps * feedbk_slope;
					dlyrd += nsmps;
					LOOP(nsmps,
						ZXP(dlywr) = ZXP(in);
						ZXP(out) = 0.f;
					);
				} else {
					LOOP(nsmps,
						float value = ZXP(dlyrd);
						ZXP(dlywr) = value * feedbk + ZXP(in);
						ZXP(out) = value;
						feedbk += feedbk_slope;
					);
				}
				iwrphase += nsmps;
				irdphase += nsmps;
			}
			unit->m_feedbk = feedbk;
			unit->m_decaytime = decaytime;
		}
	} else {

		float next_dsamp = BufCalcDelay(delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long irdphase = iwrphase - (long)dsamp;

			if (irdphase < 0) {
				bufData[iwrphase & mask] = ZXP(in);
				ZXP(out) = 0.f;
			} else {
				float value = bufData[irdphase & mask];
				bufData[iwrphase & mask] = ZXP(in) + feedbk * value;
				ZXP(out) = value;
			}
			feedbk += feedbk_slope;
			iwrphase++;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= bufSamples) {
		SETCALC(BufCombN_next);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

void BufCombL_Ctor(BufCombL *unit)
{
	BufFeedbackDelay_Reset(unit);
	SETCALC(BufCombL_next_z);
	ZOUT0(0) = 0.f;
}

void BufCombL_next(BufCombL *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(1);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	GET_BUF
	CHECK_BUF
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;
			float d1 = bufData[irdphase & mask];
			float d2 = bufData[irdphaseb & mask];
			float value = lininterp(frac, d1, d2);
			bufData[iwrphase & mask] = ZXP(in) + feedbk * value;
			ZXP(out) = value;
			iwrphase++;
		);
	} else {

		float next_dsamp = BufCalcDelay(delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;
			float d1 = bufData[irdphase & mask];
			float d2 = bufData[irdphaseb & mask];
			float value = lininterp(frac, d1, d2);
			bufData[iwrphase & mask] = ZXP(in) + feedbk * value;
			ZXP(out) = value;
			feedbk += feedbk_slope;
			iwrphase++;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

}


void BufCombL_next_z(BufCombL *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(1);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	GET_BUF
	CHECK_BUF
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;

			float zin = ZXP(in);
			if (irdphase < 0) {
				bufData[iwrphase & mask] = zin;
				ZXP(out) = 0.f;
			} else if (irdphaseb < 0) {
				float d1 = bufData[irdphase & mask];
				float value = d1 - frac * d1;
				bufData[iwrphase & mask] = zin + feedbk * value;
				//postbuf("A %d d1 %g fr %g v %g in %g fb %g\n", irdphase, d1, frac, value, zin, feedbk);
				ZXP(out) = value;
			} else {
				float d1 = bufData[irdphase & mask];
				float d2 = bufData[irdphaseb & mask];
				float value = lininterp(frac, d1, d2);
				bufData[iwrphase & mask] = zin + feedbk * value;
				//postbuf("B %d d1 %g d2 %g fr %g v %g in %g fb %g\n", irdphase, d1, d2, frac, value, zin, feedbk);
				ZXP(out) = value;
			}
			iwrphase++;
		);
	} else {

		float next_dsamp = BufCalcDelay(delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;

			float zin = ZXP(in);
			if (irdphase < 0) {
				ZXP(out) = 0.f;
				bufData[iwrphase & mask] = zin;
			} else if (irdphaseb < 0) {
				float d1 = bufData[irdphase & mask];
				float value = d1 - frac * d1;
				bufData[iwrphase & mask] = zin + feedbk * value;
				ZXP(out) = value;
			} else {
				float d1 = bufData[irdphase & mask];
				float d2 = bufData[irdphaseb & mask];
				float value = lininterp(frac, d1, d2);
				bufData[iwrphase & mask] = zin + feedbk * value;
				ZXP(out) = value;
			}
			feedbk += feedbk_slope;
			iwrphase++;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= bufSamples) {
		SETCALC(BufCombL_next);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void BufCombC_Ctor(BufCombC *unit)
{
	BufFeedbackDelay_Reset(unit);
	SETCALC(BufCombC_next_z);
	ZOUT0(0) = 0.f;
}

void BufCombC_next(BufCombC *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(1);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	GET_BUF
	CHECK_BUF
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			float d0 = bufData[irdphase0 & mask];
			float d1 = bufData[irdphase1 & mask];
			float d2 = bufData[irdphase2 & mask];
			float d3 = bufData[irdphase3 & mask];
			float value = cubicinterp(frac, d0, d1, d2, d3);
			bufData[iwrphase & mask] = ZXP(in) + feedbk * value;
			ZXP(out) = value;
			iwrphase++;
		);
	} else {

		float next_dsamp = BufCalcDelay(delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			float d0 = bufData[irdphase0 & mask];
			float d1 = bufData[irdphase1 & mask];
			float d2 = bufData[irdphase2 & mask];
			float d3 = bufData[irdphase3 & mask];
			float value = cubicinterp(frac, d0, d1, d2, d3);
			bufData[iwrphase & mask] = ZXP(in) + feedbk * value;
			ZXP(out) = value;
			feedbk += feedbk_slope;
			iwrphase++;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

}


void BufCombC_next_z(BufCombC *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(1);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	GET_BUF
	CHECK_BUF
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	float d0, d1, d2, d3;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;

			if (irdphase0 < 0) {
				bufData[iwrphase & mask] = ZXP(in);
				ZXP(out) = 0.f;
			} else {
				if (irdphase1 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = bufData[irdphase0 & mask];
				} else if (irdphase2 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = bufData[irdphase0 & mask];
					d1 = bufData[irdphase1 & mask];
				} else if (irdphase3 < 0) {
					d3 = 0.f;
					d0 = bufData[irdphase0 & mask];
					d1 = bufData[irdphase1 & mask];
					d2 = bufData[irdphase2 & mask];
				} else {
					d0 = bufData[irdphase0 & mask];
					d1 = bufData[irdphase1 & mask];
					d2 = bufData[irdphase2 & mask];
					d3 = bufData[irdphase3 & mask];
				}
				float value = cubicinterp(frac, d0, d1, d2, d3);
				bufData[iwrphase & mask] = ZXP(in) + feedbk * value;
				ZXP(out) = value;
			}
			iwrphase++;
		);
	} else {

		float next_dsamp = BufCalcDelay(delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;

			if (irdphase0 < 0) {
				bufData[iwrphase & mask] = ZXP(in);
				ZXP(out) = 0.f;
			} else {
				if (irdphase1 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = bufData[irdphase0 & mask];
				} else if (irdphase2 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = bufData[irdphase0 & mask];
					d1 = bufData[irdphase1 & mask];
				} else if (irdphase3 < 0) {
					d3 = 0.f;
					d0 = bufData[irdphase0 & mask];
					d1 = bufData[irdphase1 & mask];
					d2 = bufData[irdphase2 & mask];
				} else {
					d0 = bufData[irdphase0 & mask];
					d1 = bufData[irdphase1 & mask];
					d2 = bufData[irdphase2 & mask];
					d3 = bufData[irdphase3 & mask];
				}
				float value = cubicinterp(frac, d0, d1, d2, d3);
				bufData[iwrphase & mask] = ZXP(in) + feedbk * value;
				ZXP(out) = value;
			}
			feedbk += feedbk_slope;
			iwrphase++;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= bufSamples) {
		SETCALC(BufCombC_next);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

void BufAllpassN_Ctor(BufAllpassN *unit)
{
	SETCALC(BufAllpassN_next_z);
	BufFeedbackDelay_Reset(unit);
	ZOUT0(0) = 0.f;
}

void BufAllpassN_next(BufAllpassN *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(1);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	GET_BUF
	CHECK_BUF
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;

	//postbuf("BufAllpassN_next %g %g %g %g %d %d %d\n", delaytime, decaytime, feedbk, dsamp, mask, iwrphase, zorg);
	if (delaytime == unit->m_delaytime) {
		long irdphase = iwrphase - (long)dsamp;
		float* dlybuf1 = bufData - ZOFF;
		float* dlyrd   = dlybuf1 + (irdphase & mask);
		float* dlywr   = dlybuf1 + (iwrphase & mask);
		float* dlyN    = dlybuf1 + bufSamples;
		if (decaytime == unit->m_decaytime) {
			long remain = inNumSamples;
			while (remain) {
				long rdspace = dlyN - dlyrd;
				long wrspace = dlyN - dlywr;
				long nsmps = sc_min(rdspace, wrspace);
				if (nsmps == 0) NodeEnd(&unit->mParent->mNode); // buffer not allocated.
				nsmps = sc_min(remain, nsmps);
				remain -= nsmps;
				LOOP(nsmps,
					float value = ZXP(dlyrd);
					float dwr = value * feedbk + ZXP(in);
					ZXP(dlywr) = dwr;
					ZXP(out) = value - feedbk * dwr;
				);
				if (dlyrd == dlyN) dlyrd = dlybuf1;
				if (dlywr == dlyN) dlywr = dlybuf1;
			}
		} else {
			float next_feedbk = CalcFeedback(delaytime, decaytime);
			float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);
			long remain = inNumSamples;
			while (remain) {
				long rdspace = dlyN - dlyrd;
				long wrspace = dlyN - dlywr;
				long nsmps = sc_min(rdspace, wrspace);
				if (nsmps == 0) NodeEnd(&unit->mParent->mNode); // buffer not allocated.
				nsmps = sc_min(remain, nsmps);
				remain -= nsmps;

				LOOP(nsmps,
					float value = ZXP(dlyrd);
					float dwr = value * feedbk + ZXP(in);
					ZXP(dlywr) = dwr;
					ZXP(out) = value - feedbk * dwr;
					feedbk += feedbk_slope;
				);
				if (dlyrd == dlyN) dlyrd = dlybuf1;
				if (dlywr == dlyN) dlywr = dlybuf1;
			}
			unit->m_feedbk = feedbk;
			unit->m_decaytime = decaytime;
		}
		iwrphase += inNumSamples;
	} else {
		float next_dsamp = BufCalcDelay(delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);
		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			++iwrphase;
			long irdphase = iwrphase - (long)dsamp;
			float value = bufData[irdphase & mask];
			float dwr = value * feedbk + ZXP(in);
			bufData[iwrphase & mask] = dwr;
			ZXP(out) = value - feedbk * dwr;
			feedbk += feedbk_slope;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

}


void BufAllpassN_next_z(BufAllpassN *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(1);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	GET_BUF
	CHECK_BUF
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;

	//postbuf("BufAllpassN_next_z %g %g %g %g %d %d %d\n", delaytime, decaytime, feedbk, dsamp, mask, iwrphase, zorg);
	if (delaytime == unit->m_delaytime) {
		long irdphase = iwrphase - (long)dsamp;
		float* dlybuf1 = bufData - ZOFF;
		float* dlyN    = dlybuf1 + bufSamples;
		if (decaytime == unit->m_decaytime) {
			long remain = inNumSamples;
			while (remain) {
				float* dlywr = dlybuf1 + (iwrphase & mask);
				float* dlyrd = dlybuf1 + (irdphase & mask);
				long rdspace = dlyN - dlyrd;
				long wrspace = dlyN - dlywr;
				long nsmps = sc_min(rdspace, wrspace);
				if (nsmps == 0) NodeEnd(&unit->mParent->mNode); // buffer not allocated.
				nsmps = sc_min(remain, nsmps);
				remain -= nsmps;
				if (irdphase < 0) {
					feedbk = -feedbk;
					LOOP(nsmps,
						float dwr = ZXP(in);
						ZXP(dlywr) = dwr;
						ZXP(out) = feedbk * dwr;
					);
					feedbk = -feedbk;
				} else {
					LOOP(nsmps,
						float x1 = ZXP(dlyrd);
						float dwr = x1 * feedbk + ZXP(in);
						ZXP(dlywr) = dwr;
						ZXP(out) = x1 - feedbk * dwr;
					);
				}
				iwrphase += nsmps;
				irdphase += nsmps;
			}
		} else {
			float next_feedbk = CalcFeedback(delaytime, decaytime);
			float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);
			long remain = inNumSamples;
			while (remain) {
				float* dlyrd = dlybuf1 + (irdphase & mask);
				float* dlywr = dlybuf1 + (iwrphase & mask);
				long rdspace = dlyN - dlyrd;
				long wrspace = dlyN - dlywr;
				long nsmps = sc_min(rdspace, wrspace);
				if (nsmps == 0) NodeEnd(&unit->mParent->mNode); // buffer not allocated.
				nsmps = sc_min(remain, nsmps);
				remain -= nsmps;

				if (irdphase < 0) {
					dlyrd += nsmps;
					LOOP(nsmps,
						float dwr = ZXP(in);
						ZXP(dlywr) = dwr;
						ZXP(out) = -feedbk * dwr;
						feedbk += feedbk_slope;
					);
				} else {
					LOOP(nsmps,
						float x1 = ZXP(dlyrd);
						float dwr = x1 * feedbk + ZXP(in);
						ZXP(dlywr) = dwr;
						ZXP(out) = x1 - feedbk * dwr;
						feedbk += feedbk_slope;
					);
				}
				iwrphase += nsmps;
				irdphase += nsmps;
			}
			unit->m_feedbk = feedbk;
			unit->m_decaytime = decaytime;
		}
	} else {

		float next_dsamp = BufCalcDelay(delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long irdphase = iwrphase - (long)dsamp;

			if (irdphase < 0) {
				float dwr = ZXP(in);
				bufData[iwrphase & mask] = dwr;
				ZXP(out) = -feedbk * dwr;
			} else {
				float value = bufData[irdphase & mask];
				float dwr = feedbk * value + ZXP(in);
				bufData[iwrphase & mask] = dwr;
				ZXP(out) = value - feedbk * dwr;
			}
			feedbk += feedbk_slope;
			iwrphase++;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= bufSamples) {
		SETCALC(BufAllpassN_next);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

void BufAllpassL_Ctor(BufAllpassL *unit)
{
	BufFeedbackDelay_Reset(unit);
	SETCALC(BufAllpassL_next_z);
	ZOUT0(0) = 0.f;
}

void BufAllpassL_next(BufAllpassL *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(1);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	GET_BUF
	CHECK_BUF
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;
			float d1 = bufData[irdphase & mask];
			float d2 = bufData[irdphaseb & mask];
			float value = lininterp(frac, d1, d2);
			float dwr = ZXP(in) + feedbk * value;
			bufData[iwrphase & mask] = dwr;
			ZXP(out) = value - feedbk * dwr;


			iwrphase++;
		);
	} else {

		float next_dsamp = BufCalcDelay(delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;
			float d1 = bufData[irdphase & mask];
			float d2 = bufData[irdphaseb & mask];
			float value = lininterp(frac, d1, d2);
			float dwr = ZXP(in) + feedbk * value;
			bufData[iwrphase & mask] = dwr;
			ZXP(out) = value - feedbk * dwr;
			feedbk += feedbk_slope;
			iwrphase++;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

}


void BufAllpassL_next_z(BufAllpassL *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(1);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	GET_BUF
	CHECK_BUF
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;

			float zin = ZXP(in);
			if (irdphase < 0) {
				bufData[iwrphase & mask] = zin;
				ZXP(out) = - feedbk * zin;
			} else if (irdphaseb < 0) {
				float d1 = bufData[irdphase & mask];
				float value = d1 - frac * d1;
				float dwr = zin + feedbk * value;
				bufData[iwrphase & mask] = dwr;
				ZXP(out) = value - feedbk * dwr;
			} else {
				float d1 = bufData[irdphase & mask];
				float d2 = bufData[irdphaseb & mask];
				float value = lininterp(frac, d1, d2);
				float dwr = zin + feedbk * value;
				bufData[iwrphase & mask] = dwr;
				ZXP(out) = value - feedbk * dwr;
			}
			iwrphase++;
		);
	} else {

		float next_dsamp = BufCalcDelay(delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;

			float zin = ZXP(in);
			if (irdphase < 0) {
				bufData[iwrphase & mask] = zin;
				ZXP(out) = - feedbk * zin;
			} else if (irdphaseb < 0) {
				float d1 = bufData[irdphase & mask];
				float value = d1 - frac * d1;
				float dwr = zin + feedbk * value;
				bufData[iwrphase & mask] = dwr;
				ZXP(out) = value - feedbk * dwr;
			} else {
				float d1 = bufData[irdphase & mask];
				float d2 = bufData[irdphaseb & mask];
				float value = lininterp(frac, d1, d2);
				float dwr = zin + feedbk * value;
				bufData[iwrphase & mask] = dwr;
				ZXP(out) = value - feedbk * dwr;
			}
			feedbk += feedbk_slope;
			iwrphase++;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= bufSamples) {
		SETCALC(BufAllpassL_next);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

void BufAllpassC_Ctor(BufAllpassC *unit)
{
	BufFeedbackDelay_Reset(unit);
	SETCALC(BufAllpassC_next_z);
	ZOUT0(0) = 0.f;
}

void BufAllpassC_next(BufAllpassC *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(1);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	GET_BUF
	CHECK_BUF
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			float d0 = bufData[irdphase0 & mask];
			float d1 = bufData[irdphase1 & mask];
			float d2 = bufData[irdphase2 & mask];
			float d3 = bufData[irdphase3 & mask];
			float value = cubicinterp(frac, d0, d1, d2, d3);
			float dwr = ZXP(in) + feedbk * value;
			bufData[iwrphase & mask] = dwr;
			ZXP(out) = value - feedbk * dwr;
			iwrphase++;
		);
	} else {

		float next_dsamp = BufCalcDelay(delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			float d0 = bufData[irdphase0 & mask];
			float d1 = bufData[irdphase1 & mask];
			float d2 = bufData[irdphase2 & mask];
			float d3 = bufData[irdphase3 & mask];
			float value = cubicinterp(frac, d0, d1, d2, d3);
			float dwr = ZXP(in) + feedbk * value;
			bufData[iwrphase & mask] = dwr;
			ZXP(out) = value - feedbk * dwr;
			feedbk += feedbk_slope;
			iwrphase++;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

}


void BufAllpassC_next_z(BufAllpassC *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(1);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	GET_BUF
	CHECK_BUF
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	float d0, d1, d2, d3;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;

			if (irdphase0 < 0) {
				bufData[iwrphase & mask] = ZXP(in);
				ZXP(out) = 0.f;
			} else {
				if (irdphase1 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = bufData[irdphase0 & mask];
				} else if (irdphase2 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = bufData[irdphase0 & mask];
					d1 = bufData[irdphase1 & mask];
				} else if (irdphase3 < 0) {
					d3 = 0.f;
					d0 = bufData[irdphase0 & mask];
					d1 = bufData[irdphase1 & mask];
					d2 = bufData[irdphase2 & mask];
				} else {
					d0 = bufData[irdphase0 & mask];
					d1 = bufData[irdphase1 & mask];
					d2 = bufData[irdphase2 & mask];
					d3 = bufData[irdphase3 & mask];
				}
				float value = cubicinterp(frac, d0, d1, d2, d3);
				float dwr = ZXP(in) + feedbk * value;
				bufData[iwrphase & mask] = dwr;
				ZXP(out) = value - feedbk * dwr;
			}
			iwrphase++;
		);
	} else {

		float next_dsamp = BufCalcDelay(delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;

			if (irdphase0 < 0) {
				bufData[iwrphase & mask] = ZXP(in);
				ZXP(out) = 0.f;
			} else {
				if (irdphase1 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = bufData[irdphase0 & mask];
				} else if (irdphase2 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = bufData[irdphase0 & mask];
					d1 = bufData[irdphase1 & mask];
				} else if (irdphase3 < 0) {
					d3 = 0.f;
					d0 = bufData[irdphase0 & mask];
					d1 = bufData[irdphase1 & mask];
					d2 = bufData[irdphase2 & mask];
				} else {
					d0 = bufData[irdphase0 & mask];
					d1 = bufData[irdphase1 & mask];
					d2 = bufData[irdphase2 & mask];
					d3 = bufData[irdphase3 & mask];
				}
				float value = cubicinterp(frac, d0, d1, d2, d3);
				float dwr = ZXP(in) + feedbk * value;
				bufData[iwrphase & mask] = dwr;
				ZXP(out) = value - feedbk * dwr;
			}
			feedbk += feedbk_slope;
			iwrphase++;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= bufSamples) {
		SETCALC(BufAllpassC_next);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

void DelayUnit_AllocDelayLine(DelayUnit *unit)
{
	long delaybufsize = (long)ceil(unit->m_maxdelaytime * SAMPLERATE + 1.f);
	delaybufsize = delaybufsize + BUFLENGTH;
	delaybufsize = NEXTPOWEROFTWO(delaybufsize);  // round up to next power of two
	unit->m_fdelaylen = unit->m_idelaylen = delaybufsize;

	RTFree(unit->mWorld, unit->m_dlybuf);
	unit->m_dlybuf = (float*)RTAlloc(unit->mWorld, delaybufsize * sizeof(float));
	unit->m_mask = delaybufsize - 1;
}

float CalcDelay(DelayUnit *unit, float delaytime);
float CalcDelay(DelayUnit *unit, float delaytime)
{
	float next_dsamp = delaytime * SAMPLERATE;
	return sc_clip(next_dsamp, 1.f, unit->m_fdelaylen);
}

void DelayUnit_Reset(DelayUnit *unit)
{
	unit->m_maxdelaytime = ZIN0(1);
	unit->m_delaytime = ZIN0(2);
	unit->m_dlybuf = 0;

	DelayUnit_AllocDelayLine(unit);

	unit->m_dsamp = CalcDelay(unit, unit->m_delaytime);

	unit->m_numoutput = 0;
	unit->m_iwrphase = 0;
}


void DelayUnit_Dtor(DelayUnit *unit)
{
	RTFree(unit->mWorld, unit->m_dlybuf);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void FeedbackDelay_Reset(FeedbackDelay *unit)
{
	unit->m_decaytime = ZIN0(3);

	DelayUnit_Reset(unit);

	unit->m_feedbk = CalcFeedback(unit->m_delaytime, unit->m_decaytime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void DelayN_Ctor(DelayN *unit)
{
	DelayUnit_Reset(unit);
	SETCALC(DelayN_next_z);
	ZOUT0(0) = 0.f;
}

void DelayN_next(DelayN *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	float delaytime = ZIN0(2);

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	long mask = unit->m_mask;

	//Print("DelayN_next %08X %g %g  %d %d\n", unit, delaytime, dsamp, mask, iwrphase);
	if (delaytime == unit->m_delaytime) {
		long irdphase = iwrphase - (long)dsamp;
		float* dlybuf1 = dlybuf - ZOFF;
		float* dlyrd   = dlybuf1 + (irdphase & mask);
		float* dlywr   = dlybuf1 + (iwrphase & mask);
		float* dlyN    = dlybuf1 + unit->m_idelaylen;
		long remain = inNumSamples;
		while (remain) {
			long rdspace = dlyN - dlyrd;
			long wrspace = dlyN - dlywr;
			long nsmps = sc_min(rdspace, wrspace);
			nsmps = sc_min(remain, nsmps);
			remain -= nsmps;
			LOOP(nsmps,
				ZXP(dlywr) = ZXP(in);
				ZXP(out) = ZXP(dlyrd);
			);
			if (dlyrd == dlyN) dlyrd = dlybuf1;
			if (dlywr == dlyN) dlywr = dlybuf1;
		}
		iwrphase += inNumSamples;
	} else {
		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		LOOP(inNumSamples,
			dlybuf[iwrphase & mask] = ZXP(in);
			dsamp += dsamp_slope;
			++iwrphase;
			long irdphase = iwrphase - (long)dsamp;
			ZXP(out) = dlybuf[irdphase & mask];
		);
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
	}

	unit->m_iwrphase = iwrphase;

}


void DelayN_next_z(DelayN *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	float delaytime = ZIN0(2);

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	long mask = unit->m_mask;

	if (delaytime == unit->m_delaytime) {
		long irdphase = iwrphase - (long)dsamp;
		float* dlybuf1 = dlybuf - ZOFF;
		float* dlyN    = dlybuf1 + unit->m_idelaylen;
		long remain = inNumSamples;
		while (remain) {
			float* dlywr = dlybuf1 + (iwrphase & mask);
			float* dlyrd = dlybuf1 + (irdphase & mask);
			long rdspace = dlyN - dlyrd;
			long wrspace = dlyN - dlywr;
			long nsmps = sc_min(rdspace, wrspace);
			nsmps = sc_min(remain, nsmps);
			remain -= nsmps;
			if (irdphase < 0) {
				LOOP(nsmps,
					ZXP(dlywr) = ZXP(in);
					ZXP(out) = 0.f;
				);
			} else {
				LOOP(nsmps,
					ZXP(dlywr) = ZXP(in);
					ZXP(out) = ZXP(dlyrd);
				);
			}
			iwrphase += nsmps;
			irdphase += nsmps;
		}
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long irdphase = iwrphase - (long)dsamp;

			dlybuf[iwrphase & mask] = ZXP(in);
			if (irdphase < 0) {
				ZXP(out) = 0.f;
			} else {
				ZXP(out) = dlybuf[irdphase & mask];
			}
			iwrphase++;
		);
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
	}

	unit->m_iwrphase = iwrphase;

	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= unit->m_idelaylen) {
		SETCALC(DelayN_next);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

void DelayL_Ctor(DelayL *unit)
{
	DelayUnit_Reset(unit);
	SETCALC(DelayL_next_z);
	ZOUT0(0) = 0.f;
}

void DelayL_next(DelayL *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	float delaytime = ZIN0(2);

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	long mask = unit->m_mask;

	if (delaytime == unit->m_delaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			dlybuf[iwrphase & mask] = ZXP(in);
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;
			float d1 = dlybuf[irdphase & mask];
			float d2 = dlybuf[irdphaseb & mask];
			ZXP(out) = lininterp(frac, d1, d2);
			iwrphase++;
		);
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		LOOP(inNumSamples,
			dlybuf[iwrphase & mask] = ZXP(in);
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;
			float d1 = dlybuf[irdphase & mask];
			float d2 = dlybuf[irdphaseb & mask];
			ZXP(out) = lininterp(frac, d1, d2);
			iwrphase++;
		);
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
	}

	unit->m_iwrphase = iwrphase;

}


void DelayL_next_z(DelayL *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	float delaytime = ZIN0(2);

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	long mask = unit->m_mask;

	if (delaytime == unit->m_delaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;

			dlybuf[iwrphase & mask] = ZXP(in);
			if (irdphase < 0) {
				ZXP(out) = 0.f;
			} else if (irdphaseb < 0) {
				float d1 = dlybuf[irdphase & mask];
				//postbuf("A %d d1 %g fr %g v %g in %g fb %g\n", irdphase, d1, frac, value, zin, feedbk);
				ZXP(out) = d1 - frac * d1;
			} else {
				float d1 = dlybuf[irdphase & mask];
				float d2 = dlybuf[irdphaseb & mask];
				//postbuf("B %d d1 %g d2 %g fr %g v %g in %g fb %g\n", irdphase, d1, d2, frac, value, zin, feedbk);
				ZXP(out) = lininterp(frac, d1, d2);
			}
			iwrphase++;
		);
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;

			dlybuf[iwrphase & mask] = ZXP(in);
			if (irdphase < 0) {
				ZXP(out) = 0.f;
			} else if (irdphaseb < 0) {
				float d1 = dlybuf[irdphase & mask];
				ZXP(out) = d1 - frac * d1;
			} else {
				float d1 = dlybuf[irdphase & mask];
				float d2 = dlybuf[irdphaseb & mask];
				ZXP(out) = lininterp(frac, d1, d2);
			}
			iwrphase++;
		);
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
	}

	unit->m_iwrphase = iwrphase;

	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= unit->m_idelaylen) {
		SETCALC(DelayL_next);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void DelayC_Ctor(DelayC *unit)
{
	DelayUnit_Reset(unit);
	SETCALC(DelayC_next_z);
	ZOUT0(0) = 0.f;
}

void DelayC_next(DelayC *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	float delaytime = ZIN0(2);

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	long mask = unit->m_mask;

	if (delaytime == unit->m_delaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			dlybuf[iwrphase & mask] = ZXP(in);
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			float d0 = dlybuf[irdphase0 & mask];
			float d1 = dlybuf[irdphase1 & mask];
			float d2 = dlybuf[irdphase2 & mask];
			float d3 = dlybuf[irdphase3 & mask];
			ZXP(out) = cubicinterp(frac, d0, d1, d2, d3);
			iwrphase++;
		);
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		LOOP(inNumSamples,
			dlybuf[iwrphase & mask] = ZXP(in);
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			float d0 = dlybuf[irdphase0 & mask];
			float d1 = dlybuf[irdphase1 & mask];
			float d2 = dlybuf[irdphase2 & mask];
			float d3 = dlybuf[irdphase3 & mask];
			ZXP(out) = cubicinterp(frac, d0, d1, d2, d3);
			iwrphase++;
		);
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
	}

	unit->m_iwrphase = iwrphase;

}


void DelayC_next_z(DelayC *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	float delaytime = ZIN0(2);

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	long mask = unit->m_mask;
	float d0, d1, d2, d3;

	if (delaytime == unit->m_delaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;

			dlybuf[iwrphase & mask] = ZXP(in);
			if (irdphase0 < 0) {
				ZXP(out) = 0.f;
			} else {
				if (irdphase1 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
				} else if (irdphase2 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
				} else if (irdphase3 < 0) {
					d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
				} else {
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
					d3 = dlybuf[irdphase3 & mask];
				}
				ZXP(out) = cubicinterp(frac, d0, d1, d2, d3);
			}
			iwrphase++;
		);
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;

			dlybuf[iwrphase & mask] = ZXP(in);
			if (irdphase0 < 0) {
				ZXP(out) = 0.f;
			} else {
				if (irdphase1 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
				} else if (irdphase2 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
				} else if (irdphase3 < 0) {
					d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
				} else {
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
					d3 = dlybuf[irdphase3 & mask];
				}
				ZXP(out) = cubicinterp(frac, d0, d1, d2, d3);
			}
			iwrphase++;
		);
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
	}

	unit->m_iwrphase = iwrphase;

	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= unit->m_idelaylen) {
		SETCALC(DelayC_next);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

void CombN_Ctor(CombN *unit)
{
	SETCALC(CombN_next_z);
	FeedbackDelay_Reset(unit);
	ZOUT0(0) = 0.f;
}

void CombN_next(CombN *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	long mask = unit->m_mask;

	//postbuf("CombN_next %g %g %g %g %d %d %d\n", delaytime, decaytime, feedbk, dsamp, mask, iwrphase, zorg);
	if (delaytime == unit->m_delaytime) {
		long irdphase = iwrphase - (long)dsamp;
		float* dlybuf1 = dlybuf - ZOFF;
		float* dlyrd   = dlybuf1 + (irdphase & mask);
		float* dlywr   = dlybuf1 + (iwrphase & mask);
		float* dlyN    = dlybuf1 + unit->m_idelaylen;
		if (decaytime == unit->m_decaytime) {
			long remain = inNumSamples;
			while (remain) {
				long rdspace = dlyN - dlyrd;
				long wrspace = dlyN - dlywr;
				long nsmps = sc_min(rdspace, wrspace);
				nsmps = sc_min(remain, nsmps);
				remain -= nsmps;
				LOOP(nsmps,
					float value = ZXP(dlyrd);
					ZXP(dlywr) = value * feedbk + ZXP(in);
					ZXP(out) = value;
				);
				if (dlyrd == dlyN) dlyrd = dlybuf1;
				if (dlywr == dlyN) dlywr = dlybuf1;
			}
		} else {
			float next_feedbk = CalcFeedback(delaytime, decaytime);
			float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);
			long remain = inNumSamples;
			while (remain) {
				long rdspace = dlyN - dlyrd;
				long wrspace = dlyN - dlywr;
				long nsmps = sc_min(rdspace, wrspace);
				nsmps = sc_min(remain, nsmps);
				remain -= nsmps;

				LOOP(nsmps,
					float value = ZXP(dlyrd);
					ZXP(dlywr) = value * feedbk + ZXP(in);
					ZXP(out) = value;
					feedbk += feedbk_slope;
				);
				if (dlyrd == dlyN) dlyrd = dlybuf1;
				if (dlywr == dlyN) dlywr = dlybuf1;
			}
			unit->m_feedbk = feedbk;
			unit->m_decaytime = decaytime;
		}
		iwrphase += inNumSamples;
	} else {
		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);
		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			++iwrphase;
			long irdphase = iwrphase - (long)dsamp;
			float value = dlybuf[irdphase & mask];
			dlybuf[iwrphase & mask] = ZXP(in) + feedbk * value;
			ZXP(out) = value;
			feedbk += feedbk_slope;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

}


void CombN_next_z(CombN *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	long mask = unit->m_mask;

	//postbuf("CombN_next_z %g %g %g %g %d %d %d\n", delaytime, decaytime, feedbk, dsamp, mask, iwrphase, zorg);
	if (delaytime == unit->m_delaytime) {
		long irdphase = iwrphase - (long)dsamp;
		float* dlybuf1 = dlybuf - ZOFF;
		float* dlyN    = dlybuf1 + unit->m_idelaylen;
		if (decaytime == unit->m_decaytime) {
			long remain = inNumSamples;
			while (remain) {
				float* dlywr = dlybuf1 + (iwrphase & mask);
				float* dlyrd = dlybuf1 + (irdphase & mask);
				long rdspace = dlyN - dlyrd;
				long wrspace = dlyN - dlywr;
				long nsmps = sc_min(rdspace, wrspace);
				nsmps = sc_min(remain, nsmps);
				remain -= nsmps;
				if (irdphase < 0) {
					LOOP(nsmps,
						ZXP(dlywr) = ZXP(in);
						ZXP(out) = 0.f;
					);
				} else {
					LOOP(nsmps,
						float value = ZXP(dlyrd);
						ZXP(dlywr) = value * feedbk + ZXP(in);
						ZXP(out) = value;
					);
				}
				iwrphase += nsmps;
				irdphase += nsmps;
			}
		} else {
			float next_feedbk = CalcFeedback(delaytime, decaytime);
			float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);
			long remain = inNumSamples;
			while (remain) {
				float* dlyrd = dlybuf1 + (irdphase & mask);
				float* dlywr = dlybuf1 + (iwrphase & mask);
				long rdspace = dlyN - dlyrd;
				long wrspace = dlyN - dlywr;
				long nsmps = sc_min(rdspace, wrspace);
				nsmps = sc_min(remain, nsmps);
				remain -= nsmps;

				if (irdphase < 0) {
					feedbk += nsmps * feedbk_slope;
					dlyrd += nsmps;
					LOOP(nsmps,
						ZXP(dlywr) = ZXP(in);
						ZXP(out) = 0.f;
					);
				} else {
					LOOP(nsmps,
						float value = ZXP(dlyrd);
						ZXP(dlywr) = value * feedbk + ZXP(in);
						ZXP(out) = value;
						feedbk += feedbk_slope;
					);
				}
				iwrphase += nsmps;
				irdphase += nsmps;
			}
			unit->m_feedbk = feedbk;
			unit->m_decaytime = decaytime;
		}
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long irdphase = iwrphase - (long)dsamp;

			if (irdphase < 0) {
				dlybuf[iwrphase & mask] = ZXP(in);
				ZXP(out) = 0.f;
			} else {
				float value = dlybuf[irdphase & mask];
				dlybuf[iwrphase & mask] = ZXP(in) + feedbk * value;
				ZXP(out) = value;
			}
			feedbk += feedbk_slope;
			iwrphase++;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= unit->m_idelaylen) {
		SETCALC(CombN_next);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

void CombL_Ctor(CombL *unit)
{
	FeedbackDelay_Reset(unit);
	SETCALC(CombL_next_z);
	ZOUT0(0) = 0.f;
}

void CombL_next(CombL *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	long mask = unit->m_mask;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;
			float d1 = dlybuf[irdphase & mask];
			float d2 = dlybuf[irdphaseb & mask];
			float value = lininterp(frac, d1, d2);
			dlybuf[iwrphase & mask] = ZXP(in) + feedbk * value;
			ZXP(out) = value;
			iwrphase++;
		);
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;
			float d1 = dlybuf[irdphase & mask];
			float d2 = dlybuf[irdphaseb & mask];
			float value = lininterp(frac, d1, d2);
			dlybuf[iwrphase & mask] = ZXP(in) + feedbk * value;
			ZXP(out) = value;
			feedbk += feedbk_slope;
			iwrphase++;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

}


void CombL_next_z(CombL *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	long mask = unit->m_mask;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;

			float zin = ZXP(in);
			if (irdphase < 0) {
				dlybuf[iwrphase & mask] = zin;
				ZXP(out) = 0.f;
			} else if (irdphaseb < 0) {
				float d1 = dlybuf[irdphase & mask];
				float value = d1 - frac * d1;
				dlybuf[iwrphase & mask] = zin + feedbk * value;
				//postbuf("A %d d1 %g fr %g v %g in %g fb %g\n", irdphase, d1, frac, value, zin, feedbk);
				ZXP(out) = value;
			} else {
				float d1 = dlybuf[irdphase & mask];
				float d2 = dlybuf[irdphaseb & mask];
				float value = lininterp(frac, d1, d2);
				dlybuf[iwrphase & mask] = zin + feedbk * value;
				//postbuf("B %d d1 %g d2 %g fr %g v %g in %g fb %g\n", irdphase, d1, d2, frac, value, zin, feedbk);
				ZXP(out) = value;
			}
			iwrphase++;
		);
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;

			float zin = ZXP(in);
			if (irdphase < 0) {
				ZXP(out) = 0.f;
				dlybuf[iwrphase & mask] = zin;
			} else if (irdphaseb < 0) {
				float d1 = dlybuf[irdphase & mask];
				float value = d1 - frac * d1;
				dlybuf[iwrphase & mask] = zin + feedbk * value;
				ZXP(out) = value;
			} else {
				float d1 = dlybuf[irdphase & mask];
				float d2 = dlybuf[irdphaseb & mask];
				float value = lininterp(frac, d1, d2);
				dlybuf[iwrphase & mask] = zin + feedbk * value;
				ZXP(out) = value;
			}
			feedbk += feedbk_slope;
			iwrphase++;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= unit->m_idelaylen) {
		SETCALC(CombL_next);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void CombC_Ctor(CombC *unit)
{
	FeedbackDelay_Reset(unit);
	SETCALC(CombC_next_z);
	ZOUT0(0) = 0.f;
}

void CombC_next(CombC *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	long mask = unit->m_mask;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			float d0 = dlybuf[irdphase0 & mask];
			float d1 = dlybuf[irdphase1 & mask];
			float d2 = dlybuf[irdphase2 & mask];
			float d3 = dlybuf[irdphase3 & mask];
			float value = cubicinterp(frac, d0, d1, d2, d3);
			dlybuf[iwrphase & mask] = ZXP(in) + feedbk * value;
			ZXP(out) = value;
			iwrphase++;
		);
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			float d0 = dlybuf[irdphase0 & mask];
			float d1 = dlybuf[irdphase1 & mask];
			float d2 = dlybuf[irdphase2 & mask];
			float d3 = dlybuf[irdphase3 & mask];
			float value = cubicinterp(frac, d0, d1, d2, d3);
			dlybuf[iwrphase & mask] = ZXP(in) + feedbk * value;
			ZXP(out) = value;
			feedbk += feedbk_slope;
			iwrphase++;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

}


void CombC_next_z(CombC *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	long mask = unit->m_mask;
	float d0, d1, d2, d3;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;

			if (irdphase0 < 0) {
				dlybuf[iwrphase & mask] = ZXP(in);
				ZXP(out) = 0.f;
			} else {
				if (irdphase1 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
				} else if (irdphase2 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
				} else if (irdphase3 < 0) {
					d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
				} else {
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
					d3 = dlybuf[irdphase3 & mask];
				}
				float value = cubicinterp(frac, d0, d1, d2, d3);
				dlybuf[iwrphase & mask] = ZXP(in) + feedbk * value;
				ZXP(out) = value;
			}
			iwrphase++;
		);
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;

			if (irdphase0 < 0) {
				dlybuf[iwrphase & mask] = ZXP(in);
				ZXP(out) = 0.f;
			} else {
				if (irdphase1 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
				} else if (irdphase2 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
				} else if (irdphase3 < 0) {
					d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
				} else {
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
					d3 = dlybuf[irdphase3 & mask];
				}
				float value = cubicinterp(frac, d0, d1, d2, d3);
				dlybuf[iwrphase & mask] = ZXP(in) + feedbk * value;
				ZXP(out) = value;
			}
			feedbk += feedbk_slope;
			iwrphase++;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= unit->m_idelaylen) {
		SETCALC(CombC_next);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

void AllpassN_Ctor(AllpassN *unit)
{
	SETCALC(AllpassN_next_z);
	FeedbackDelay_Reset(unit);
	ZOUT0(0) = 0.f;
}

void AllpassN_next(AllpassN *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	long mask = unit->m_mask;

	//postbuf("AllpassN_next %g %g %g %g %d %d %d\n", delaytime, decaytime, feedbk, dsamp, mask, iwrphase, zorg);
	if (delaytime == unit->m_delaytime) {
		long irdphase = iwrphase - (long)dsamp;
		float* dlybuf1 = dlybuf - ZOFF;
		float* dlyrd   = dlybuf1 + (irdphase & mask);
		float* dlywr   = dlybuf1 + (iwrphase & mask);
		float* dlyN    = dlybuf1 + unit->m_idelaylen;
		if (decaytime == unit->m_decaytime) {
			long remain = inNumSamples;
			while (remain) {
				long rdspace = dlyN - dlyrd;
				long wrspace = dlyN - dlywr;
				long nsmps = sc_min(rdspace, wrspace);
				nsmps = sc_min(remain, nsmps);
				remain -= nsmps;
				LOOP(nsmps,
					float value = ZXP(dlyrd);
					float dwr = value * feedbk + ZXP(in);
					ZXP(dlywr) = dwr;
					ZXP(out) = value - feedbk * dwr;
				);
				if (dlyrd == dlyN) dlyrd = dlybuf1;
				if (dlywr == dlyN) dlywr = dlybuf1;
			}
		} else {
			float next_feedbk = CalcFeedback(delaytime, decaytime);
			float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);
			long remain = inNumSamples;
			while (remain) {
				long rdspace = dlyN - dlyrd;
				long wrspace = dlyN - dlywr;
				long nsmps = sc_min(rdspace, wrspace);
				nsmps = sc_min(remain, nsmps);
				remain -= nsmps;

				LOOP(nsmps,
					float value = ZXP(dlyrd);
					float dwr = value * feedbk + ZXP(in);
					ZXP(dlywr) = dwr;
					ZXP(out) = value - feedbk * dwr;
					feedbk += feedbk_slope;
				);
				if (dlyrd == dlyN) dlyrd = dlybuf1;
				if (dlywr == dlyN) dlywr = dlybuf1;
			}
			unit->m_feedbk = feedbk;
			unit->m_decaytime = decaytime;
		}
		iwrphase += inNumSamples;
	} else {
		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);
		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			++iwrphase;
			long irdphase = iwrphase - (long)dsamp;
			float value = dlybuf[irdphase & mask];
			float dwr = value * feedbk + ZXP(in);
			dlybuf[iwrphase & mask] = dwr;
			ZXP(out) = value - feedbk * dwr;
			feedbk += feedbk_slope;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

}


void AllpassN_next_z(AllpassN *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	long mask = unit->m_mask;

	//postbuf("AllpassN_next_z %g %g %g %g %d %d %d\n", delaytime, decaytime, feedbk, dsamp, mask, iwrphase, zorg);
	if (delaytime == unit->m_delaytime) {
		long irdphase = iwrphase - (long)dsamp;
		float* dlybuf1 = dlybuf - ZOFF;
		float* dlyN    = dlybuf1 + unit->m_idelaylen;
		if (decaytime == unit->m_decaytime) {
			long remain = inNumSamples;
			while (remain) {
				float* dlywr = dlybuf1 + (iwrphase & mask);
				float* dlyrd = dlybuf1 + (irdphase & mask);
				long rdspace = dlyN - dlyrd;
				long wrspace = dlyN - dlywr;
				long nsmps = sc_min(rdspace, wrspace);
				nsmps = sc_min(remain, nsmps);
				remain -= nsmps;
				if (irdphase < 0) {
					feedbk = -feedbk;
					LOOP(nsmps,
						float dwr = ZXP(in);
						ZXP(dlywr) = dwr;
						ZXP(out) = feedbk * dwr;
					);
					feedbk = -feedbk;
				} else {
					LOOP(nsmps,
						float x1 = ZXP(dlyrd);
						float dwr = x1 * feedbk + ZXP(in);
						ZXP(dlywr) = dwr;
						ZXP(out) = x1 - feedbk * dwr;
					);
				}
				iwrphase += nsmps;
				irdphase += nsmps;
			}
		} else {
			float next_feedbk = CalcFeedback(delaytime, decaytime);
			float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);
			long remain = inNumSamples;
			while (remain) {
				float* dlyrd = dlybuf1 + (irdphase & mask);
				float* dlywr = dlybuf1 + (iwrphase & mask);
				long rdspace = dlyN - dlyrd;
				long wrspace = dlyN - dlywr;
				long nsmps = sc_min(rdspace, wrspace);
				nsmps = sc_min(remain, nsmps);
				remain -= nsmps;

				if (irdphase < 0) {
					dlyrd += nsmps;
					LOOP(nsmps,
						float dwr = ZXP(in);
						ZXP(dlywr) = dwr;
						ZXP(out) = -feedbk * dwr;
						feedbk += feedbk_slope;
					);
				} else {
					LOOP(nsmps,
						float x1 = ZXP(dlyrd);
						float dwr = x1 * feedbk + ZXP(in);
						ZXP(dlywr) = dwr;
						ZXP(out) = x1 - feedbk * dwr;
						feedbk += feedbk_slope;
					);
				}
				iwrphase += nsmps;
				irdphase += nsmps;
			}
			unit->m_feedbk = feedbk;
			unit->m_decaytime = decaytime;
		}
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long irdphase = iwrphase - (long)dsamp;

			if (irdphase < 0) {
				float dwr = ZXP(in);
				dlybuf[iwrphase & mask] = dwr;
				ZXP(out) = -feedbk * dwr;
			} else {
				float value = dlybuf[irdphase & mask];
				float dwr = feedbk * value + ZXP(in);
				dlybuf[iwrphase & mask] = dwr;
				ZXP(out) = value - feedbk * dwr;
			}
			feedbk += feedbk_slope;
			iwrphase++;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= unit->m_idelaylen) {
		SETCALC(AllpassN_next);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

void AllpassL_Ctor(AllpassL *unit)
{
	FeedbackDelay_Reset(unit);
	SETCALC(AllpassL_next_z);
	ZOUT0(0) = 0.f;
}

void AllpassL_next(AllpassL *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	long mask = unit->m_mask;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;
			float d1 = dlybuf[irdphase & mask];
			float d2 = dlybuf[irdphaseb & mask];
			float value = lininterp(frac, d1, d2);
			float dwr = ZXP(in) + feedbk * value;
			dlybuf[iwrphase & mask] = dwr;
			ZXP(out) = value - feedbk * dwr;


			iwrphase++;
		);
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;
			float d1 = dlybuf[irdphase & mask];
			float d2 = dlybuf[irdphaseb & mask];
			float value = lininterp(frac, d1, d2);
			float dwr = ZXP(in) + feedbk * value;
			dlybuf[iwrphase & mask] = dwr;
			ZXP(out) = value - feedbk * dwr;
			feedbk += feedbk_slope;
			iwrphase++;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

}


void AllpassL_next_z(AllpassL *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	long mask = unit->m_mask;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;

			float zin = ZXP(in);
			if (irdphase < 0) {
				dlybuf[iwrphase & mask] = zin;
				ZXP(out) = - feedbk * zin;
			} else if (irdphaseb < 0) {
				float d1 = dlybuf[irdphase & mask];
				float value = d1 - frac * d1;
				float dwr = zin + feedbk * value;
				dlybuf[iwrphase & mask] = dwr;
				ZXP(out) = value - feedbk * dwr;
			} else {
				float d1 = dlybuf[irdphase & mask];
				float d2 = dlybuf[irdphaseb & mask];
				float value = lininterp(frac, d1, d2);
				float dwr = zin + feedbk * value;
				dlybuf[iwrphase & mask] = dwr;
				ZXP(out) = value - feedbk * dwr;
			}
			iwrphase++;
		);
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase = iwrphase - idsamp;
			long irdphaseb = irdphase - 1;

			float zin = ZXP(in);
			if (irdphase < 0) {
				dlybuf[iwrphase & mask] = zin;
				ZXP(out) = - feedbk * zin;
			} else if (irdphaseb < 0) {
				float d1 = dlybuf[irdphase & mask];
				float value = d1 - frac * d1;
				float dwr = zin + feedbk * value;
				dlybuf[iwrphase & mask] = dwr;
				ZXP(out) = value - feedbk * dwr;
			} else {
				float d1 = dlybuf[irdphase & mask];
				float d2 = dlybuf[irdphaseb & mask];
				float value = lininterp(frac, d1, d2);
				float dwr = zin + feedbk * value;
				dlybuf[iwrphase & mask] = dwr;
				ZXP(out) = value - feedbk * dwr;
			}
			feedbk += feedbk_slope;
			iwrphase++;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= unit->m_idelaylen) {
		SETCALC(AllpassL_next);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

void AllpassC_Ctor(AllpassC *unit)
{
	FeedbackDelay_Reset(unit);
	SETCALC(AllpassC_next_z);
	ZOUT0(0) = 0.f;
}

void AllpassC_next(AllpassC *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	long mask = unit->m_mask;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			float d0 = dlybuf[irdphase0 & mask];
			float d1 = dlybuf[irdphase1 & mask];
			float d2 = dlybuf[irdphase2 & mask];
			float d3 = dlybuf[irdphase3 & mask];
			float value = cubicinterp(frac, d0, d1, d2, d3);
			float dwr = ZXP(in) + feedbk * value;
			dlybuf[iwrphase & mask] = dwr;
			ZXP(out) = value - feedbk * dwr;
			iwrphase++;
		);
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			float d0 = dlybuf[irdphase0 & mask];
			float d1 = dlybuf[irdphase1 & mask];
			float d2 = dlybuf[irdphase2 & mask];
			float d3 = dlybuf[irdphase3 & mask];
			float value = cubicinterp(frac, d0, d1, d2, d3);
			float dwr = ZXP(in) + feedbk * value;
			dlybuf[iwrphase & mask] = dwr;
			ZXP(out) = value - feedbk * dwr;
			feedbk += feedbk_slope;
			iwrphase++;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

}


void AllpassC_next_z(AllpassC *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	long mask = unit->m_mask;
	float d0, d1, d2, d3;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP(inNumSamples,
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;

			if (irdphase0 < 0) {
				dlybuf[iwrphase & mask] = ZXP(in);
				ZXP(out) = 0.f;
			} else {
				if (irdphase1 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
				} else if (irdphase2 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
				} else if (irdphase3 < 0) {
					d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
				} else {
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
					d3 = dlybuf[irdphase3 & mask];
				}
				float value = cubicinterp(frac, d0, d1, d2, d3);
				float dwr = ZXP(in) + feedbk * value;
				dlybuf[iwrphase & mask] = dwr;
				ZXP(out) = value - feedbk * dwr;
			}
			iwrphase++;
		);
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		LOOP(inNumSamples,
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;

			if (irdphase0 < 0) {
				dlybuf[iwrphase & mask] = ZXP(in);
				ZXP(out) = 0.f;
			} else {
				if (irdphase1 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
				} else if (irdphase2 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
				} else if (irdphase3 < 0) {
					d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
				} else {
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
					d3 = dlybuf[irdphase3 & mask];
				}
				float value = cubicinterp(frac, d0, d1, d2, d3);
				float dwr = ZXP(in) + feedbk * value;
				dlybuf[iwrphase & mask] = dwr;
				ZXP(out) = value - feedbk * dwr;
			}
			feedbk += feedbk_slope;
			iwrphase++;
		);
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_iwrphase = iwrphase;

	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= unit->m_idelaylen) {
		SETCALC(AllpassC_next);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////


inline double sc_loop1(int32 in, int32 lo, int32 hi)
{
	// avoid the divide if possible
	if (in >= hi) {
		in -= hi;
		if (in < hi) return in;
	} else if (in < lo) {
		in += hi;
		if (in >= lo) return in;
	} else return in;

	int32 range = hi - lo;
	return lo + range * (in-lo) / range;
}

#if NOTYET

void SimpleLoopBuf_next_kk(SimpleLoopBuf *unit, int inNumSamples)
{
	float trig     = ZIN0(1);
	double loopstart  = (double)ZIN0(2);
	double loopend    = (double)ZIN0(3);
	GET_BUF
	CHECK_BUF
	SETUP_OUT

	loopend = sc_max(loopend, bufFrames);
	int32 phase = unit->m_phase;
	if (trig > 0.f && unit->m_prevtrig <= 0.f) {
		unit->mDone = false;
		phase = (int32)ZIN0(2);
	}
	unit->m_prevtrig = trig;
	for (int i=0; i<inNumSamples; ++i) {

		phase = sc_loop1(phase, loopstart, loopend);
		int32 iphase = (int32)phase;
		float* table1 = bufData + iphase * bufChannels;
		int32 index = 0;
		for (uint32 i=0; i<bufChannels; ++i) {
			*++(out[i]) = table1[index++];
		}

		phase++;
	}
	unit->m_phase = phase;
}


void SimpleLoopBuf_Ctor(SimpleLoopBuf *unit)
{
	SETCALC(SimpleLoopBuf_next_kk);

	unit->m_fbufnum = -1e9f;
	unit->m_prevtrig = 0.;
	unit->mOut = 0;
	unit->m_phase = ZIN0(2);

	ClearUnitOutputs(unit, 1);
}

void SimpleLoopBuf_Dtor(SimpleLoopBuf *unit)
{
	TAKEDOWN_OUT
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////

#define GET_SCOPEBUF \
	float fbufnum  = ZIN0(0); \
	if (fbufnum != unit->m_fbufnum) { \
		World *world = unit->mWorld; \
		if (!world->mNumSndBufs) { \
			ClearUnitOutputs(unit, inNumSamples); \
			return; \
		} \
		uint32 bufnum = (int)fbufnum; \
		if (bufnum >= world->mNumSndBufs) bufnum = 0; \
		unit->m_fbufnum = fbufnum; \
		unit->m_buf = world->mSndBufs + bufnum; \
		unit->m_bufupdates = world->mSndBufUpdates + bufnum; \
	} \
	SndBuf *buf = unit->m_buf; \
	SndBufUpdates *bufupdates = unit->m_bufupdates; \
	float *bufData __attribute__((__unused__)) = buf->data; \
	uint32 bufChannels __attribute__((__unused__)) = buf->channels; \
	uint32 bufFrames __attribute__((__unused__)) = buf->frames; \

void ScopeOut_next(ScopeOut *unit, int inNumSamples)
{
	GET_SCOPEBUF

	if (!bufData) {
		unit->m_framepos = 0;
		return;
	}

	SETUP_IN(1)

	uint32 framepos = unit->m_framepos;
	if (framepos >= bufFrames) {
		unit->m_framepos = 0;
	}

	if (bufupdates->reads != bufupdates->writes) {
		unit->m_framepos += inNumSamples;
		return;
	}

	bufData += framepos * bufChannels;

	int remain = (bufFrames - framepos), wrap = 0;

	if(inNumSamples <= remain) {
		remain = inNumSamples;
		wrap = 0;
	}
	else
		wrap = inNumSamples - remain;

	if (bufChannels > 2) {
		for (int j=0; j<remain; ++j) {
			for (uint32 i=0; i<bufChannels; ++i) {
				*bufData++ = *++(in[i]);
			}
		}

		bufData = buf->data;

		for (int j=0; j<wrap; ++j) {
			for (uint32 i=0; i<bufChannels; ++i) {
				*bufData++ = *++(in[i]);
			}
		}
	} else if (bufChannels == 2) {
		float *in0 = in[0];
		float *in1 = in[1];
		for (int j=0; j<remain; ++j) {
			*bufData++ = *++in0;
			*bufData++ = *++in1;
		}

		bufData = buf->data;

		for (int j=0; j<wrap; ++j) {
			*bufData++ = *++in0;
			*bufData++ = *++in1;
		}
	} else {
		float *in0 = in[0];
		for (int j=0; j<remain; ++j) {
			*bufData++ = *++in0;
		}

		bufData = buf->data;

		for (int j=0; j<wrap; ++j) {
			*bufData++ = *++in0;
		}
	}

	unit->m_framepos += inNumSamples;
	unit->m_framecount += inNumSamples;

	if (unit->m_framecount >= bufFrames) {
		bufupdates->writes++;
		unit->m_framecount = 0;
	}
}



void ScopeOut_Ctor(ScopeOut *unit)
{
	unit->m_fbufnum = -1e9;
	unit->m_framepos = 0;
	unit->m_framecount = 0;
	unit->mIn = 0;
	SETCALC(ScopeOut_next);
}

void ScopeOut_Dtor(ScopeOut *unit)
{
	TAKEDOWN_IN
}


////////////////////////////////////////////////////////////////////////////////////////////////////////

struct PitchShift : public Unit
{
	SndBuf *m_buf;
	float *dlybuf;
	float dsamp1, dsamp1_slope, ramp1, ramp1_slope;
	float dsamp2, dsamp2_slope, ramp2, ramp2_slope;
	float dsamp3, dsamp3_slope, ramp3, ramp3_slope;
	float dsamp4, dsamp4_slope, ramp4, ramp4_slope;
	float fdelaylen, slope;
	long iwrphase, idelaylen, mask;
	long counter, stage, numoutput, framesize;
};

void PitchShift_next(PitchShift *unit, int inNumSamples);
void PitchShift_next(PitchShift *unit, int inNumSamples)
{
	float *out, *in, *dlybuf;
	float disppchratio, pchratio, pchratio1, value;
	float dsamp1, dsamp1_slope, ramp1, ramp1_slope;
	float dsamp2, dsamp2_slope, ramp2, ramp2_slope;
	float dsamp3, dsamp3_slope, ramp3, ramp3_slope;
	float dsamp4, dsamp4_slope, ramp4, ramp4_slope;
	float fdelaylen, d1, d2, frac, slope, samp_slope, startpos, winsize, pchdisp, timedisp;
	long remain, nsmps, idelaylen, irdphase, irdphaseb, iwrphase, mask, idsamp;
	long counter, stage, framesize;

	RGET

	out = ZOUT(0);
	in = ZIN(0);

	pchratio = ZIN0(2);
	winsize = ZIN0(1);
	pchdisp = ZIN0(3);
	timedisp = ZIN0(4);
	timedisp = sc_clip(timedisp, 0.f, winsize) * SAMPLERATE;

	dlybuf = unit->dlybuf;
	fdelaylen = unit->fdelaylen;
	idelaylen = unit->idelaylen;
	iwrphase = unit->iwrphase;

	counter = unit->counter;
	stage = unit->stage;
	mask = unit->mask;
	framesize = unit->framesize;

	dsamp1 = unit->dsamp1;
	dsamp2 = unit->dsamp2;
	dsamp3 = unit->dsamp3;
	dsamp4 = unit->dsamp4;

	dsamp1_slope = unit->dsamp1_slope;
	dsamp2_slope = unit->dsamp2_slope;
	dsamp3_slope = unit->dsamp3_slope;
	dsamp4_slope = unit->dsamp4_slope;

	ramp1 = unit->ramp1;
	ramp2 = unit->ramp2;
	ramp3 = unit->ramp3;
	ramp4 = unit->ramp4;

	ramp1_slope = unit->ramp1_slope;
	ramp2_slope = unit->ramp2_slope;
	ramp3_slope = unit->ramp3_slope;
	ramp4_slope = unit->ramp4_slope;

	slope = unit->slope;

	remain = inNumSamples;
	while (remain) {
		if (counter <= 0) {
			counter = framesize >> 2;
			unit->stage = stage = (stage + 1) & 3;
			disppchratio = pchratio;
			if (pchdisp != 0.f) {
				disppchratio += (pchdisp * frand2(s1,s2,s3));
			}
			disppchratio = sc_clip(disppchratio, 0.f, 4.f);
			pchratio1 = disppchratio - 1.f;
			samp_slope = -pchratio1;
			startpos = pchratio1 < 0.f ? 2.f : framesize * pchratio1 + 2.f;
			startpos += (timedisp * frand(s1,s2,s3));
			switch(stage) {
				case 0 :
					unit->dsamp1_slope = dsamp1_slope = samp_slope;
					dsamp1 = startpos;
					ramp1 = 0.0;
					unit->ramp1_slope = ramp1_slope =  slope;
					unit->ramp3_slope = ramp3_slope = -slope;
					break;
				case 1 :
					unit->dsamp2_slope = dsamp2_slope = samp_slope;
					dsamp2 = startpos;
					ramp2 = 0.0;
					unit->ramp2_slope = ramp2_slope =  slope;
					unit->ramp4_slope = ramp4_slope = -slope;
					break;
				case 2 :
					unit->dsamp3_slope = dsamp3_slope = samp_slope;
					dsamp3 = startpos;
					ramp3 = 0.0;
					unit->ramp3_slope = ramp3_slope =  slope;
					unit->ramp1_slope = ramp1_slope = -slope;
					break;
				case 3 :
					unit->dsamp4_slope = dsamp4_slope = samp_slope;
					dsamp4 = startpos;
					ramp4 = 0.0;
					unit->ramp2_slope = ramp2_slope = -slope;
					unit->ramp4_slope = ramp4_slope =  slope;
					break;
			}
		/*Print("%d %d    %g %g %g %g    %g %g %g %g    %g %g %g %g\n",
			counter, stage, dsamp1_slope, dsamp2_slope, dsamp3_slope, dsamp4_slope,
				dsamp1, dsamp2, dsamp3, dsamp4,
				ramp1, ramp2, ramp3, ramp4);*/

		}

		nsmps = sc_min(remain, counter);
		remain -= nsmps;
		counter -= nsmps;

		LOOP(nsmps,
			iwrphase = (iwrphase + 1) & mask;

			dsamp1 += dsamp1_slope;
			idsamp = (long)dsamp1;
			frac = dsamp1 - idsamp;
			irdphase = (iwrphase - idsamp) & mask;
			irdphaseb = (irdphase - 1) & mask;
			d1 = dlybuf[irdphase];
			d2 = dlybuf[irdphaseb];
			value = (d1 + frac * (d2 - d1)) * ramp1;
			ramp1 += ramp1_slope;

			dsamp2 += dsamp2_slope;
			idsamp = (long)dsamp2;
			frac = dsamp2 - idsamp;
			irdphase = (iwrphase - idsamp) & mask;
			irdphaseb = (irdphase - 1) & mask;
			d1 = dlybuf[irdphase];
			d2 = dlybuf[irdphaseb];
			value += (d1 + frac * (d2 - d1)) * ramp2;
			ramp2 += ramp2_slope;

			dsamp3 += dsamp3_slope;
			idsamp = (long)dsamp3;
			frac = dsamp3 - idsamp;
			irdphase = (iwrphase - idsamp) & mask;
			irdphaseb = (irdphase - 1) & mask;
			d1 = dlybuf[irdphase];
			d2 = dlybuf[irdphaseb];
			value += (d1 + frac * (d2 - d1)) * ramp3;
			ramp3 += ramp3_slope;

			dsamp4 += dsamp4_slope;
			idsamp = (long)dsamp4;
			frac = dsamp4 - idsamp;
			irdphase = (iwrphase - idsamp) & mask;
			irdphaseb = (irdphase - 1) & mask;
			d1 = dlybuf[irdphase];
			d2 = dlybuf[irdphaseb];
			value += (d1 + frac * (d2 - d1)) * ramp4;
			ramp4 += ramp4_slope;

			dlybuf[iwrphase] = ZXP(in);
			ZXP(out) = value *= 0.5;
		);
	}

	unit->counter = counter;

	unit->dsamp1 = dsamp1;
	unit->dsamp2 = dsamp2;
	unit->dsamp3 = dsamp3;
	unit->dsamp4 = dsamp4;

	unit->ramp1 = ramp1;
	unit->ramp2 = ramp2;
	unit->ramp3 = ramp3;
	unit->ramp4 = ramp4;

	unit->iwrphase = iwrphase;

	RPUT
}




void PitchShift_next_z(PitchShift *unit, int inNumSamples);
void PitchShift_next_z(PitchShift *unit, int inNumSamples)
{
	float *out, *in, *dlybuf;
	float disppchratio, pchratio, pchratio1, value;
	float dsamp1, dsamp1_slope, ramp1, ramp1_slope;
	float dsamp2, dsamp2_slope, ramp2, ramp2_slope;
	float dsamp3, dsamp3_slope, ramp3, ramp3_slope;
	float dsamp4, dsamp4_slope, ramp4, ramp4_slope;
	float fdelaylen, d1, d2, frac, slope, samp_slope, startpos, winsize, pchdisp, timedisp;
	long remain, nsmps, idelaylen, irdphase, irdphaseb, iwrphase;
	long mask, idsamp;
	long counter, stage, framesize, numoutput;

	RGET

	out = ZOUT(0);
	in = ZIN(0);
	pchratio = ZIN0(2);
	winsize = ZIN0(1);
	pchdisp = ZIN0(3);
	timedisp = ZIN0(4);
	timedisp = sc_clip(timedisp, 0.f, winsize) * SAMPLERATE;

	dlybuf = unit->dlybuf;
	fdelaylen = unit->fdelaylen;
	idelaylen = unit->idelaylen;
	iwrphase = unit->iwrphase;
	numoutput = unit->numoutput;

	counter = unit->counter;
	stage = unit->stage;
	mask = unit->mask;
	framesize = unit->framesize;

	dsamp1 = unit->dsamp1;
	dsamp2 = unit->dsamp2;
	dsamp3 = unit->dsamp3;
	dsamp4 = unit->dsamp4;

	dsamp1_slope = unit->dsamp1_slope;
	dsamp2_slope = unit->dsamp2_slope;
	dsamp3_slope = unit->dsamp3_slope;
	dsamp4_slope = unit->dsamp4_slope;

	ramp1 = unit->ramp1;
	ramp2 = unit->ramp2;
	ramp3 = unit->ramp3;
	ramp4 = unit->ramp4;

	ramp1_slope = unit->ramp1_slope;
	ramp2_slope = unit->ramp2_slope;
	ramp3_slope = unit->ramp3_slope;
	ramp4_slope = unit->ramp4_slope;

	slope = unit->slope;

	remain = inNumSamples;
	while (remain) {
		if (counter <= 0) {
			counter = framesize >> 2;
			unit->stage = stage = (stage + 1) & 3;
			disppchratio = pchratio;
			if (pchdisp != 0.f) {
				disppchratio += (pchdisp * frand2(s1,s2,s3));
			}
			disppchratio = sc_clip(disppchratio, 0.f, 4.f);
			pchratio1 = disppchratio - 1.f;
			samp_slope = -pchratio1;
			startpos = pchratio1 < 0.f ? 2.f : framesize * pchratio1 + 2.f;
			startpos += (timedisp * frand(s1,s2,s3));
			switch(stage) {
				case 0 :
					unit->dsamp1_slope = dsamp1_slope = samp_slope;
					dsamp1 = startpos;
					ramp1 = 0.0;
					unit->ramp1_slope = ramp1_slope =  slope;
					unit->ramp3_slope = ramp3_slope = -slope;
					break;
				case 1 :
					unit->dsamp2_slope = dsamp2_slope = samp_slope;
					dsamp2 = startpos;
					ramp2 = 0.0;
					unit->ramp2_slope = ramp2_slope =  slope;
					unit->ramp4_slope = ramp4_slope = -slope;
					break;
				case 2 :
					unit->dsamp3_slope = dsamp3_slope = samp_slope;
					dsamp3 = startpos;
					ramp3 = 0.0;
					unit->ramp3_slope = ramp3_slope =  slope;
					unit->ramp1_slope = ramp1_slope = -slope;
					break;
				case 3 :
					unit->dsamp4_slope = dsamp4_slope = samp_slope;
					dsamp4 = startpos;
					ramp4 = 0.0;
					unit->ramp2_slope = ramp2_slope = -slope;
					unit->ramp4_slope = ramp4_slope =  slope;
					break;
			}
		/*Print("z %d %d    %g %g %g %g    %g %g %g %g    %g %g %g %g\n",
			counter, stage, dsamp1_slope, dsamp2_slope, dsamp3_slope, dsamp4_slope,
				dsamp1, dsamp2, dsamp3, dsamp4,
				ramp1, ramp2, ramp3, ramp4);*/
		}
		nsmps = sc_min(remain, counter);
		remain -= nsmps;
		counter -= nsmps;

		while (nsmps--) {
			numoutput++;
			iwrphase = (iwrphase + 1) & mask;

			dsamp1 += dsamp1_slope;
			idsamp = (long)dsamp1;
			frac = dsamp1 - idsamp;
			irdphase = (iwrphase - idsamp) & mask;
			irdphaseb = (irdphase - 1) & mask;
			if (numoutput < idelaylen) {
				if (irdphase > iwrphase) {
					value = 0.f;
				} else if (irdphaseb > iwrphase) {
					d1 = dlybuf[irdphase];
					value = (d1 - frac * d1) * ramp1;
				} else {
					d1 = dlybuf[irdphase];
					d2 = dlybuf[irdphaseb];
					value = (d1 + frac * (d2 - d1)) * ramp1;
				}
			} else {
				d1 = dlybuf[irdphase];
				d2 = dlybuf[irdphaseb];
				value = (d1 + frac * (d2 - d1)) * ramp1;
			}
			ramp1 += ramp1_slope;

			dsamp2 += dsamp2_slope;
			idsamp = (long)dsamp2;
			frac = dsamp2 - idsamp;
			irdphase = (iwrphase - idsamp) & mask;
			irdphaseb = (irdphase - 1) & mask;
			if (numoutput < idelaylen) {
				if (irdphase > iwrphase) {
					//value += 0.f;
				} else if (irdphaseb > iwrphase) {
					d1 = dlybuf[irdphase];
					value += (d1 - frac * d1) * ramp2;
				} else {
					d1 = dlybuf[irdphase];
					d2 = dlybuf[irdphaseb];
					value += (d1 + frac * (d2 - d1)) * ramp2;
				}
			} else {
				d1 = dlybuf[irdphase];
				d2 = dlybuf[irdphaseb];
				value += (d1 + frac * (d2 - d1)) * ramp2;
			}
			ramp2 += ramp2_slope;

			dsamp3 += dsamp3_slope;
			idsamp = (long)dsamp3;
			frac = dsamp3 - idsamp;
			irdphase = (iwrphase - idsamp) & mask;
			irdphaseb = (irdphase - 1) & mask;
			if (numoutput < idelaylen) {
				if (irdphase > iwrphase) {
					//value += 0.f;
				} else if (irdphaseb > iwrphase) {
					d1 = dlybuf[irdphase];
					value += (d1 - frac * d1) * ramp3;
				} else {
					d1 = dlybuf[irdphase];
					d2 = dlybuf[irdphaseb];
					value += (d1 + frac * (d2 - d1)) * ramp3;
				}
			} else {
				d1 = dlybuf[irdphase];
				d2 = dlybuf[irdphaseb];
				value += (d1 + frac * (d2 - d1)) * ramp3;
			}
			ramp3 += ramp3_slope;

			dsamp4 += dsamp4_slope;
			idsamp = (long)dsamp4;
			frac = dsamp4 - idsamp;
			irdphase = (iwrphase - idsamp) & mask;
			irdphaseb = (irdphase - 1) & mask;

			if (numoutput < idelaylen) {
				if (irdphase > iwrphase) {
					//value += 0.f;
				} else if (irdphaseb > iwrphase) {
					d1 = dlybuf[irdphase];
					value += (d1 - frac * d1) * ramp4;
				} else {
					d1 = dlybuf[irdphase];
					d2 = dlybuf[irdphaseb];
					value += (d1 + frac * (d2 - d1)) * ramp4;
				}
			} else {
				d1 = dlybuf[irdphase];
				d2 = dlybuf[irdphaseb];
				value += (d1 + frac * (d2 - d1)) * ramp4;
			}
			ramp4 += ramp4_slope;

			dlybuf[iwrphase] = ZXP(in);
			ZXP(out) = value *= 0.5;
		}
	}

	unit->counter = counter;
	unit->stage = stage;
	unit->mask = mask;

	unit->dsamp1 = dsamp1;
	unit->dsamp2 = dsamp2;
	unit->dsamp3 = dsamp3;
	unit->dsamp4 = dsamp4;

	unit->ramp1 = ramp1;
	unit->ramp2 = ramp2;
	unit->ramp3 = ramp3;
	unit->ramp4 = ramp4;

	unit->numoutput = numoutput;
	unit->iwrphase = iwrphase;

	if (numoutput >= idelaylen) {
		SETCALC(PitchShift_next);
	}

	RPUT
}


void PitchShift_Ctor(PitchShift *unit);
void PitchShift_Ctor(PitchShift *unit)
{
	long delaybufsize;
	float *out, *in, *dlybuf;
	float winsize, pchratio;
	float fdelaylen, slope;
	long framesize, last;

	out = ZOUT(0);
	in = ZIN(0);
	pchratio = ZIN0(2);
	winsize = ZIN0(1);

	delaybufsize = (long)ceil(winsize * SAMPLERATE * 3.f + 3.f);
	fdelaylen = delaybufsize - 3;

	delaybufsize = delaybufsize + BUFLENGTH;
	delaybufsize = NEXTPOWEROFTWO(delaybufsize);  // round up to next power of two
	dlybuf = (float*)RTAlloc(unit->mWorld, delaybufsize * sizeof(float));

	SETCALC(PitchShift_next_z);

	*dlybuf = ZIN0(0);
	ZOUT0(0) = 0.f;

	unit->dlybuf = dlybuf;
	unit->idelaylen = delaybufsize;
	unit->fdelaylen = fdelaylen;
	unit->iwrphase = 0;
	unit->numoutput = 0;
	unit->mask = last = (delaybufsize - 1);

	unit->framesize = framesize = ((long)(winsize * SAMPLERATE) + 2) & ~3;
	unit->slope = slope = 2.f / framesize;
	unit->stage = 3;
	unit->counter = framesize >> 2;
	unit->ramp1 = 0.5;
	unit->ramp2 = 1.0;
	unit->ramp3 = 0.5;
	unit->ramp4 = 0.0;

	unit->ramp1_slope = -slope;
	unit->ramp2_slope = -slope;
	unit->ramp3_slope =  slope;
	unit->ramp4_slope =  slope;

	dlybuf[last  ] = 0.f; // put a few zeroes where we start the read heads
	dlybuf[last-1] = 0.f;
	dlybuf[last-2] = 0.f;

	unit->numoutput = 0;

	// start all read heads 2 samples behind the write head
	unit->dsamp1 = unit->dsamp2 = unit->dsamp3 = unit->dsamp4 = 2.f;
	// pch ratio is initially zero for the read heads
	unit->dsamp1_slope = unit->dsamp2_slope = unit->dsamp3_slope = unit->dsamp4_slope = 1.f;
}



void PitchShift_Dtor(PitchShift *unit)
{
	RTFree(unit->mWorld, unit->dlybuf);
}




typedef struct graintap1 {
	float pos, rate, level, slope, curve;
	long counter;
	struct graintap1 *next;
} GrainTap1;

#define MAXDGRAINS 32

struct GrainTap : public Unit
{
	float m_fbufnum;
	SndBuf *m_buf;

	float fdelaylen;
	long bufsize, iwrphase;
	long nextTime;
	GrainTap1 grains[MAXDGRAINS];
	GrainTap1 *firstActive, *firstFree;
};


// coefs: pos, rate, level, slope, curve, counter

void GrainTap_next(GrainTap *unit, int inNumSamples);
void GrainTap_next(GrainTap *unit, int inNumSamples)
{
	float *out, *out0, *dlybuf;
	float sdur, rdur, rdur2;
	float dsamp, dsamp_slope, fdelaylen, d1, d2, frac;
	float level, slope, curve;
	float maxpitch, pitch, maxtimedisp, timedisp, density;
	long remain, nsmps, irdphase, irdphaseb, iwrphase, iwrphase0;
	long idsamp, koffset;
	long counter;
	uint32 bufsize;
	GrainTap1 *grain, *prevGrain, *nextGrain;

	GET_BUF

	RGET

	out0 = ZOUT(0);

	// bufnum, grainDur, pchRatio, pchDisp, timeDisp, overlap
	// 0       1         2         3        4         5

	density = ZIN0(5);
	density = sc_max(0.0001, density);

	bufsize = unit->bufsize;
	if (bufsize != bufSamples) {
		ClearUnitOutputs(unit, inNumSamples);
		return;
	}

	dlybuf = bufData;
	fdelaylen = unit->fdelaylen;
	iwrphase0 = unit->iwrphase;

	// initialize buffer to zero
	out = out0;
	LOOP(inNumSamples, ZXP(out) = 0.f;);

	// do all current grains
	prevGrain = NULL;
	grain = unit->firstActive;
	while (grain) {

		dsamp = grain->pos;
		dsamp_slope = grain->rate;
		level = grain->level;
		slope = grain->slope;
		curve = grain->curve;
		counter = grain->counter;

		nsmps = sc_min(counter, inNumSamples);
		iwrphase = iwrphase0;
		out = out0;
		LOOP(nsmps,
			dsamp += dsamp_slope;
			idsamp = (long)dsamp;
			frac = dsamp - idsamp;
			iwrphase = (iwrphase + 1) & mask;
			irdphase = (iwrphase - idsamp) & mask;
			irdphaseb = (irdphase - 1) & mask;
			d1 = dlybuf[irdphase];
			d2 = dlybuf[irdphaseb];
			ZXP(out) += (d1 + frac * (d2 - d1)) * level;
			level += slope;
			slope += curve;
		);
		grain->pos = dsamp;
		grain->level = level;
		grain->slope = slope;
		grain->counter -= nsmps;

		nextGrain = grain->next;
		if (grain->counter <= 0) {
			// unlink from active list
			if (prevGrain) prevGrain->next = nextGrain;
			else unit->firstActive = nextGrain;

			// link onto free list
			grain->next = unit->firstFree;
			unit->firstFree = grain;
		} else {
			prevGrain = grain;
		}
		grain = nextGrain;
	}
	// start new grains
	remain = inNumSamples;
	while (unit->nextTime <= remain) {
		remain -= unit->nextTime;
		sdur = ZIN0(1) * SAMPLERATE;
		sdur = sc_max(sdur, 4.f);

		grain = unit->firstFree;
		if (grain) {
			unit->firstFree = grain->next;
			grain->next = unit->firstActive;
			unit->firstActive = grain;

			koffset = inNumSamples - remain;
			iwrphase = (iwrphase0 + koffset) & mask;

			grain->counter = (long)sdur;

			timedisp = ZIN0(4);
			timedisp = sc_max(timedisp, 0.f);
			timedisp = frand(s1,s2,s3) * timedisp * SAMPLERATE;

			pitch = ZIN0(2) + frand2(s1,s2,s3) * ZIN0(3);
			if (pitch >= 1.f) {
				maxpitch = 1.f + (fdelaylen/sdur);
				pitch = sc_min(pitch, maxpitch);

				dsamp_slope = 1.f - pitch;
				grain->rate = dsamp_slope;

				maxtimedisp = fdelaylen + sdur * dsamp_slope;
				timedisp = sc_min(timedisp, maxtimedisp);

				dsamp = BUFLENGTH + koffset + 2.f + timedisp - sdur * dsamp_slope;
				dsamp = sc_min(dsamp, fdelaylen);
			} else {
				maxpitch = -(1.f + (fdelaylen/sdur));
				pitch = sc_max(pitch, maxpitch);

				dsamp_slope = 1.f - pitch;
				grain->rate = dsamp_slope;

				maxtimedisp = fdelaylen - sdur * dsamp_slope;
				timedisp = sc_min(timedisp, maxtimedisp);

				dsamp = BUFLENGTH + koffset + 2.f + timedisp;
				dsamp = sc_min(dsamp, fdelaylen);
			}

			grain->pos = dsamp;
			//postbuf("ds %g %g %g\n", dsamp_slope, dsamp, fdelaylen);

			rdur = 1.f / sdur;
			rdur2 = rdur * rdur;
			grain->level = level = 0.f;
			grain->slope = slope = 4.0 * (rdur - rdur2);	// ampslope
			grain->curve = curve = -8.0 * rdur2;			// ampcurve

			nsmps = remain;
			out = out0 + koffset;
			LOOP(nsmps,
				dsamp += dsamp_slope;
				idsamp = (long)dsamp;
				frac = dsamp - idsamp;
				iwrphase = (iwrphase + 1) & mask;
				irdphase = (iwrphase - idsamp) & mask;
				irdphaseb = (irdphase - 1) & mask;
				d1 = dlybuf[irdphase];
				d2 = dlybuf[irdphaseb];
				ZXP(out) += (d1 + frac * (d2 - d1)) * level;
				level += slope;
				slope += curve;
			);
			grain->pos = dsamp;
			grain->level = level;
			grain->slope = slope;
			grain->counter -= nsmps;

			if (grain->counter <= 0) {
				// unlink from active list
				unit->firstActive = grain->next;

				// link onto free list
				grain->next = unit->firstFree;
				unit->firstFree = grain;
			}
		}
		unit->nextTime = (long)(sdur / density);
		if (unit->nextTime < 1) unit->nextTime = 1;

		/*if (grain == NULL) {
			postbuf("nextTime %d %g %g %08X %08X %08X\n", unit->nextTime, sdur, density,
				grain, unit->firstActive, unit->firstFree);
		}*/
	}
	iwrphase = (iwrphase0 + BUFLENGTH) & mask;
	unit->nextTime -= remain;
	if (unit->nextTime < 0) unit->nextTime = 0;

	unit->iwrphase = iwrphase;

	RPUT
}


void GrainTap_Ctor(GrainTap *unit);
void GrainTap_Ctor(GrainTap *unit)
{
	float fdelaylen;
	float maxdelaytime;

	GET_BUF

	if (!ISPOWEROFTWO(bufSamples)) {
		Print("GrainTap buffer size not a power of two.\n");
		SETCALC(*ClearUnitOutputs);
		return;
	}

	fdelaylen = bufSamples - 2 * BUFLENGTH - 3;
	maxdelaytime = fdelaylen * SAMPLEDUR;

	SETCALC(GrainTap_next);

	ZOUT0(0) = 0.f;

	unit->bufsize = bufSamples;
	unit->fdelaylen = fdelaylen;
	unit->iwrphase = 0;
	unit->nextTime = 0;
	for (int i=0; i<MAXDGRAINS-1; ++i) {
		unit->grains[i].next = unit->grains + (i + 1);
	}
	unit->grains[MAXDGRAINS-1].next = NULL;
	unit->firstFree = unit->grains;
	unit->firstActive = NULL;
}




////////////////////////////////////////////////////////////////////////////////////////////////////////


#define GRAIN_BUF \
	SndBuf *buf = bufs + bufnum; \
	float *bufData __attribute__((__unused__)) = buf->data; \
	uint32 bufChannels __attribute__((__unused__)) = buf->channels; \
	uint32 bufSamples __attribute__((__unused__)) = buf->samples; \
	uint32 bufFrames = buf->frames; \
	int guardFrame __attribute__((__unused__)) = bufFrames - 2; \


inline float IN_AT(Unit* unit, int index, int offset)
{
	if (INRATE(index) == calc_FullRate) return IN(index)[offset];
	if (INRATE(index) == calc_DemandRate) return DEMANDINPUT_A(index, offset + 1);
	return ZIN0(index);
}

inline double sc_gloop(double in, double hi)
{
	// avoid the divide if possible
	if (in >= hi) {
		in -= hi;
		if (in < hi) return in;
	} else if (in < 0.) {
		in += hi;
		if (in >= 0.) return in;
	} else return in;

	return in - hi * floor(in/hi);
}

#define GRAIN_LOOP_BODY_4 \
		float amp = y1 * y1; \
		phase = sc_gloop(phase, loopMax); \
		int32 iphase = (int32)phase; \
		float* table1 = bufData + iphase; \
		float* table0 = table1 - 1; \
		float* table2 = table1 + 1; \
		float* table3 = table1 + 2; \
		if (iphase == 0) { \
			table0 += bufSamples; \
		} else if (iphase >= guardFrame) { \
			if (iphase == guardFrame) { \
				table3 -= bufSamples; \
			} else { \
				table2 -= bufSamples; \
				table3 -= bufSamples; \
			} \
		} \
		float fracphase = phase - (double)iphase; \
		float a = table0[0]; \
		float b = table1[0]; \
		float c = table2[0]; \
		float d = table3[0]; \
		float outval = amp * cubicinterp(fracphase, a, b, c, d); \
		ZXP(out1) += outval * pan1; \
		ZXP(out2) += outval * pan2; \
		double y0 = b1 * y1 - y2; \
		y2 = y1; \
		y1 = y0; \


#define GRAIN_LOOP_BODY_2 \
		float amp = y1 * y1; \
		phase = sc_gloop(phase, loopMax); \
		int32 iphase = (int32)phase; \
		float* table1 = bufData + iphase; \
		float* table2 = table1 + 1; \
		if (iphase > guardFrame) { \
			table2 -= bufSamples; \
		} \
		float fracphase = phase - (double)iphase; \
		float b = table1[0]; \
		float c = table2[0]; \
		float outval = amp * (b + fracphase * (c - b)); \
		ZXP(out1) += outval * pan1; \
		ZXP(out2) += outval * pan2; \
		double y0 = b1 * y1 - y2; \
		y2 = y1; \
		y1 = y0; \


#define GRAIN_LOOP_BODY_1 \
		float amp = y1 * y1; \
		phase = sc_gloop(phase, loopMax); \
		int32 iphase = (int32)phase; \
		float outval = amp * bufData[iphase]; \
		ZXP(out1) += outval * pan1; \
		ZXP(out2) += outval * pan2; \
		double y0 = b1 * y1 - y2; \
		y2 = y1; \
		y1 = y0; \


void TGrains_next(TGrains *unit, int inNumSamples)
{
	float *trigin = IN(0);
	float prevtrig = unit->mPrevTrig;

	uint32 numOutputs = unit->mNumOutputs;
	ClearUnitOutputs(unit, inNumSamples);
	float *out[16];
	for (uint32 i=0; i<numOutputs; ++i) out[i] = ZOUT(i);

	World *world = unit->mWorld;
	SndBuf *bufs = world->mSndBufs;
	uint32 numBufs = world->mNumSndBufs;

	for (int i=0; i < unit->mNumActive; ) {
		Grain *grain = unit->mGrains + i;
		uint32 bufnum = grain->bufnum;

		GRAIN_BUF

		if (bufChannels != 1) {
			 ++i;
			 continue;
		}

		double loopMax = (double)bufFrames;

		float pan1 = grain->pan1;
		float pan2 = grain->pan2;
		double rate = grain->rate;
		double phase = grain->phase;
		double b1 = grain->b1;
		double y1 = grain->y1;
		double y2 = grain->y2;

		uint32 chan1 = grain->chan;
		uint32 chan2 = chan1 + 1;
		if (chan2 >= numOutputs) chan2 = 0;

		float *out1 = out[chan1];
		float *out2 = out[chan2];
		//printf("B chan %d %d  %08X %08X", chan1, chan2, out1, out2);

		int nsmps = sc_min(grain->counter, inNumSamples);
		if (grain->interp >= 4) {
			for (int j=0; j<nsmps; ++j) {
				GRAIN_LOOP_BODY_4;
				phase += rate;
			}
		} else if (grain->interp >= 2) {
			for (int j=0; j<nsmps; ++j) {
				GRAIN_LOOP_BODY_2;
				phase += rate;
			}
		} else {
			for (int j=0; j<nsmps; ++j) {
				GRAIN_LOOP_BODY_1;
				phase += rate;
			}
		}

		grain->phase = phase;
		grain->y1 = y1;
		grain->y2 = y2;

		grain->counter -= nsmps;
		if (grain->counter <= 0) {
			// remove grain
			*grain = unit->mGrains[--unit->mNumActive];
		} else ++i;
	}

	int trigSamples = INRATE(0) == calc_FullRate ? inNumSamples : 1;

	for (int i=0; i<trigSamples; ++i) {
		float trig = trigin[i];

		if (trig > 0.f && prevtrig <= 0.f) {
			// start a grain
			if (unit->mNumActive+1 >= kMaxGrains) break;
			uint32 bufnum = (uint32)IN_AT(unit, 1, i);
			if (bufnum >= numBufs) continue;
			GRAIN_BUF

			if (bufChannels != 1) continue;

			float bufSampleRate = buf->samplerate;
			float bufRateScale = bufSampleRate * SAMPLEDUR;
			double loopMax = (double)bufFrames;

			Grain *grain = unit->mGrains + unit->mNumActive++;
			grain->bufnum = bufnum;

			double counter = floor(IN_AT(unit, 4, i) * SAMPLERATE);
			counter = sc_max(4., counter);
			grain->counter = (int)counter;

			double rate = grain->rate = IN_AT(unit, 2, i) * bufRateScale;
			double centerPhase = IN_AT(unit, 3, i) * bufSampleRate;
			double phase = centerPhase - 0.5 * counter * rate;

			float pan = IN_AT(unit, 5, i);
			float amp = IN_AT(unit, 6, i);
			grain->interp = (int)IN_AT(unit, 7, i);

			float panangle;
			if (numOutputs > 2) {
				pan = sc_wrap(pan * 0.5f, 0.f, 1.f);
				float cpan = numOutputs * pan + 0.5;
				float ipan = floor(cpan);
				float panfrac = cpan - ipan;
				panangle = panfrac * pi2;
				grain->chan = (int)ipan;
				if (grain->chan >= (int)numOutputs) grain->chan -= numOutputs;
			} else {
				grain->chan = 0;
				pan = sc_wrap(pan * 0.5f + 0.5f, 0.f, 1.f);
				panangle = pan * pi2;
			}
			float pan1 = grain->pan1 = amp * cos(panangle);
			float pan2 = grain->pan2 = amp * sin(panangle);
			double w = pi / counter;
			double b1 = grain->b1 = 2. * cos(w);
			double y1 = sin(w);
			double y2 = 0.;

			uint32 chan1 = grain->chan;
			uint32 chan2 = chan1 + 1;
			if (chan2 >= numOutputs) chan2 = 0;

			float *out1 = out[chan1] + i;
			float *out2 = out[chan2] + i;

			int nsmps = sc_min(grain->counter, inNumSamples - i);
			if (grain->interp >= 4) {
				for (int j=0; j<nsmps; ++j) {
					GRAIN_LOOP_BODY_4;
					phase += rate;
				}
			} else if (grain->interp >= 2) {
				for (int j=0; j<nsmps; ++j) {
					GRAIN_LOOP_BODY_2;
					phase += rate;
				}
			} else {
				for (int j=0; j<nsmps; ++j) {
					GRAIN_LOOP_BODY_1;
					phase += rate;
				}
			}

			grain->phase = phase;
			grain->y1 = y1;
			grain->y2 = y2;

			grain->counter -= nsmps;
			if (grain->counter <= 0) {
				// remove grain
				*grain = unit->mGrains[--unit->mNumActive];
			}
		}
		prevtrig = trig;
	}

	unit->mPrevTrig = prevtrig;
}

void TGrains_Ctor(TGrains *unit)
{
	SETCALC(TGrains_next);

	unit->mNumActive = 0;
	unit->mPrevTrig = 0.;

	ClearUnitOutputs(unit, 1);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
Pluck - Karplus-Strong
*/
void Pluck_Ctor(Pluck *unit)
{
//	FeedbackDelay_Reset(unit);
	float maxdelaytime = unit->m_maxdelaytime = IN0(2);
	float delaytime = unit->m_delaytime = IN0(3);
	unit->m_dlybuf = 0;
	DelayUnit_AllocDelayLine(unit);
	unit->m_dsamp = CalcDelay(unit, unit->m_delaytime);

	unit->m_numoutput = 0;
	unit->m_iwrphase = 0;
	unit->m_feedbk = CalcFeedback(unit->m_delaytime, unit->m_decaytime);

	if (INRATE(1) == calc_FullRate) {
	    if(INRATE(5) == calc_FullRate){
		SETCALC(Pluck_next_aa_z);
		} else {
		SETCALC(Pluck_next_ak_z); //ak
		}
	    } else {
	    if(INRATE(5) == calc_FullRate){
		SETCALC(Pluck_next_ka_z); //ka
		} else {
		SETCALC(Pluck_next_kk_z); //kk
		}
	    }
	OUT0(0) = unit->m_lastsamp = 0.f;
	unit->m_prevtrig = 0.f;
	unit->m_inputsamps = 0;
	unit->m_coef = IN0(5);
}

void Pluck_next_aa(Pluck *unit, int inNumSamples)
{
	float *out = OUT(0);
	float *in = IN(0);
	float *trig = IN(1);
	float delaytime = IN0(3);
	float decaytime = IN0(4);
	float *coef = IN(5);
	float lastsamp = unit->m_lastsamp;
	unsigned long inputsamps = unit->m_inputsamps;

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	long mask = unit->m_mask;
	float thisin, curtrig;
	float prevtrig = unit->m_prevtrig;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
	for(int i = 0; i < inNumSamples; i++){
			curtrig = trig[i];
			if ((prevtrig <= 0.f) && (curtrig > 0.f)) {
			    inputsamps = (long)(delaytime * unit->mRate->mSampleRate + .5f);
			    }
			prevtrig = curtrig;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			if (inputsamps > 0) {
			    thisin = in[i];
			    --inputsamps;
			    } else {
			    thisin = 0.f;
			    }
			float d0 = dlybuf[irdphase0 & mask];
			float d1 = dlybuf[irdphase1 & mask];
			float d2 = dlybuf[irdphase2 & mask];
			float d3 = dlybuf[irdphase3 & mask];
			float value = cubicinterp(frac, d0, d1, d2, d3);
			float thiscoef = coef[i];
			float onepole = ((1. - fabs(thiscoef)) * value) + (thiscoef * lastsamp);
			dlybuf[iwrphase & mask] = thisin + feedbk * onepole;
			out[i] = lastsamp = onepole;
			iwrphase++;
		};
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

	    for(int i = 0; i < inNumSamples; i++){
			curtrig = trig[i];
			if ((prevtrig <= 0.f) && (curtrig > 0.f)) {
			    inputsamps = (long)(delaytime * unit->mRate->mSampleRate + .5f);
			    }
			prevtrig = curtrig;
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			if (inputsamps > 0) {
			    thisin = in[i];
			    --inputsamps;
			    } else {
			    thisin = 0.f;
			    }
			float d0 = dlybuf[irdphase0 & mask];
			float d1 = dlybuf[irdphase1 & mask];
			float d2 = dlybuf[irdphase2 & mask];
			float d3 = dlybuf[irdphase3 & mask];
			float value = cubicinterp(frac, d0, d1, d2, d3);
			float thiscoef = coef[i];
			float onepole = ((1. - fabs(thiscoef)) * value) + (thiscoef * lastsamp);
			dlybuf[iwrphase & mask] = thisin + feedbk * onepole;
			out[i] = lastsamp = onepole;
			feedbk += feedbk_slope;
			iwrphase++;
		};
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_prevtrig = prevtrig;
	unit->m_inputsamps = inputsamps;
	unit->m_lastsamp = zapgremlins(lastsamp);
	unit->m_iwrphase = iwrphase;

}


void Pluck_next_aa_z(Pluck *unit, int inNumSamples)
{
	float *out = OUT(0);
	float *in = IN(0);
	float *trig = IN(1);
	float delaytime = IN0(3);
	float decaytime = IN0(4);
	float *coef = IN(5);
	float lastsamp = unit->m_lastsamp;

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	long mask = unit->m_mask;
	float d0, d1, d2, d3;
	float thisin, curtrig;
	unsigned long inputsamps = unit->m_inputsamps;
	float prevtrig = unit->m_prevtrig;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		for(int i = 0; i < inNumSamples; i++){
			curtrig = trig[i];
			if ((prevtrig <= 0.f) && (curtrig > 0.f)) {
			    inputsamps = (long)(delaytime * unit->mRate->mSampleRate + .5f);
			    }
			prevtrig = curtrig;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			if (inputsamps > 0) {
			    thisin = in[i];
			    --inputsamps;
			    } else {
			    thisin = 0.f;
			    }
			if (irdphase0 < 0) {
				dlybuf[iwrphase & mask] = thisin;
				out[i] = 0.f;
			} else {
				if (irdphase1 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
				} else if (irdphase2 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
				} else if (irdphase3 < 0) {
					d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
				} else {
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
					d3 = dlybuf[irdphase3 & mask];
				}
				float value = cubicinterp(frac, d0, d1, d2, d3);
			float thiscoef = coef[i];
			float onepole = ((1. - fabs(thiscoef)) * value) + (thiscoef * lastsamp);
			dlybuf[iwrphase & mask] = thisin + feedbk * onepole;
			out[i] = lastsamp = onepole;
			}
			iwrphase++;
		};
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		for(int i = 0; i < inNumSamples; i++) {
			curtrig = trig[i];
			if ((prevtrig <= 0.f) && (curtrig > 0.f)) {
			    inputsamps = (long)(delaytime * unit->mRate->mSampleRate + .5f);
			    }
			prevtrig = curtrig;
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			if (inputsamps > 0) {
			    thisin = in[i];
			    --inputsamps;
			    } else {
			    thisin = 0.f;
			    }
			if (irdphase0 < 0) {
				dlybuf[iwrphase & mask] = thisin;
				out[i] = 0.f;
			    } else {
				if (irdphase1 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
				} else if (irdphase2 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
				} else if (irdphase3 < 0) {
					d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
				} else {
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
					d3 = dlybuf[irdphase3 & mask];
				}
				float value = cubicinterp(frac, d0, d1, d2, d3);
			float thiscoef = coef[i];
			float onepole = ((1. - fabs(thiscoef)) * value) + (thiscoef * lastsamp);
			dlybuf[iwrphase & mask] = thisin + feedbk * onepole;
			out[i] = lastsamp = onepole;
			}
			feedbk += feedbk_slope;
			iwrphase++;
		};
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_inputsamps = inputsamps;
	unit->m_prevtrig = prevtrig;
	unit->m_lastsamp = zapgremlins(lastsamp);
	unit->m_iwrphase = iwrphase;

	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= unit->m_idelaylen) {
		SETCALC(Pluck_next_aa);
	}
}

void Pluck_next_kk(Pluck *unit, int inNumSamples)
{
	float *out = OUT(0);
	float *in = IN(0);
	float trig = IN0(1);
	float delaytime = IN0(3);
	float decaytime = IN0(4);
	float coef = IN0(5);
	float lastsamp = unit->m_lastsamp;
	unsigned long inputsamps = unit->m_inputsamps;

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	long mask = unit->m_mask;
	float thisin;

	if ((unit->m_prevtrig <= 0.f) && (trig > 0.f)) {
	    inputsamps = (long)(delaytime * unit->mRate->mSampleRate + .5f);
	    }
	unit->m_prevtrig = trig;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime && coef == unit->m_coef) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;

	    for(int i = 0; i < inNumSamples; i++){
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			if (inputsamps > 0) {
			    thisin = in[i];
			    --inputsamps;
			    } else {
			    thisin = 0.f;
			    }
			float d0 = dlybuf[irdphase0 & mask];
			float d1 = dlybuf[irdphase1 & mask];
			float d2 = dlybuf[irdphase2 & mask];
			float d3 = dlybuf[irdphase3 & mask];
			float value = cubicinterp(frac, d0, d1, d2, d3);
			float onepole = ((1. - fabs(coef)) * value) + (coef * lastsamp);
			dlybuf[iwrphase & mask] = thisin + (feedbk * onepole);
			out[i] = lastsamp = onepole; //value;
			iwrphase++;
		};
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		float curcoef = unit->m_coef;
		float coef_slope = CALCSLOPE(coef, curcoef);

	    for(int i = 0; i < inNumSamples; i++){
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			if (inputsamps > 0) {
			    thisin = in[i];
			    --inputsamps;
			    } else {
			    thisin = 0.f;
			    }
			float d0 = dlybuf[irdphase0 & mask];
			float d1 = dlybuf[irdphase1 & mask];
			float d2 = dlybuf[irdphase2 & mask];
			float d3 = dlybuf[irdphase3 & mask];
			float value = cubicinterp(frac, d0, d1, d2, d3);
			float onepole = ((1. - fabs(curcoef)) * value) + (curcoef * lastsamp);
			dlybuf[iwrphase & mask] = thisin + (feedbk * onepole);
			out[i] = lastsamp = onepole; //value;
			feedbk += feedbk_slope;
			curcoef += coef_slope;
			iwrphase++;
		};
		unit->m_feedbk = feedbk;
		unit->m_coef = coef;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_inputsamps = inputsamps;
	unit->m_lastsamp = zapgremlins(lastsamp);
	unit->m_iwrphase = iwrphase;

}


void Pluck_next_kk_z(Pluck *unit, int inNumSamples)
{
	float *out = OUT(0);
	float *in = IN(0);
	float trig = IN0(1);
	float delaytime = IN0(3);
	float decaytime = IN0(4);
	float coef = IN0(5);
	float lastsamp = unit->m_lastsamp;

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	long mask = unit->m_mask;
	float d0, d1, d2, d3;
	float thisin;
	unsigned long inputsamps = unit->m_inputsamps;

	if ((unit->m_prevtrig <= 0.f) && (trig > 0.f)) {
	    inputsamps = (long)(delaytime * unit->mRate->mSampleRate + .5f);
	    }
	unit->m_prevtrig = trig;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime && coef == unit->m_coef) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		for(int i = 0; i < inNumSamples; i++){

			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			if (inputsamps > 0) {
			    thisin = in[i];
			    --inputsamps;
			    } else {
			    thisin = 0.f;
			    }
			if (irdphase0 < 0) {
				dlybuf[iwrphase & mask] = thisin;
				out[i] = 0.f;
			} else {
				if (irdphase1 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
				} else if (irdphase2 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
				} else if (irdphase3 < 0) {
					d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
				} else {
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
					d3 = dlybuf[irdphase3 & mask];
				}
				float value = cubicinterp(frac, d0, d1, d2, d3);
			float onepole = ((1. - fabs(coef)) * value) + (coef * lastsamp);
			dlybuf[iwrphase & mask] = thisin + (feedbk * onepole);
			out[i] = lastsamp = onepole; //value;
			}
			iwrphase++;
		};
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		float curcoef = unit->m_coef;
		float coef_slope = CALCSLOPE(coef, curcoef);

		for(int i = 0; i < inNumSamples; i++) {
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			if (inputsamps > 0) {
			    thisin = in[i];
			    --inputsamps;
			    } else {
			    thisin = 0.f;
			    }
			if (irdphase0 < 0) {
				dlybuf[iwrphase & mask] = thisin;
				out[i] = 0.f;
			    } else {
				if (irdphase1 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
				} else if (irdphase2 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
				} else if (irdphase3 < 0) {
					d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
				} else {
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
					d3 = dlybuf[irdphase3 & mask];
				}
			float value = cubicinterp(frac, d0, d1, d2, d3);
			float onepole = ((1. - fabs(curcoef)) * value) + (curcoef * lastsamp);
			dlybuf[iwrphase & mask] = thisin + (feedbk * onepole);
			out[i] = lastsamp = onepole; //value;
			}
			feedbk += feedbk_slope;
			curcoef += coef_slope;
			iwrphase++;
		};
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
		unit->m_coef = coef;
	}

	unit->m_inputsamps = inputsamps;
	unit->m_lastsamp = zapgremlins(lastsamp);
	unit->m_iwrphase = iwrphase;

	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= unit->m_idelaylen) {
		SETCALC(Pluck_next_kk);
	}
}

void Pluck_next_ak(Pluck *unit, int inNumSamples)
{
	float *out = OUT(0);
	float *in = IN(0);
	float *trig = IN(1);
	float delaytime = IN0(3);
	float decaytime = IN0(4);
	float coef = IN0(5);
	float lastsamp = unit->m_lastsamp;
	unsigned long inputsamps = unit->m_inputsamps;

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	long mask = unit->m_mask;
	float thisin, curtrig;
	float prevtrig = unit->m_prevtrig;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
	for(int i = 0; i < inNumSamples; i++){
			curtrig = trig[i];
			if ((prevtrig <= 0.f) && (curtrig > 0.f)) {
			    inputsamps = (long)(delaytime * unit->mRate->mSampleRate + .5f);
			    }
			prevtrig = curtrig;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			if (inputsamps > 0) {
			    thisin = in[i];
			    --inputsamps;
			    } else {
			    thisin = 0.f;
			    }
			float d0 = dlybuf[irdphase0 & mask];
			float d1 = dlybuf[irdphase1 & mask];
			float d2 = dlybuf[irdphase2 & mask];
			float d3 = dlybuf[irdphase3 & mask];
			float value = cubicinterp(frac, d0, d1, d2, d3);
			float onepole = ((1. - fabs(coef)) * value) + (coef * lastsamp);
			dlybuf[iwrphase & mask] = thisin + feedbk * onepole;
			out[i] = lastsamp = onepole;
			iwrphase++;
		};
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		float curcoef = unit->m_coef;
		float coef_slope = CALCSLOPE(coef, curcoef);

	    for(int i = 0; i < inNumSamples; i++){
			curtrig = trig[i];
			if ((prevtrig <= 0.f) && (curtrig > 0.f)) {
			    inputsamps = (long)(delaytime * unit->mRate->mSampleRate + .5f);
			    }
			prevtrig = curtrig;
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			if (inputsamps > 0) {
			    thisin = in[i];
			    --inputsamps;
			    } else {
			    thisin = 0.f;
			    }
			float d0 = dlybuf[irdphase0 & mask];
			float d1 = dlybuf[irdphase1 & mask];
			float d2 = dlybuf[irdphase2 & mask];
			float d3 = dlybuf[irdphase3 & mask];
			float value = cubicinterp(frac, d0, d1, d2, d3);
			float onepole = ((1. - fabs(curcoef)) * value) + (curcoef * lastsamp);
			dlybuf[iwrphase & mask] = thisin + feedbk * onepole;
			out[i] = lastsamp = onepole;
			feedbk += feedbk_slope;
			curcoef += coef_slope;
			iwrphase++;
		};
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
		unit->m_coef = coef;
	}

	unit->m_prevtrig = prevtrig;
	unit->m_inputsamps = inputsamps;
	unit->m_lastsamp = zapgremlins(lastsamp);
	unit->m_iwrphase = iwrphase;

}


void Pluck_next_ak_z(Pluck *unit, int inNumSamples)
{
	float *out = OUT(0);
	float *in = IN(0);
	float *trig = IN(1);
	float delaytime = IN0(3);
	float decaytime = IN0(4);
	float coef = IN0(5);
	float lastsamp = unit->m_lastsamp;

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	long mask = unit->m_mask;
	float d0, d1, d2, d3;
	float thisin, curtrig;
	unsigned long inputsamps = unit->m_inputsamps;
	float prevtrig = unit->m_prevtrig;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime && coef == unit->m_coef) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		for(int i = 0; i < inNumSamples; i++){
			curtrig = trig[i];
			if ((prevtrig <= 0.f) && (curtrig > 0.f)) {
			    inputsamps = (long)(delaytime * unit->mRate->mSampleRate + .5f);
			    }
			prevtrig = curtrig;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			if (inputsamps > 0) {
			    thisin = in[i];
			    --inputsamps;
			    } else {
			    thisin = 0.f;
			    }
			if (irdphase0 < 0) {
				dlybuf[iwrphase & mask] = thisin;
				out[i] = 0.f;
			} else {
				if (irdphase1 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
				} else if (irdphase2 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
				} else if (irdphase3 < 0) {
					d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
				} else {
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
					d3 = dlybuf[irdphase3 & mask];
				}
				float value = cubicinterp(frac, d0, d1, d2, d3);
			float onepole = ((1. - fabs(coef)) * value) + (coef * lastsamp);
			dlybuf[iwrphase & mask] = thisin + feedbk * onepole;
			out[i] = lastsamp = onepole;
			}
			iwrphase++;
		};
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		float curcoef = unit->m_coef;
		float coef_slope = CALCSLOPE(coef, curcoef);

		for(int i = 0; i < inNumSamples; i++) {
			curtrig = trig[i];
			if ((prevtrig <= 0.f) && (curtrig > 0.f)) {
			    inputsamps = (long)(delaytime * unit->mRate->mSampleRate + .5f);
			    }
			prevtrig = curtrig;
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			if (inputsamps > 0) {
			    thisin = in[i];
			    --inputsamps;
			    } else {
			    thisin = 0.f;
			    }
			if (irdphase0 < 0) {
				dlybuf[iwrphase & mask] = thisin;
				out[i] = 0.f;
			    } else {
				if (irdphase1 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
				} else if (irdphase2 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
				} else if (irdphase3 < 0) {
					d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
				} else {
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
					d3 = dlybuf[irdphase3 & mask];
				}
				float value = cubicinterp(frac, d0, d1, d2, d3);
			float onepole = ((1. - fabs(curcoef)) * value) + (curcoef * lastsamp);
			dlybuf[iwrphase & mask] = thisin + feedbk * onepole;
			out[i] = lastsamp = onepole;
			}
			feedbk += feedbk_slope;
			curcoef +=coef_slope;
			iwrphase++;
		};
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
		unit->m_coef = coef;
	}

	unit->m_inputsamps = inputsamps;
	unit->m_prevtrig = prevtrig;
	unit->m_lastsamp = zapgremlins(lastsamp);
	unit->m_iwrphase = iwrphase;

	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= unit->m_idelaylen) {
		SETCALC(Pluck_next_ak);
	}
}


void Pluck_next_ka(Pluck *unit, int inNumSamples)
{
	float *out = OUT(0);
	float *in = IN(0);
	float trig = IN0(1);
	float delaytime = IN0(3);
	float decaytime = IN0(4);
	float *coef = IN(5);
	float lastsamp = unit->m_lastsamp;
	unsigned long inputsamps = unit->m_inputsamps;

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	long mask = unit->m_mask;
	float thisin;

	if ((unit->m_prevtrig <= 0.f) && (trig > 0.f)) {
	    inputsamps = (long)(delaytime * unit->mRate->mSampleRate + .5f);
	    }
	unit->m_prevtrig = trig;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
	for(int i = 0; i < inNumSamples; i++){
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			if (inputsamps > 0) {
			    thisin = in[i];
			    --inputsamps;
			    } else {
			    thisin = 0.f;
			    }
			float d0 = dlybuf[irdphase0 & mask];
			float d1 = dlybuf[irdphase1 & mask];
			float d2 = dlybuf[irdphase2 & mask];
			float d3 = dlybuf[irdphase3 & mask];
			float value = cubicinterp(frac, d0, d1, d2, d3);
			float thiscoef = coef[i];
			float onepole = ((1. - fabs(thiscoef)) * value) + (thiscoef * lastsamp);
			dlybuf[iwrphase & mask] = thisin + feedbk * onepole;
			out[i] = lastsamp = onepole;
			iwrphase++;
		};
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

	    for(int i = 0; i < inNumSamples; i++){
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			if (inputsamps > 0) {
			    thisin = in[i];
			    --inputsamps;
			    } else {
			    thisin = 0.f;
			    }
			float d0 = dlybuf[irdphase0 & mask];
			float d1 = dlybuf[irdphase1 & mask];
			float d2 = dlybuf[irdphase2 & mask];
			float d3 = dlybuf[irdphase3 & mask];
			float value = cubicinterp(frac, d0, d1, d2, d3);
			float thiscoef = coef[i];
			float onepole = ((1. - fabs(thiscoef)) * value) + (thiscoef * lastsamp);
			dlybuf[iwrphase & mask] = thisin + feedbk * onepole;
			out[i] = lastsamp = onepole;
			feedbk += feedbk_slope;
			iwrphase++;
		};
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_inputsamps = inputsamps;
	unit->m_lastsamp = zapgremlins(lastsamp);
	unit->m_iwrphase = iwrphase;

}


void Pluck_next_ka_z(Pluck *unit, int inNumSamples)
{
	float *out = OUT(0);
	float *in = IN(0);
	float trig = IN0(1);
	float delaytime = IN0(3);
	float decaytime = IN0(4);
	float *coef = IN(5);
	float lastsamp = unit->m_lastsamp;

	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	long mask = unit->m_mask;
	float d0, d1, d2, d3;
	float thisin;
	unsigned long inputsamps = unit->m_inputsamps;

	if ((unit->m_prevtrig <= 0.f) && (trig > 0.f)) {
	    inputsamps = (long)(delaytime * unit->mRate->mSampleRate + .5f);
	    }

	unit->m_prevtrig = trig;

	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		for(int i = 0; i < inNumSamples; i++){
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			if (inputsamps > 0) {
			    thisin = in[i];
			    --inputsamps;
			    } else {
			    thisin = 0.f;
			    }
			if (irdphase0 < 0) {
				dlybuf[iwrphase & mask] = thisin;
				out[i] = 0.f;
			} else {
				if (irdphase1 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
				} else if (irdphase2 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
				} else if (irdphase3 < 0) {
					d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
				} else {
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
					d3 = dlybuf[irdphase3 & mask];
				}
				float value = cubicinterp(frac, d0, d1, d2, d3);
			float thiscoef = coef[i];
			float onepole = ((1. - fabs(thiscoef)) * value) + (thiscoef * lastsamp);
			dlybuf[iwrphase & mask] = thisin + feedbk * onepole;
			out[i] = lastsamp = onepole;
			}
			iwrphase++;
		};
	} else {

		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);

		float next_feedbk = CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);

		for(int i = 0; i < inNumSamples; i++) {
			dsamp += dsamp_slope;
			long idsamp = (long)dsamp;
			float frac = dsamp - idsamp;
			long irdphase1 = iwrphase - idsamp;
			long irdphase2 = irdphase1 - 1;
			long irdphase3 = irdphase1 - 2;
			long irdphase0 = irdphase1 + 1;
			if (inputsamps > 0) {
			    thisin = in[i];
			    --inputsamps;
			    } else {
			    thisin = 0.f;
			    }
			if (irdphase0 < 0) {
				dlybuf[iwrphase & mask] = thisin;
				out[i] = 0.f;
			    } else {
				if (irdphase1 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
				} else if (irdphase2 < 0) {
					d1 = d2 = d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
				} else if (irdphase3 < 0) {
					d3 = 0.f;
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
				} else {
					d0 = dlybuf[irdphase0 & mask];
					d1 = dlybuf[irdphase1 & mask];
					d2 = dlybuf[irdphase2 & mask];
					d3 = dlybuf[irdphase3 & mask];
				}
				float value = cubicinterp(frac, d0, d1, d2, d3);
			float thiscoef = coef[i];
			float onepole = ((1. - fabs(thiscoef)) * value) + (thiscoef * lastsamp);
			dlybuf[iwrphase & mask] = thisin + feedbk * onepole;
			out[i] = lastsamp = onepole;
			}
			feedbk += feedbk_slope;
			iwrphase++;
		};
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}

	unit->m_inputsamps = inputsamps;
	unit->m_lastsamp = zapgremlins(lastsamp);
	unit->m_iwrphase = iwrphase;

	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= unit->m_idelaylen) {
		SETCALC(Pluck_next_ka);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////

PluginLoad(Delay)
{
	ft = inTable;

#define DefineInfoUnit(name) \
	(*ft->fDefineUnit)(#name, sizeof(Unit), (UnitCtorFunc)&name##_Ctor, 0, 0);

	DefineInfoUnit(ControlRate);
	DefineInfoUnit(SampleRate);
	DefineInfoUnit(SampleDur);
	DefineInfoUnit(ControlDur);
	DefineInfoUnit(SubsampleOffset);
	DefineInfoUnit(RadiansPerSample);
	DefineInfoUnit(NumInputBuses);
	DefineInfoUnit(NumOutputBuses);
	DefineInfoUnit(NumAudioBuses);
	DefineInfoUnit(NumControlBuses);
	DefineInfoUnit(NumBuffers);
	DefineInfoUnit(NumRunningSynths);

#define DefineBufInfoUnit(name) \
	(*ft->fDefineUnit)(#name, sizeof(BufInfoUnit), (UnitCtorFunc)&name##_Ctor, 0, 0);

	DefineBufInfoUnit(BufSampleRate);
	DefineBufInfoUnit(BufRateScale);
	DefineBufInfoUnit(BufSamples);
	DefineBufInfoUnit(BufFrames);
	DefineBufInfoUnit(BufChannels);
	DefineBufInfoUnit(BufDur);

	DefineDtorCantAliasUnit(PlayBuf);
#if NOTYET
	DefineDtorUnit(SimpleLoopBuf);
#endif
	DefineDtorUnit(RecordBuf);
	DefineDtorUnit(BufRd);
	DefineDtorUnit(BufWr);
	DefineDtorUnit(Pitch);

	DefineSimpleUnit(BufDelayN);
	DefineSimpleUnit(BufDelayL);
	DefineSimpleUnit(BufDelayC);
	DefineSimpleUnit(BufCombN);
	DefineSimpleUnit(BufCombL);
	DefineSimpleUnit(BufCombC);
	DefineSimpleUnit(BufAllpassN);
	DefineSimpleUnit(BufAllpassL);
	DefineSimpleUnit(BufAllpassC);

#define DefineDelayUnit(name) \
	(*ft->fDefineUnit)(#name, sizeof(name), (UnitCtorFunc)&name##_Ctor, \
	(UnitDtorFunc)&DelayUnit_Dtor, 0);

	DefineDelayUnit(DelayN);
	DefineDelayUnit(DelayL);
	DefineDelayUnit(DelayC);
	DefineDelayUnit(CombN);
	DefineDelayUnit(CombL);
	DefineDelayUnit(CombC);
	DefineDelayUnit(AllpassN);
	DefineDelayUnit(AllpassL);
	DefineDelayUnit(AllpassC);

	DefineDtorUnit(PitchShift);
	DefineSimpleUnit(GrainTap);
	DefineSimpleCantAliasUnit(TGrains);
	DefineDtorUnit(ScopeOut);
	DefineDelayUnit(Pluck);

	DefineDtorUnit(LocalBuf);
	DefineSimpleUnit(MaxLocalBufs);
	DefineSimpleUnit(SetBuf);
	DefineSimpleUnit(ClearBuf);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
