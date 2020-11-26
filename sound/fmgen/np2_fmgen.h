#ifndef NP2_FMGEN_H
#define NP2_FMGEN_H

#include <compiler.h>

#include "fmgen_opm.h"
#include "fmgen_opna.h"
#include "fmgen_fmgen.h"

#define BUILD_OPN
#define BUILD_OPNA
#define BUILD_OPNB
#define BUILD_OPM

namespace FM
{
#ifdef BUILD_OPN
	class NP2_OPN : public OPN
	{
	protected:
		void	TimerA(NP2_OPN* obj);
		void	TimerB(NP2_OPN* obj);
	};
#endif

#ifdef BUILD_OPNA
	class NP2_OPNA : public OPNA
	{
	protected:
		void	TimerA(NP2_OPNA* obj);
		void	TimerB(NP2_OPNA* obj);
	};
#endif

#ifdef BUILD_OPNB
	class NP2_OPNB : public OPNB
	{
	protected:
		void	TimerA(NP2_OPNB* obj);
		void	TimerB(NP2_OPNB* obj);
	};
#endif

#ifdef BUILD_OPN2
	class NP2_OPN2 : public OPN2
	{
	protected:
		void	TimerA(NP2_OPN2* obj);
		void	TimerB(NP2_OPN2* obj);
	};
#endif

#ifdef BUILD_OPM
	class NP2_OPM : public OPM
	{
	protected:
		void	TimerA(NP2_OPM* obj);
		void	TimerB(NP2_OPM* obj);
	};
#endif
}

#endif  // NP2_FMGEN_H
