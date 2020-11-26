#include "np2_fmgen.h"
#include <pccore.h>
#include <io/iocore.h>
#include <sound/fmboard.h>
#include <sound/opna.h>

namespace FM
{
#ifdef BUILD_OPN
void NP2_OPN::TimerA(NP2_OPN* obj) {
	int i;
	::OPNA* opna = NULL;

	for(i = 0; i < OPNA_MAX; i++) {
		if(obj == g_opna[i].fmgen) {
			opna = &g_opna[i];
			break;
		}
	}

printf("fm int-A 000\n");
	if(opna) {
printf("fm int-A 001\n");
		if (opna->s.irq != 0xff)
		{
			pic_setirq(opna->s.irq);
printf("fm int-A\n");
		}
		OPNBase::TimerA(obj);
	}
}
void NP2_OPN::TimerB(NP2_OPN* obj) {
	int i;
	::OPNA* opna = NULL;

	for(i = 0; i < OPNA_MAX; i++) {
		if(obj == g_opna[i].fmgen) {
			opna = &g_opna[i];
			break;
		}
	}

	if(opna) {
		if (opna->s.irq != 0xff)
		{
			pic_setirq(opna->s.irq);
//			TRACEOUT(("fm int-B"));
		}
		OPNBase::TimerB(obj);
	}
}
#endif

#ifdef BUILD_OPNA
void NP2_OPNA::TimerA(NP2_OPNA* obj) {
	int i;
	::OPNA* opna = NULL;

	for(i = 0; i < OPNA_MAX; i++) {
		if(obj == g_opna[i].fmgen) {
			opna = &g_opna[i];
			break;
		}
	}

	if(opna) {
		if (opna->s.irq != 0xff)
		{
			pic_setirq(opna->s.irq);
//			TRACEOUT(("fm int-A"));
		}
		OPNBase::TimerA(obj);
	}
}
void NP2_OPNA::TimerB(NP2_OPNA* obj) {
	int i;
	::OPNA* opna = NULL;

	for(i = 0; i < OPNA_MAX; i++) {
		if(obj == g_opna[i].fmgen) {
			opna = &g_opna[i];
			break;
		}
	}

	if(opna) {
		if (opna->s.irq != 0xff)
		{
			pic_setirq(opna->s.irq);
//			TRACEOUT(("fm int-B"));
		}
		OPNBase::TimerB(obj);
	}
}
#endif

#ifdef BUILD_OPNB
void NP2_OPNB::TimerA(NP2_OPNB* obj) {
	int i;
	::OPNA* opna = NULL;

	for(i = 0; i < OPNA_MAX; i++) {
		if(obj == g_opna[i].fmgen) {
			opna = &g_opna[i];
			break;
		}
	}

	if(opna) {
		if (opna->s.irq != 0xff)
		{
			pic_setirq(opna->s.irq);
//			TRACEOUT(("fm int-A"));
		}
		OPNBase::TimerA(obj);
	}
}
void NP2_OPNB::TimerB(NP2_OPNB* obj) {
	int i;
	::OPNA* opna = NULL;

	for(i = 0; i < OPNA_MAX; i++) {
		if(obj == g_opna[i].fmgen) {
			opna = &g_opna[i];
			break;
		}
	}

	if(opna) {
		if (opna->s.irq != 0xff)
		{
			pic_setirq(opna->s.irq);
//			TRACEOUT(("fm int-B"));
		}
		OPNBase::TimerB(obj);
	}
}
#endif

#ifdef BUILD_OPN2
void NP2_OPN2::TimerA(NP2_OPN2* obj) {
	int i;
	::OPNA* opna = NULL;

	for(i = 0; i < OPNA_MAX; i++) {
		if(obj == g_opna[i].fmgen) {
			opna = &g_opna[i];
			break;
		}
	}

	if(opna) {
		if (opna->s.irq != 0xff)
		{
			pic_setirq(opna->s.irq);
//			TRACEOUT(("fm int-A"));
		}
		OPNBase::TimerA(obj);
	}
}
void NP2_OPN2::TimerB(NP2_OPN2* obj) {
	int i;
	::OPNA* opna = NULL;

	for(i = 0; i < OPNA_MAX; i++) {
		if(obj == g_opna[i].fmgen) {
			opna = &g_opna[i];
			break;
		}
	}

	if(opna) {
		if (opna->s.irq != 0xff)
		{
			pic_setirq(opna->s.irq);
//			TRACEOUT(("fm int-B"));
		}
		OPNBase::TimerB(obj);
	}
}
#endif

#ifdef BUILD_OPM
void NP2_OPM::TimerA(NP2_OPM* obj) {
	int i;
	::OPNA* opna = NULL;

	for(i = 0; i < OPNA_MAX; i++) {
		if(obj == g_opna[i].fmgen) {
			opna = &g_opna[i];
			break;
		}
	}

	if(opna) {
		if (opna->s.irq != 0xff)
		{
			pic_setirq(opna->s.irq);
//			TRACEOUT(("fm int-A"));
		}
		OPM::TimerA(obj);
	}
}
void NP2_OPM::TimerB(NP2_OPM* obj) {
	int i;
	::OPNA* opna = NULL;

	for(i = 0; i < OPNA_MAX; i++) {
		if(obj == g_opna[i].fmgen) {
			opna = &g_opna[i];
			break;
		}
	}

	if(opna) {
		if (opna->s.irq != 0xff)
		{
			pic_setirq(opna->s.irq);
//			TRACEOUT(("fm int-B"));
		}
		OPM::TimerB(obj);
	}
}
#endif
}

