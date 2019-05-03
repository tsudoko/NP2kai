/**
 * @file	sound.c
 * @brief	Implementation of the sound
 */

#include "compiler.h"
#include "sound.h"
#include "cpucore.h"
#include "pccore.h"
#include "iocore.h"
#include "sndcsec.h"
#include "beep.h"
#include "soundmng.h"
#if defined(SUPPORT_WAVEREC)
#include "common/wavefile.h"
#endif	/* defined(SUPPORT_WAVEREC) */
#if defined(SUPPORT_SOUND_THREAD)
#include "np2_thread.h"
#endif	/* defined(SUPPORT_SOUND_THREAD) */

	SOUNDCFG	soundcfg;


#define	STREAM_CBMAX	16

typedef struct {
	void	*hdl;
	SOUNDCB	cbfn;
} CBTBL;

typedef struct {
	SINT32	*buffer;
	SINT32	*ptr;
	UINT	samples;
	UINT	reserve;
	UINT	remain;
#if defined(SUPPORT_WAVEREC)
	WAVEFILEH rec;
#endif	/* defined(SUPPORT_WAVEREC) */
	CBTBL	*cbreg;
	CBTBL	cb[STREAM_CBMAX];
} SNDSTREAM;

static	SNDSTREAM	sndstream;


static void streamreset(void) {

	SNDCSEC_ENTER;
	sndstream.ptr = sndstream.buffer;
	sndstream.remain = sndstream.samples + sndstream.reserve;
	sndstream.cbreg = sndstream.cb;
	SNDCSEC_LEAVE;
}

static void streamprepare(UINT samples) {

	CBTBL	*cb;
	UINT	count;

	count = np2min(sndstream.remain, samples);
	if (count) {
		ZeroMemory(sndstream.ptr, count * 2 * sizeof(SINT32));
		cb = sndstream.cb;
		while(cb < sndstream.cbreg) {
			cb->cbfn(cb->hdl, sndstream.ptr, count);
			cb++;
		}
		sndstream.ptr += count * 2;
		sndstream.remain -= count;
	}
}


#if defined(SUPPORT_WAVEREC)
// ---- wave rec

/**
 * Starts recording
 * @param[in] lpFilename The filename
 * @retval SUCCESS If succeeded
 * @retval FAILURE If failed
 */
#if defined(SUPPORT_SOUND_THREAD)
static BRESULT sound_recstart_dist(const OEMCHAR *lpFilename)
#else
BRESULT sound_recstart(const OEMCHAR *lpFilename)
#endif  /* SUPPORT_SOUND_THREAD */
{
	WAVEFILEH rec;

#if defined(SUPPORT_SOUND_THREAD)
	sound_recstop_dist();
#else
	sound_recstop();
#endif  /* SUPPORT_SOUND_THREAD */
	if (sndstream.buffer == NULL)
	{
		return FAILURE;
	}
	rec = wavefile_create(lpFilename, soundcfg.rate, 16, 2);
	sndstream.rec = rec;
	if (rec)
	{
		return SUCCESS;
	}
	return FAILURE;
}

/**
 * Stops recording
 */
#if defined(SUPPORT_SOUND_THREAD)
static void sound_recstop_dist(void)
#else
void sound_recstop(void)
#endif  /* SUPPORT_SOUND_THREAD */
{
	WAVEFILEH rec;

	rec = sndstream.rec;
	sndstream.rec = NULL;
	wavefile_close(rec);
}

/**
 * is recording?
 * @retval TRUE Yes
 */
BOOL sound_isrecording(void)
{
	return (sndstream.rec != NULL) ? TRUE : FALSE;
}

/**
 * write
 * @param[in] samples The count of samples
 */
static void streamfilewrite(UINT nSamples)
{
	UINT nCount;
	SINT32 buf32[2 * 512];
	CBTBL *cb;
	UINT8 buf[2 * 512][2];
	UINT r;
	UINT i;
	SINT32 nSample;

	while (nSamples)
	{
		nCount = np2min(nSamples, 512);
		memset(buf32, 0, nCount * 2 * sizeof(buf32[0]));
		cb = sndstream.cb;
		while (cb < sndstream.cbreg)
		{
			cb->cbfn(cb->hdl, buf32, nCount);
			cb++;
		}
		r = np2min(sndstream.remain, nCount);
		if (r)
		{
			memcpy(sndstream.ptr, buf32, r * 2 * sizeof(buf32[0]));
			sndstream.ptr += r * 2;
			sndstream.remain -= r;
		}
		for (i = 0; i < nCount * 2; i++)
		{
			nSample = buf32[i];
			if (nSample > 32767)
			{
				nSample = 32767;
			}
			else if (nSample < -32768)
			{
				nSample = -32768;
			}
			// little endianなので satuation_s16は使えない
			buf[i][0] = (UINT8)nSample;
			buf[i][1] = (UINT8)(nSample >> 8);
		}
		wavefile_write(sndstream.rec, buf, nCount * 2 * sizeof(buf[0]));
		nSamples -= nCount;
	}
}

/**
 * fill
 * @param[in] samples The count of samples
 */
static void filltailsample(UINT nCount)
{
	SINT32 *ptr;
	UINT nOrgSize;
	SINT32 nSampleL;
	SINT32 nSampleR;

	nCount = np2min(sndstream.remain, nCount);
	if (nCount)
	{
		ptr = sndstream.ptr;
		nOrgSize = (UINT)((ptr - sndstream.buffer) / 2);
		if (nOrgSize == 0)
		{
			nSampleL = 0;
			nSampleR = 0;
		}
		else
		{
			nSampleL = *(ptr - 2);
			nSampleR = *(ptr - 1);
		}
		sndstream.ptr += nCount * 2;
		sndstream.remain -= nCount;
		do
		{
			ptr[0] = nSampleL;
			ptr[1] = nSampleR;
			ptr += 2;
		} while (--nCount);
	}
}
#endif	/* defined(SUPPORT_WAVEREC) */


// ----

#if defined(SUPPORT_SOUND_THREAD)
static BRESULT sound_create_dist(UINT rate, UINT ms) {
#else
BRESULT sound_create(UINT rate, UINT ms) {
#endif  /* SUPPORT_SOUND_THREAD */

	UINT	samples;
	UINT	reserve;

	ZeroMemory(&sndstream, sizeof(sndstream));
	switch(rate) {
		case 11025:
		case 22050:
		case 44100:
		case 48000:
		case 88200:
		case 96000:
		case 176400:
		case 192000:
			break;

		default:
			return(FAILURE);
	}
	samples = soundmng_create(rate, ms);
	if (samples == 0) {
		goto scre_err1;
	}
	soundmng_reset();

	soundcfg.rate = rate;
	sound_changeclock();

#if defined(SOUNDRESERVE)
	reserve = rate * SOUNDRESERVE / 1000;
#else
	reserve = 0;
#endif
	sndstream.buffer = (SINT32 *)_MALLOC((samples + reserve) * 2 
												* sizeof(SINT32), "stream");
	if (sndstream.buffer == NULL) {
		goto scre_err2;
	}
	sndstream.samples = samples;
	sndstream.reserve = reserve;

	SNDCSEC_INIT;
	streamreset();
	return(SUCCESS);

scre_err2:
	soundmng_destroy();

scre_err1:
	return(FAILURE);
}

#if defined(SUPPORT_SOUND_THREAD)
static void sound_destroy_dist(void) {
#else
void sound_destroy(void) {
#endif  /* SUPPORT_SOUND_THREAD */

	if (sndstream.buffer) {
#if defined(SUPPORT_WAVEREC)
#if defined(SUPPORT_SOUND_THREAD)
		sound_recstop_dist();
#else
		sound_recstop();
#endif  /* SUPPORT_SOUND_THREAD */
#endif	/* defined(SUPPORT_WAVEREC) */
		soundmng_stop();
		streamreset();
		soundmng_destroy();
		SNDCSEC_TERM;
		_MFREE(sndstream.buffer);
		sndstream.buffer = NULL;
	}
}

#if defined(SUPPORT_SOUND_THREAD)
static void sound_reset_dist(void) {
#else
void sound_reset(void) {
#endif  /* SUPPORT_SOUND_THREAD */

	if (sndstream.buffer) {
		soundmng_reset();
		streamreset();
		soundcfg.lastclock = CPU_CLOCK;
		beep_eventreset();
	}
}

#if defined(SUPPORT_SOUND_THREAD)
static void sound_changeclock_dist(void) {
#else
void sound_changeclock(void) {
#endif  /* SUPPORT_SOUND_THREAD */

	UINT32	clk;
	UINT	hz;
	UINT	hzmax;

	if (sndstream.buffer == NULL) {
		return;
	}

	// とりあえず 25で割り切れる。
	clk = pccore.realclock / 25;
	hz = soundcfg.rate / 25;

	// で、クロック数に合せて調整。(64bit演算しろよな的)
	hzmax = (1 << (32 - 8)) / (clk >> 8);
	while(hzmax < hz) {
		clk = (clk + 1) >> 1;
		hz = (hz + 1) >> 1;
	}
	TRACEOUT(("hzbase/clockbase = %d/%d", hz, clk));
	soundcfg.hzbase = hz;
	soundcfg.clockbase = clk;
	soundcfg.minclock = 2 * clk / hz;
	soundcfg.lastclock = CPU_CLOCK;
}

#if defined(SUPPORT_SOUND_THREAD)
static void sound_streamregist_dist(void *hdl, SOUNDCB cbfn) {
#else
void sound_streamregist(void *hdl, SOUNDCB cbfn) {
#endif  /* SUPPORT_SOUND_THREAD */

	if (sndstream.buffer) {
		if ((cbfn) &&
			(sndstream.cbreg < (sndstream.cb + STREAM_CBMAX))) {
			sndstream.cbreg->hdl = hdl;
			sndstream.cbreg->cbfn = cbfn;
			sndstream.cbreg++;
		}
	}
}


// ----

#if defined(SUPPORT_SOUND_THREAD)
static void sound_sync_dist(void) {
#else
void sound_sync(void) {
#endif  /* SUPPORT_SOUND_THREAD */

	UINT32	length;

	if (sndstream.buffer == NULL) {
		return;
	}

	length = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK - soundcfg.lastclock;
	if (length < soundcfg.minclock) {
		return;
	}
	length = (length * soundcfg.hzbase) / soundcfg.clockbase;
	if (length == 0) {
		return;
	}
	SNDCSEC_ENTER;
#if defined(SUPPORT_WAVEREC)
	if (sndstream.rec) {
		streamfilewrite(length);
	}
	else
#endif	/* defined(SUPPORT_WAVEREC) */
		streamprepare(length);
	soundcfg.lastclock += length * soundcfg.clockbase / soundcfg.hzbase;
	beep_eventreset();
	SNDCSEC_LEAVE;

	soundcfg.writecount += length;
	if (soundcfg.writecount >= 100) {
		soundcfg.writecount = 0;
		soundmng_sync();
	}
}

static volatile int locks = 0;

#if defined(SUPPORT_SOUND_THREAD)
static const SINT32 *sound_pcmlock_dist(void) {
#else
const SINT32 *sound_pcmlock(void) {
#endif  /* SUPPORT_SOUND_THREAD */

const SINT32 *ret;

#if defined(SUPPORT_SOUND_THREAD)
	if(np2_thread_en) {
		ret = (const SINT32 *)1;
	} else {
#endif  /* SUPPORT_SOUND_THREAD */
	if (locks) {
		TRACEOUT(("sound pcm lock: already locked"));
		return(NULL);
	}
	locks++;
	ret = sndstream.buffer;
#if defined(SUPPORT_SOUND_THREAD)
	}
#endif  /* SUPPORT_SOUND_THREAD */
	if (ret) {
		SNDCSEC_ENTER;
		if (sndstream.remain > sndstream.reserve)
#if defined(SUPPORT_WAVEREC)
			if (sndstream.rec) {
				filltailsample(sndstream.remain - sndstream.reserve);
			}
			else
#endif	/* defined(SUPPORT_WAVEREC) */
		{
			streamprepare(sndstream.remain - sndstream.reserve);
			soundcfg.lastclock = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
			beep_eventreset();
		}
	}
	else {
		locks--;
	}
	return(ret);
}

#if defined(SUPPORT_SOUND_THREAD)
static void sound_pcmmix_dist(
#else
void sound_pcmmix(
#endif  /* SUPPORT_SOUND_THREAD */
  void PARTSCALL (*fnmix)(SINT16*, const SINT32*, UINT),
  SINT16* dst,
  const SINT32* src,
  UINT size
) {
  (*fnmix)(dst, src, size);
}

#if defined(SUPPORT_SOUND_THREAD)
static void sound_pcmunlock_dist(const SINT32 *hdl) {
#else
void sound_pcmunlock(const SINT32 *hdl) {
#endif  /* SUPPORT_SOUND_THREAD */

	int		leng;

	if (hdl) {
		leng = sndstream.reserve - sndstream.remain;
		if (leng > 0) {
			CopyMemory(sndstream.buffer,
						sndstream.buffer + (sndstream.samples * 2),
												leng * 2 * sizeof(SINT32));
		}
		sndstream.ptr = sndstream.buffer + (leng * 2);
		sndstream.remain = sndstream.samples + sndstream.reserve - leng;
//		sndstream.remain += sndstream.samples;
		SNDCSEC_LEAVE;
#if defined(SUPPORT_SOUND_THREAD)
	if(!np2_thread_en) {
#endif  /* SUPPORT_SOUND_THREAD */
		locks--;
#if defined(SUPPORT_SOUND_THREAD)
	}
#endif  /* SUPPORT_SOUND_THREAD */
	}
}

#if defined(SUPPORT_SOUND_THREAD)
typedef enum {
  NP2_THREAD_SOUND_NOP = 0,
#if defined(SUPPORT_WAVEREC)
  NP2_THREAD_SOUND_SOUND_RECSTART,
  NP2_THREAD_SOUND_SOUND_RECSTOP,
#endif  /* SUPPORT_WAVEREC */
  NP2_THREAD_SOUND_CREATE,
  NP2_THREAD_SOUND_DESTROY,
  NP2_THREAD_SOUND_RESET,
  NP2_THREAD_SOUND_CHANGECLOCK,
  NP2_THREAD_SOUND_STREAMREGIST,
  NP2_THREAD_SOUND_SYNC,
  NP2_THREAD_SOUND_PCMLOCK,
  NP2_THREAD_SOUND_PCMMIX,
  NP2_THREAD_SOUND_PCMUNLOCK,
} NP2_Thread_sound_func_t;

typedef struct NP2_Thread_sound_req_base_t_ {
  NP2_Thread_sound_func_t func;
  void* param;
} NP2_Thread_sound_req_base_t;

#if defined(SUPPORT_WAVEREC)
typedef struct NP2_Thread_sound_req_recstart_t_ {
  NP2_Thread_sound_func_t func;
  char filename[1024];
} NP2_Thread_sound_req_recstart_t;
#endif  /* SUPPORT_WAVEREC */

typedef struct NP2_Thread_sound_req_create_t_ {
  NP2_Thread_sound_func_t func;
  UINT rate;
  UINT ms;
} NP2_Thread_sound_req_create_t;

typedef struct NP2_Thread_sound_req_streamregist_t_ {
  NP2_Thread_sound_func_t func;
  void *hdl;
  SOUNDCB cbfn;
} NP2_Thread_sound_req_streamregist_t;

typedef struct NP2_Thread_sound_req_pcmmix_t_ {
  NP2_Thread_sound_func_t func;
  void PARTSCALL (*fnmix)(SINT16*, const SINT32*, UINT);
  SINT16* dst;
  const SINT32* src;
  UINT size;
} NP2_Thread_sound_req_pcmmix_t;

typedef union NP2_Thread_sound_req_t_ {
  NP2_Thread_sound_req_base_t base;
#if defined(SUPPORT_WAVEREC)
  NP2_Thread_sound_req_recstart_t recstart;
#endif  /* SUPPORT_WAVEREC */
  NP2_Thread_sound_req_create_t create;
  NP2_Thread_sound_req_streamregist_t streamregist;
  NP2_Thread_sound_req_pcmmix_t pcmmix;
} NP2_Thread_sound_req_t;

static NP2_Thread_t thSound;
static NP2_WaitQueue_t queSound;
static NP2_Semaphore_t semSound;

#define NP2_SOUND_WAITQUEUE_RING

static void* NP2_Sound(void* param) {
  int loop = 1;
  NP2_Thread_sound_req_t* req;

  (void)param;
  NP2_Semaphore_Create(&semSound, 1);
#if defined(NP2_SOUND_WAITQUEUE_RING)
  NP2_WaitQueue_Ring_Create(&queSound, 256);
#else
  NP2_WaitQueue_List_Create(&queSound);
#endif  /* NP2_SOUND_WAITQUEUE_RING */

  while(loop) {
    NP2_WaitQueue_Shift_Wait(&queSound, &semSound, (void**)&req);

    switch(req->base.func) {
    case NP2_THREAD_SOUND_NOP:
      break;
#if defined(SUPPORT_WAVEREC)
    case NP2_THREAD_SOUND_SOUND_RECSTART:
      sound_recstart_dist((const OEMCHAR*)req->recstart.filename);
      break;
    case NP2_THREAD_SOUND_SOUND_RECSTOP:
      sound_recstop_dist();
      break;
#endif  /* SUPPORT_WAVEREC */
    case NP2_THREAD_SOUND_CREATE:
      sound_create_dist(req->create.rate, req->create.ms);
      break;
    case NP2_THREAD_SOUND_DESTROY:
      sound_destroy_dist();
      loop = 0;
      break;
    case NP2_THREAD_SOUND_RESET:
      sound_reset_dist();
      break;
    case NP2_THREAD_SOUND_CHANGECLOCK:
      sound_changeclock_dist();
      break;
    case NP2_THREAD_SOUND_STREAMREGIST:
      sound_streamregist_dist(
        req->streamregist.hdl,
        req->streamregist.cbfn
      );
      break;
    case NP2_THREAD_SOUND_SYNC:
      sound_sync_dist();
      break;
    case NP2_THREAD_SOUND_PCMLOCK:
      sound_pcmlock_dist();
      break;
    case NP2_THREAD_SOUND_PCMMIX:
      sound_pcmmix_dist(
        req->pcmmix.fnmix,
        req->pcmmix.dst,
        req->pcmmix.src,
        req->pcmmix.size
      );
      break;
    case NP2_THREAD_SOUND_PCMUNLOCK:
      sound_pcmunlock_dist((const SINT32 *)1);
      break;
    }

#if !defined(NP2_SOUND_WAITQUEUE_RING)
    if(queSound.list.type == NP2_WAITQUEUE_TYPE_LIST)
      free(req);
#endif  /* NP2_SOUND_WAITQUEUE_RING */
  }

  NP2_WaitQueue_Destroy(&queSound);
  NP2_Semaphore_Destroy(&semSound);

  NP2_Thread_Exit(NULL);

  return NULL;
}

#if defined(SUPPORT_WAVEREC)
BRESULT sound_recstart(const OEMCHAR *filename) {
  NP2_Thread_sound_req_t* preq;

  if(np2_thread_en) {
#if defined(NP2_SOUND_WAITQUEUE_RING)
    preq = NP2_WaitQueue_Ring_GetMemory(&queSound, &semSound);
#else
    preq = malloc(sizeof(NP2_Thread_sound_req_t));
#endif  /* NP2_SOUND_WAITQUEUE_RING */
    preq->recstart.func = NP2_THREAD_SOUND_SOUND_RECSTART;
    strcpy(preq->recstart.filename, filename);

    NP2_WaitQueue_Append(&queSound, &semSound, preq);

    return SUCCESS;
  } else {
    return sound_recstart_dist(pparam);
  }
}

void sound_recstop(void) {
  NP2_Thread_sound_req_t* preq;

  if(np2_thread_en) {
#if defined(NP2_SOUND_WAITQUEUE_RING)
    preq = NP2_WaitQueue_Ring_GetMemory(&queSound, &semSound);
#else
    preq = malloc(sizeof(NP2_Thread_sound_req_t));
#endif  /* NP2_SOUND_WAITQUEUE_RING */
    preq->base.func = NP2_THREAD_SOUND_SOUND_RECSTOP;
    preq->base.param = NULL;

    NP2_WaitQueue_Append(&queSound, &semSound, preq);
  } else {
    sound_recstop_dist();
  }
}
#endif  /* SUPPORT_WAVEREC */

BRESULT sound_create(UINT rate, UINT ms) {
  NP2_Thread_sound_req_t* preq;

  if(np2_thread_en) {
    NP2_Thread_Create(&thSound, NP2_Sound, NULL);
    NP2_Sleep_ms(10);

#if defined(NP2_SOUND_WAITQUEUE_RING)
    preq = NP2_WaitQueue_Ring_GetMemory(&queSound, &semSound);
#else
    preq = malloc(sizeof(NP2_Thread_sound_req_t));
#endif  /* NP2_SOUND_WAITQUEUE_RING */
    preq->create.func = NP2_THREAD_SOUND_CREATE;
    preq->create.rate = rate;
    preq->create.ms = ms;

    NP2_WaitQueue_Append(&queSound, &semSound, preq);

    return SUCCESS;
  } else {
    return sound_create_dist(rate, ms);
  }
}

void sound_destroy(void) {
  NP2_Thread_sound_req_t* preq;

  if(np2_thread_en) {
#if defined(NP2_SOUND_WAITQUEUE_RING)
    preq = NP2_WaitQueue_Ring_GetMemory(&queSound, &semSound);
#else
    preq = malloc(sizeof(NP2_Thread_sound_req_t));
#endif  /* NP2_SOUND_WAITQUEUE_RING */
    preq->base.func = NP2_THREAD_SOUND_DESTROY;
    preq->base.param = NULL;

    NP2_WaitQueue_Append(&queSound, &semSound, preq);

    NP2_Sleep_ms(10);
    NP2_Thread_Destroy(&thSound);
  } else {
    sound_destroy_dist();
  }
}

void sound_reset(void) {
  NP2_Thread_sound_req_t* preq;

  if(np2_thread_en) {
#if defined(NP2_SOUND_WAITQUEUE_RING)
    preq = NP2_WaitQueue_Ring_GetMemory(&queSound, &semSound);
#else
    preq = malloc(sizeof(NP2_Thread_sound_req_t));
#endif  /* NP2_SOUND_WAITQUEUE_RING */
    preq->base.func = NP2_THREAD_SOUND_RESET;
    preq->base.param = NULL;

    NP2_WaitQueue_Append(&queSound, &semSound, preq);
  } else {
    sound_reset_dist();
  }
}

void sound_changeclock(void) {
  NP2_Thread_sound_req_t* preq;

  if(np2_thread_en) {
#if defined(NP2_SOUND_WAITQUEUE_RING)
    preq = NP2_WaitQueue_Ring_GetMemory(&queSound, &semSound);
#else
    preq = malloc(sizeof(NP2_Thread_sound_req_t));
#endif  /* NP2_SOUND_WAITQUEUE_RING */
    preq->base.func = NP2_THREAD_SOUND_CHANGECLOCK;
    preq->base.param = NULL;

    NP2_WaitQueue_Append(&queSound, &semSound, preq);
  } else {
    sound_changeclock_dist();
  }
}

void sound_streamregist(void *hdl, SOUNDCB cbfn) {
  NP2_Thread_sound_req_t* preq;

  if(np2_thread_en) {
#if defined(NP2_SOUND_WAITQUEUE_RING)
    preq = NP2_WaitQueue_Ring_GetMemory(&queSound, &semSound);
#else
    preq = malloc(sizeof(NP2_Thread_sound_req_t));
#endif  /* NP2_SOUND_WAITQUEUE_RING */
    preq->streamregist.func = NP2_THREAD_SOUND_STREAMREGIST;
    preq->streamregist.hdl = hdl;
    preq->streamregist.cbfn = cbfn;

    NP2_WaitQueue_Append(&queSound, &semSound, preq);
    NP2_Sleep_ms(10);	/* need? */
  } else {
    sound_streamregist_dist(hdl, cbfn);
  }
}

void sound_sync(void) {
  NP2_Thread_sound_req_t* preq;

  if(np2_thread_en) {
#if defined(NP2_SOUND_WAITQUEUE_RING)
    preq = NP2_WaitQueue_Ring_GetMemory(&queSound, &semSound);
#else
    preq = malloc(sizeof(NP2_Thread_sound_req_t));
#endif  /* NP2_SOUND_WAITQUEUE_RING */
    preq->base.func = NP2_THREAD_SOUND_SYNC;
    preq->base.param = NULL;

    NP2_WaitQueue_Append(&queSound, &semSound, preq);
  } else {
    sound_sync_dist();
  }
}

const SINT32 *sound_pcmlock(void) {
  const SINT32 *ret;
  NP2_Thread_sound_req_t* preq;

  if(np2_thread_en) {
    if (locks) {
      TRACEOUT(("sound pcm lock: already locked"));
      return(NULL);
    }
    locks++;
    ret = sndstream.buffer;

    if (ret) {
#if defined(NP2_SOUND_WAITQUEUE_RING)
      preq = NP2_WaitQueue_Ring_GetMemory(&queSound, &semSound);
#else
      preq = malloc(sizeof(NP2_Thread_sound_req_t));
#endif  /* NP2_SOUND_WAITQUEUE_RING */
      preq->base.func = NP2_THREAD_SOUND_PCMLOCK;
      preq->base.param = NULL;

      NP2_WaitQueue_Append(&queSound, &semSound, preq);
    } else {
      locks--;
    }
    return ret;
  } else {
    return sound_pcmlock_dist();
  }
}

void sound_pcmmix(
  void PARTSCALL (*fnmix)(SINT16*, const SINT32*, UINT),
  SINT16* dst,
  const SINT32* src,
  UINT size
) {
  NP2_Thread_sound_req_t* preq;

  if(np2_thread_en) {

#if defined(NP2_SOUND_WAITQUEUE_RING)
    preq = NP2_WaitQueue_Ring_GetMemory(&queSound, &semSound);
#else
    preq = malloc(sizeof(NP2_Thread_sound_req_t));
#endif  /* NP2_SOUND_WAITQUEUE_RING */
    preq->pcmmix.func = NP2_THREAD_SOUND_PCMMIX;
    preq->pcmmix.fnmix = fnmix;
    preq->pcmmix.dst = dst;
    preq->pcmmix.src = src;
    preq->pcmmix.size = size;

    NP2_WaitQueue_Append(&queSound, &semSound, preq);
  } else {
    sound_pcmmix_dist(fnmix, dst, src, size);
  }
}

void sound_pcmunlock(const SINT32 *hdl) {
  NP2_Thread_sound_req_t* preq;

  if(np2_thread_en) {
    if(hdl) {
#if defined(NP2_SOUND_WAITQUEUE_RING)
      preq = NP2_WaitQueue_Ring_GetMemory(&queSound, &semSound);
#else
      preq = malloc(sizeof(NP2_Thread_sound_req_t));
#endif  /* NP2_SOUND_WAITQUEUE_RING */
      preq->base.func = NP2_THREAD_SOUND_PCMUNLOCK;
      preq->base.param = NULL;

      NP2_WaitQueue_Append(&queSound, &semSound, preq);
      locks--;
    }
  } else {
    sound_pcmunlock_dist(hdl);
  }
}
#endif  /* SUPPORT_SOUND_THREAD */

