#include <compiler.h>

#include <sound/fmgen/fmgen_fmgwrap.h>
#include "np2_fmgen.h"

using namespace FM;

int fmgen_opndata_size = sizeof(OPNData);
int fmgen_opnadata_size = sizeof(OPNAData);
int fmgen_opnbdata_size = sizeof(OPNBData);
int fmgen_opmdata_size = sizeof(OPMData);

//	YM2203(OPN) ----------------------------------------------------
void*	OPN_Construct(void) { return new NP2_OPN; }
void	OPN_Destruct(void* OPN) { if(OPN) delete (NP2_OPN*)OPN; }

bool	OPN_Init(void* OPN, uint c, uint r, bool ip, const char* str) { return ((NP2_OPN*)OPN)->Init(c, r, ip); }
bool	OPN_SetRate(void* OPN, uint c, uint r, bool b) { return ((NP2_OPN*)OPN)->SetRate(c, r); }

void	OPN_SetVolumeFM(void* OPN, int db) { ((NP2_OPN*)OPN)->SetVolumeFM(db); }
void	OPN_SetVolumePSG(void* OPN, int db) { ((NP2_OPN*)OPN)->SetVolumePSG(db); }
void	OPN_SetLPFCutoff(void* OPN, uint freq) { ((NP2_OPN*)OPN)->SetLPFCutoff(freq); }

bool	OPN_Count(void* OPN, int32 us) { return ((NP2_OPN*)OPN)->Count(us); }
int32	OPN_GetNextEvent(void* OPN) { return ((NP2_OPN*)OPN)->GetNextEvent(); }

void	OPN_Reset(void* OPN) { ((NP2_OPN*)OPN)->Reset(); }
void 	SOUNDCALL OPN_Mix(void* OPN, int32* buffer, int nsamples) { ((NP2_OPN*)OPN)->Mix(buffer, nsamples); }
void 	OPN_SetReg(void* OPN, uint addr, uint data) { ((NP2_OPN*)OPN)->SetReg(addr, data); }
uint	OPN_GetReg(void* OPN, uint addr) { return ((NP2_OPN*)OPN)->GetReg(addr); }

uint	OPN_ReadStatus(void* OPN) { return ((NP2_OPN*)OPN)->ReadStatus(); }
uint	OPN_ReadStatusEx(void* OPN) { return ((NP2_OPN*)OPN)->ReadStatusEx(); }

void	OPN_SetChannelMask(void* OPN, uint mask) { ((NP2_OPN*)OPN)->SetChannelMask(mask); }

int	OPN_dbgGetOpOut(void* OPN, int c, int s) { return ((NP2_OPN*)OPN)->dbgGetOpOut(c, s); }
int	OPN_dbgGetPGOut(void* OPN, int c, int s) { return ((NP2_OPN*)OPN)->dbgGetPGOut(c, s); }
void* OPN_dbgGetCh(void* OPN, int c) { return ((NP2_OPN*)OPN)->dbgGetCh(c); }

void	OPN_DataSave(void* OPN, void* opndata) { ((NP2_OPN*)OPN)->DataSave((OPNData*)opndata); }
void	OPN_DataLoad(void* OPN, void* opndata) { ((NP2_OPN*)OPN)->DataLoad((OPNData*)opndata); }

//	YM2608(OPNA) ---------------------------------------------------
void*	OPNA_Construct(void) { return new OPNA; }
void	OPNA_Destruct(void* OPNA) { if(OPNA) delete (NP2_OPNA*)OPNA; }

bool	OPNA_Init(void* OPNA, uint c, uint r, bool b, const char* str) { return ((NP2_OPNA*)OPNA)->Init(c, r, b, str); }
bool	OPNA_LoadRhythmSample(void* OPNA, const char* str) { return ((NP2_OPNA*)OPNA)->LoadRhythmSample(str); }

void	OPNA_SetVolumeFM(void* OPNA, int db) { ((NP2_OPNA*)OPNA)->SetVolumeFM(db); }
void	OPNA_SetVolumePSG(void* OPNA, int db) { ((NP2_OPNA*)OPNA)->SetVolumePSG(db); }
void	OPNA_SetLPFCutoff(void* OPNA, uint freq) { ((NP2_OPNA*)OPNA)->SetLPFCutoff(freq); }

bool	OPNA_SetRate(void* OPNA, uint c, uint r, bool b) { return ((NP2_OPNA*)OPNA)->SetRate(c, r, b); }
void 	SOUNDCALL OPNA_Mix(void* OPNA, int32* buffer, int nsamples) { ((NP2_OPNA*)OPNA)->Mix(buffer, nsamples); }

bool	OPNA_Count(void* OPNA, int32 us) { return ((NP2_OPNA*)OPNA)->Count(us); }
int32	OPNA_GetNextEvent(void* OPNA) { return ((NP2_OPNA*)OPNA)->GetNextEvent(); }

void	OPNA_Reset(void* OPNA) { ((NP2_OPNA*)OPNA)->Reset(); }
void 	OPNA_SetReg(void* OPNA, uint addr, uint data) { ((NP2_OPNA*)OPNA)->SetReg(addr, data); }
uint	OPNA_GetReg(void* OPNA, uint addr) { return ((NP2_OPNA*)OPNA)->GetReg(addr); }

uint	OPNA_ReadStatus(void* OPNA) { return ((NP2_OPNA*)OPNA)->ReadStatus(); }
uint	OPNA_ReadStatusEx(void* OPNA) { return ((NP2_OPNA*)OPNA)->ReadStatusEx(); }

void	OPNA_SetVolumeADPCM(void* OPNA, int db) { ((NP2_OPNA*)OPNA)->SetVolumeADPCM(db); }
void	OPNA_SetVolumeRhythmTotal(void* OPNA, int db) { ((NP2_OPNA*)OPNA)->SetVolumeRhythmTotal(db); }
void	OPNA_SetVolumeRhythm(void* OPNA, int index, int db) { ((NP2_OPNA*)OPNA)->SetVolumeRhythm(index, db); }

uint8*	OPNA_GetADPCMBuffer(void* OPNA) { return ((NP2_OPNA*)OPNA)->GetADPCMBuffer(); }

int	OPNA_dbgGetOpOut(void* OPNA, int c, int s) { return ((NP2_OPNA*)OPNA)->dbgGetOpOut(c, s); }
int	OPNA_dbgGetPGOut(void* OPNA, int c, int s) { return ((NP2_OPNA*)OPNA)->dbgGetPGOut(c, s); }
void* OPNA_dbgGetCh(void* OPNA, int c) { return ((NP2_OPNA*)OPNA)->dbgGetCh(c); }

void	OPNA_DataSave(void* OPNA, void* opnadata) { ((NP2_OPNA*)OPNA)->DataSave((OPNAData*)opnadata); }
void	OPNA_DataLoad(void* OPNA, void* opnadata) { ((NP2_OPNA*)OPNA)->DataLoad((OPNAData*)opnadata); }

//	YM2610/B(OPNB) -------------------------------------------------
void*	OPNB_Construct(void) { return new OPNB; }
void	OPNB_Destruct(void* OPNB) { if(OPNB) delete (NP2_OPNB*)OPNB; }

bool	OPNB_Init(void* OPNB, uint c, uint r, bool ipflag, uint8 *_adpcma, int _adpcma_size, uint8 *_adpcmb, int _adpcmb_size) { return ((NP2_OPNB*)OPNB)->Init(c, r, ipflag, _adpcma, _adpcma_size, _adpcmb, _adpcmb_size); }

void	OPNB_SetVolumeFM(void* OPNB, int db) { ((NP2_OPNB*)OPNB)->SetVolumeFM(db); }
void	OPNB_SetVolumePSG(void* OPNB, int db) { ((NP2_OPNB*)OPNB)->SetVolumePSG(db); }
void	OPNB_SetLPFCutoff(void* OPNB, uint freq) { ((NP2_OPNB*)OPNB)->SetLPFCutoff(freq); }

bool	OPNB_SetRate(void* OPNB, uint c, uint r, bool b) { return ((NP2_OPNB*)OPNB)->SetRate(c, r, b); }
void 	SOUNDCALL OPNB_Mix(void* OPNB, int32* buffer, int nsamples) { ((NP2_OPNB*)OPNB)->Mix(buffer, nsamples); }

bool	OPNB_Count(void* OPNB, int32 us) { return ((NP2_OPNB*)OPNB)->Count(us); }
int32	OPNB_GetNextEvent(void* OPNB) { return ((NP2_OPNB*)OPNB)->GetNextEvent(); }

void	OPNB_Reset(void* OPNB) { ((NP2_OPNB*)OPNB)->Reset(); }
void 	OPNB_SetReg(void* OPNB, uint addr, uint data) { ((NP2_OPNB*)OPNB)->SetReg(addr, data); }
uint	OPNB_GetReg(void* OPNB, uint addr) { return ((NP2_OPNB*)OPNB)->GetReg(addr); }

uint	OPNB_ReadStatus(void* OPNB) { return ((NP2_OPNB*)OPNB)->ReadStatus(); }
uint	OPNB_ReadStatusEx(void* OPNB) { return ((NP2_OPNB*)OPNB)->ReadStatusEx(); }

void	OPNB_SetVolumeADPCMA(void* OPNB, int index, int db) { ((NP2_OPNB*)OPNB)->SetVolumeADPCMA(index, db); }
void	OPNB_SetVolumeADPCMATotal(void* OPNB, int db) { ((NP2_OPNB*)OPNB)->SetVolumeADPCMATotal(db); }
void	OPNB_SetVolumeADPCMB(void* OPNB, int db) { ((NP2_OPNB*)OPNB)->SetVolumeADPCMB(db); }

void	OPNB_DataSave(void* OPNB, void* opnbdata, void* adpcmadata) { ((NP2_OPNB*)OPNB)->DataSave((OPNBData*)opnbdata, adpcmadata); }
void	OPNB_DataLoad(void* OPNB, void* opnbdata, void* adpcmadata) { ((NP2_OPNB*)OPNB)->DataLoad((OPNBData*)opnbdata, adpcmadata); }

//	YM2151(OPM) ----------------------------------------------------
void*	OPM_Construct(void) { return new OPM; }
void	OPM_Destruct(void* OPM) { if(OPM) delete (NP2_OPM*)OPM; }

bool	OPM_Init(void* OPM, uint c, uint r, bool ip) { return ((NP2_OPM*)OPM)->Init(c, r, ip); }
bool	OPM_SetRate(void* OPM, uint c, uint r, bool b) { return ((NP2_OPM*)OPM)->SetRate(c, r, b); }
void	OPM_Reset(void* OPM) { ((NP2_OPM*)OPM)->Reset(); }

bool	OPM_Count(void* OPM, int32 us) { return ((NP2_OPM*)OPM)->Count(us); }
int32	OPM_GetNextEvent(void* OPM) { return ((NP2_OPM*)OPM)->GetNextEvent(); }

void 	OPM_SetReg(void* OPM, uint addr, uint data) { ((NP2_OPM*)OPM)->SetReg(addr, data); }
uint	OPM_ReadStatus(void* OPM) { return ((NP2_OPM*)OPM)->ReadStatus(); }

void 	SOUNDCALL OPM_Mix(void* OPM, int32* buffer, int nsamples) { ((NP2_OPM*)OPM)->Mix(buffer, nsamples); }

void	OPM_SetVolume(void* OPM, int db) { ((NP2_OPM*)OPM)->SetVolume(db); }
void	OPM_SetChannelMask(void* OPM, uint mask) { ((NP2_OPM*)OPM)->SetChannelMask(mask); }

void	OPM_DataSave(void* OPM, void* opmdata) { ((NP2_OPM*)OPM)->DataSave((OPMData*)opmdata); }
void	OPM_DataLoad(void* OPM, void* opmdata) { ((NP2_OPM*)OPM)->DataLoad((OPMData*)opmdata); }

