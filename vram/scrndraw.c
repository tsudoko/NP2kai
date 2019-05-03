#include	"compiler.h"
#include	"scrnmng.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"scrndraw.h"
#include	"sdraw.h"
#include	"dispsync.h"
#include	"palettes.h"
#ifdef SUPPORT_WAB
#include	"wab/wab.h"
#endif
#if defined(SUPPORT_SCRNDRAW_THREAD)
#include	"np2_thread.h"
#endif  /* SUPPORT_SCRNDRAW_THREAD */


	UINT8	renewal_line[SURFACE_HEIGHT];
	UINT8	np2_tram[SURFACE_SIZE];
	UINT8	np2_vram[2][SURFACE_SIZE];
	UINT8	redrawpending = 0;


static void updateallline(UINT32 update) {

	UINT	i;

	for (i=0; i<SURFACE_HEIGHT; i+=4) {
		*(UINT32 *)(renewal_line + i) |= update;
	}
}

#if defined(SUPPORT_SCRNDRAW_THREAD)
static void scrndraw_updateallline_dist(void) {
#else
void scrndraw_updateallline(void) {
#endif  /* SUPPORT_SCRNDRAW_THREAD */
	redrawpending = 1;
}

// ----

#if defined(SUPPORT_SCRNDRAW_THREAD)
static void scrndraw_initialize_dist(void) {
#else
void scrndraw_initialize(void) {
#endif  /* SUPPORT_SCRNDRAW_THREAD */

	ZeroMemory(np2_tram, sizeof(np2_tram));
	ZeroMemory(np2_vram, sizeof(np2_vram));
	updateallline(0x80808080);
	scrnmng_allflash();
}

#if !defined(SUPPORT_SCRNDRAW_THREAD)
void scrndraw_destroy(void) {
}
#endif  /* SUPPORT_SCRNDRAW_THREAD */

#if defined(SUPPORT_SCRNDRAW_THREAD)
static void scrndraw_changepalette_dist(void) {
#else
void scrndraw_changepalette(void) {
#endif  /* SUPPORT_SCRNDRAW_THREAD */

#if defined(SUPPORT_8BPP)
	if (scrnmng_getbpp() == 8) {
		scrnmng_palchanged();
		return;
	}
#endif
	updateallline(0x80808080);
}

static UINT8 rasterdraw(SDRAWFN sdrawfn, SDRAW sdraw, int maxy) {

	RGB32		pal[16];
	SINT32		clk;
	PAL1EVENT	*event;
	PAL1EVENT	*eventterm;
	int			nextupdate;
	int			y;

	//TRACEOUT(("rasterdraw: maxy = %d", maxy));
	CopyMemory(pal, palevent.pal, sizeof(pal));
	clk = maxy;
	clk += 2;
	clk += np2cfg.realpal;
	clk -= 32;
	clk += (gdc.m.para[GDC_SYNC + 5] >> 2) & 0x3f;
	clk *= gdc.rasterclock;
	event = palevent.event;
	eventterm = event + palevent.events;
	nextupdate = 0;
	for (y=2; y<maxy; y+=2) {
		if (event >= eventterm) {
			break;
		}
		// お弁当はあった？
		if (clk < event->clock) {
			if (!(np2cfg.LCD_MODE & 1)) {
				pal_makeanalog(pal, 0xffff);
			}
			else {
				pal_makeanalog_lcd(pal, 0xffff);
			}
			if (np2cfg.skipline) {
				np2_pal32[0].d = np2_pal32[NP2PAL_SKIP].d;
#if defined(SUPPORT_16BPP)
				np2_pal16[0] = np2_pal16[NP2PAL_SKIP];
#endif
			}
			(*sdrawfn)(sdraw, y);
			nextupdate = y;
			// お弁当を食べる
			while(clk < event->clock) {
				((UINT8 *)pal)[event->color] = event->value;
				event++;
				if (event >= eventterm) {
					break;
				}
			}
		}
		clk -= 2 * gdc.rasterclock;
	}
	if (nextupdate < maxy) {
		if (!(np2cfg.LCD_MODE & 1)) {
			pal_makeanalog(pal, 0xffff);
		}
		else {
			pal_makeanalog_lcd(pal, 0xffff);
		}
		if (np2cfg.skipline) {
			np2_pal32[0].d = np2_pal32[NP2PAL_SKIP].d;
#if defined(SUPPORT_16BPP)
			np2_pal16[0] = np2_pal16[NP2PAL_SKIP];
#endif
		}
		(*sdrawfn)(sdraw, maxy);
	}
	if (palevent.vsyncpal) {
		return(2);
	}
	else if (nextupdate) {
		for (y=0; y<nextupdate; y+=2) {
			*(UINT16 *)(renewal_line + y) |= 0x8080;
		}
		return(1);
	}
	else {
		return(0);
	}
}

#if defined(SUPPORT_SCRNDRAW_THREAD)
static UINT8 scrndraw_draw_dist(UINT8 redraw) {
#else
UINT8 scrndraw_draw(UINT8 redraw) {
#endif  /* SUPPORT_SCRNDRAW_THREAD */

	UINT8		ret;
const SCRNSURF	*surf;
const SDRAWFN	*sdrawfn;
	_SDRAW		sdraw;
	UINT8		bit;
	int			i;
	int			height;
	

	if (redraw || redrawpending) {
		updateallline(0x80808080);
		redrawpending = 0;
	}

	ret = 0;
#ifdef SUPPORT_WAB
	if(np2wab.relay & 0x3){
		np2wab_drawframe(); 
		if(!np2wabwnd.multiwindow){
			// XXX: ウィンドウアクセラレータ動作中は内蔵グラフィックを描画しない
			scrnmng_update();
			ret = 1;
			return(ret);
		}
	}
#endif
	surf = scrnmng_surflock();
	if (surf == NULL) {
		goto sddr_exit1;
	}
#if defined(SUPPORT_PC9821)
	if (gdc.analog & 2) {
		sdrawfn = sdraw_getproctblex(surf);
	}
	else
#endif
	sdrawfn = sdraw_getproctbl(surf);
	if (sdrawfn == NULL) {
		goto sddr_exit2;
	}
	
	bit = 0x00;
	if (gdc.mode1 & 0x80) {						// ver0.28
		if (gdcs.grphdisp & GDCSCRN_ENABLE) {
#if defined(SUPPORT_PC9821)
			if ((gdc.analog & 6) == 6) {
				bit |= 0x01;
			}
			else
#endif
			bit |= (1 << gdcs.disp);
		}
		if (gdcs.textdisp & GDCSCRN_ENABLE) {
			bit |= 4;
		}
		else if (gdc.mode1 & 2) {
			bit = 0;
		}
	}
	bit |= 0x80;
	for (i=0; i<SURFACE_HEIGHT; i++) {
		sdraw.dirty[i] = renewal_line[i] & bit;
		if (sdraw.dirty[i]) {
			renewal_line[i] &= ~bit;
		}
	}
	height = surf->height;
	do {
#if defined(SUPPORT_PC9821)
		if (gdc.analog & 2) {
			break;
		}
#endif
#if defined(SUPPORT_CRT15KHZ)
		if (gdc.crt15khz & 2) {
			sdrawfn += 12;
			height >>= 1;
			break;
		}
#endif
		if (gdc.mode1 & 0x10) {
			sdrawfn += 4;
			if (np2cfg.skipline) {
				sdrawfn += 4;
			}
		}
	} while(0);
	switch(bit & 7) {
		case 1:								// grph1
			sdrawfn += 2;
			sdraw.src = np2_vram[0];
			break;

		case 2:								// grph2
			sdrawfn += 2;
			sdraw.src = np2_vram[1];
			break;

		case 4:								// text
			sdrawfn += 1;
			sdraw.src = np2_tram;
			break;

		case 5:								// text + grph1
			sdrawfn += 3;
			sdraw.src = np2_vram[0];
			sdraw.src2 = np2_tram;
			break;

		case 6:								// text + grph2
			sdrawfn += 3;
			sdraw.src = np2_vram[1];
			sdraw.src2 = np2_tram;
			break;
	}
	sdraw.dst = surf->ptr;
	sdraw.width = surf->width;
	sdraw.xbytes = surf->xalign * surf->width;
	sdraw.y = 0;
	sdraw.xalign = surf->xalign;
	sdraw.yalign = surf->yalign;
	if (((gdc.analog & 3) != 1) || (palevent.events >= PALEVENTMAX)) {
		(*(*sdrawfn))(&sdraw, height);
	}
	else {
		ret = rasterdraw(*sdrawfn, &sdraw, height);
	}

sddr_exit2:
	scrnmng_surfunlock(surf);

sddr_exit1:
	return(ret);
}

#if defined(SUPPORT_SCRNDRAW_THREAD)
static void scrndraw_redraw_dist(void) {
#else
void scrndraw_redraw(void) {
#endif  /* SUPPORT_SCRNDRAW_THREAD */

	scrnmng_allflash();
	pal_change(1);
	dispsync_renewalmode();
	scrndraw_draw(1);
}

#if defined(SUPPORT_SCRNDRAW_THREAD)
typedef enum {
  NP2_THREAD_SCRNDRAW_NOP = 0,
  NP2_THREAD_SCRNDRAW_UPDATEALLLINE,
  NP2_THREAD_SCRNDRAW_INITIALIZE,
  NP2_THREAD_SCRNDRAW_DESTROY,
  NP2_THREAD_SCRNDRAW_CHANGEPALETTE,
  NP2_THREAD_SCRNDRAW_DRAW,
  NP2_THREAD_SCRNDRAW_REDRAW,
} NP2_Thread_scrndraw_func_t;

typedef struct NP2_Thread_scrndraw_req_base_t_ {
  NP2_Thread_scrndraw_func_t func;
  void* param;
} NP2_Thread_scrndraw_req_base_t;

typedef struct NP2_Thread_scrndraw_req_draw_t_ {
  NP2_Thread_scrndraw_func_t func;
  UINT8 redraw;
} NP2_Thread_scrndraw_req_draw_t;

typedef union NP2_Thread_scrndraw_req_t_ {
  NP2_Thread_scrndraw_req_base_t base;
  NP2_Thread_scrndraw_req_draw_t draw;
} NP2_Thread_scrndraw_req_t;

static NP2_Thread_t thScrnDraw;
static NP2_WaitQueue_t queScrnDraw;
static NP2_Semaphore_t semScrnDraw;

#define NP2_SCRNDRAW_WAITQUEUE_RING

static void* NP2_ScrnDraw(void* param) {
  int loop = 1;
  NP2_Thread_scrndraw_req_t* req;

  (void)param;
  NP2_Semaphore_Create(&semScrnDraw, 1);
#if defined(NP2_SCRNDRAW_WAITQUEUE_RING)
  NP2_WaitQueue_Ring_Create(&queScrnDraw, 16);
#else
  NP2_WaitQueue_List_Create(&queScrnDraw);
#endif  /* NP2_SCRNDRAW_WAITQUEUE_RING */

  while(loop) {
    NP2_WaitQueue_Shift_Wait(&queScrnDraw, &semScrnDraw, (void**)&req);

    switch(req->base.func) {
    case NP2_THREAD_SCRNDRAW_NOP:
      break;
    case NP2_THREAD_SCRNDRAW_UPDATEALLLINE:
      scrndraw_updateallline_dist();
      break;
    case NP2_THREAD_SCRNDRAW_INITIALIZE:
      scrndraw_initialize_dist();
      break;
    case NP2_THREAD_SCRNDRAW_DESTROY:
      loop = 0;
      break;
    case NP2_THREAD_SCRNDRAW_CHANGEPALETTE:
      scrndraw_changepalette_dist();
      break;
    case NP2_THREAD_SCRNDRAW_DRAW:
      scrndraw_draw_dist(req->draw.redraw);
      break;
    case NP2_THREAD_SCRNDRAW_REDRAW:
      scrndraw_redraw_dist();
      break;
    }

    free(req);
  }

  NP2_WaitQueue_Destroy(&queScrnDraw);
  NP2_Semaphore_Destroy(&semScrnDraw);

  NP2_Thread_Exit(NULL);

  return NULL;
}

void scrndraw_updateallline(void) {
  NP2_Thread_scrndraw_req_t* preq;

  if(np2_thread_en) {
    preq = malloc(sizeof(NP2_Thread_scrndraw_req_t));
    preq->base.func = NP2_THREAD_SCRNDRAW_UPDATEALLLINE;
    preq->base.param = NULL;

    NP2_WaitQueue_Append(&queScrnDraw, &semScrnDraw, preq);
  } else {
    scrndraw_updateallline_dist();
  }
}

void scrndraw_initialize(void) {
  NP2_Thread_scrndraw_req_t* preq;

  if(np2_thread_en) {
    NP2_Thread_Create(&thScrnDraw, NP2_ScrnDraw, NULL);
    NP2_Sleep_ms(10);

    preq = malloc(sizeof(NP2_Thread_scrndraw_req_t));
    preq->base.func = NP2_THREAD_SCRNDRAW_INITIALIZE;
    preq->base.param = NULL;

    NP2_WaitQueue_Append(&queScrnDraw, &semScrnDraw, preq);
  } else {
    scrndraw_initialize_dist();
  }
}

void scrndraw_destroy(void) {
  NP2_Thread_scrndraw_req_t* preq;

  if(np2_thread_en) {
    preq = malloc(sizeof(NP2_Thread_scrndraw_req_t));
    preq->base.func = NP2_THREAD_SCRNDRAW_DESTROY;
    preq->base.param = NULL;

    NP2_WaitQueue_Append(&queScrnDraw, &semScrnDraw, preq);

    NP2_Sleep_ms(10);
    NP2_Thread_Destroy(&thScrnDraw);
  }
}

void scrndraw_changepalette(void) {
  NP2_Thread_scrndraw_req_t* preq;

  if(np2_thread_en) {
    preq = malloc(sizeof(NP2_Thread_scrndraw_req_t));
    preq->base.func = NP2_THREAD_SCRNDRAW_CHANGEPALETTE;
    preq->base.param = NULL;

    NP2_WaitQueue_Append(&queScrnDraw, &semScrnDraw, preq);
  } else {
    scrndraw_changepalette_dist();
  }
}

UINT8 scrndraw_draw(UINT8 redraw) {
  NP2_Thread_scrndraw_req_t* preq;

  if(np2_thread_en) {
    preq = malloc(sizeof(NP2_Thread_scrndraw_req_t));
    preq->draw.func = NP2_THREAD_SCRNDRAW_DRAW;
    preq->draw.redraw = redraw;

    NP2_WaitQueue_Append(&queScrnDraw, &semScrnDraw, preq);
  } else {
    (void)scrndraw_draw_dist(redraw);
  }

  return 1;
}

void scrndraw_redraw(void) {
  NP2_Thread_scrndraw_req_t* preq;

  if(np2_thread_en) {
    preq = malloc(sizeof(NP2_Thread_scrndraw_req_t));
    preq->base.func = NP2_THREAD_SCRNDRAW_REDRAW;
    preq->base.param = NULL;

    NP2_WaitQueue_Append(&queScrnDraw, &semScrnDraw, preq);
  } else {
    scrndraw_redraw_dist();
  }
}
#endif  /* SUPPORT_SCRNDRAW_THREAD */

