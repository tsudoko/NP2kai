
#if defined(NP2_SIZE_QVGA)
enum {
	DLGCFG_WIDTH	= 254,
#if defined(SUPPORT_NP2_THREAD)
	DLGCFG_HEIGHT	= 198
#else
	DLGCFG_HEIGHT	= 180
#endif	/* SUPPORT_NP2_THREAD */
};
#else
enum {
	DLGCFG_WIDTH	= 303,
#if defined(SUPPORT_NP2_THREAD)
	DLGCFG_HEIGHT	= 234
#else
	DLGCFG_HEIGHT	= 214
#endif	/* SUPPORT_NP2_THREAD */
};
#endif


#ifdef __cplusplus
extern "C" {
#endif

int dlgcfg_cmd(int msg, MENUID id, long param);

#ifdef __cplusplus
}
#endif

