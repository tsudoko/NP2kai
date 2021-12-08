#include <stddef.h>
#include <stdint.h>
#include "compiler.h"
#include "cpumem.h"
#include "pccore.h"
#include <io/iocore.h>
#include "c9.h"
#include "c9hl.h"
#include "np2_9p.h"

#define nelem(x) (int)(sizeof(x)/sizeof((x)[0]))

enum {
	Qmem = 1,
	Qio8dir,
	Qio8port,
	Qio8data,
};

char *c9hl_uid = "neko";

struct c9hl_entry c9hl_qids[] = {
	[Qroot] = {"/",   0,     0, {Qroot, 0, C9qtdir}},
	[Qmem]  = {"mem", Qroot, 0, {Qmem,  0, C9qtfile}},
	[Qio8dir]  = {"io8",    Qroot,   0,         {Qio8dir, 0, C9qtdir}},
	[Qio8port] = {"port",   Qio8dir, C9hlwstat, {Qio8port, 0, C9qtfile}},
	[Qio8data] = {"data",   Qio8dir, C9hlwstat, {Qio8data, 0, C9qtfile}},
};
int c9hl_nqids = nelem(c9hl_qids);

static inline size_t
memsize(void)
{
	/* not exactly true (conventional memory size is usually lower, see
	   generic/np2info.c:/^static void info_mem1/ for details) but we
	   don't care as lower conventional memory sizes just leave holes
	   in the address space without making it smaller */
	return 1024*1024 + pccore.extmem*1024*1024;
}

int
c9hl_stat(uint64_t path, C9stat *st, char **err, struct C9aux *aux)
{
	switch(path) {
	case Qio8port:
		st->size = snprintf(NULL, 0, "%d\n", aux->io8_port);
		st->mode |= 0222;
		break;
	case Qio8data:
		st->size = 1;
		st->mode |= 0222;
		break;
	case Qmem:
		st->size = memsize();
		st->mode |= 0222;
		break;
	}
	return 0;
}

int
c9hl_readf(uint64_t path, unsigned char *p, size_t size, int64_t offset, char **err, struct C9aux *aux)
{
	if(offset != (UINT32)offset) {
		*err = "offset too large";
		return -1;
	}
	if(size != (UINT)size) {
		*err = "size too large";
		return -1;
	}
	switch(path) {
	case Qio8port: {
		char buf[16];
		size_t nbuf = snprintf(buf, sizeof buf, "%d\n", aux->io8_port);
		if(offset >= nbuf)
			return 0;
		if(size > nbuf-offset)
			size = nbuf-offset;
		memcpy(p, buf+offset, size);
		return size;
	}
	case Qio8data:
		if(offset >= 1)
			return 0;
		if(size < 1) {
			*err = "size too small";
			return -1;
		}
		p[0] = iocore_inp8(aux->io8_port);
		return 1;
	case Qmem: {
		size_t membytes = memsize();
		offset = MIN(offset, membytes);
		size = MIN(size, membytes-offset);
		MEMP_READS((UINT32)offset, p, (UINT)size);
		return size;
	}
	default:
		*err = "reading not implemented for this file";
	}
	return -1;
}

int
c9hl_writef(uint64_t path, unsigned char *p, size_t size, int64_t offset, char **err, struct C9aux *aux)
{
	if(offset != (UINT32)offset) {
		*err = "offset too large";
		return -1;
	}
	if(size != (UINT)size) {
		*err = "size too large";
		return -1;
	}
	switch(path) {
	case Qio8port: {
		char buf[16];
		UINT port;
		if(offset > 0) {
			*err = "no partial writes allowed";
			return -1;
		}
		if(size > sizeof buf - 1) {
			*err = "size too large";
			return -1;
		}
		memcpy(buf, p, size);
		buf[size] = '\0';
		if(sscanf(buf, "%i", &port) < 1) {
			*err = "invalid port";
			return -1;
		}
		aux->io8_port = port;
		return size;
	}
	case Qio8data:
		if(offset >= 1) {
			*err = "offset too large";
			return -1;
		}
		if(size < 1) {
			*err = "size too small";
			return -1;
		}
		if(size > 1) {
			*err = "size too large";
			return -1;
		}
		iocore_out8(aux->io8_port, p[0]);
		return 1;
	case Qmem: {
		size_t membytes = memsize();
		offset = MIN(offset, membytes);
		size = MIN(size, membytes-offset);
		MEMP_WRITES((UINT32)offset, p, (UINT)size);
		return size;
	}
	default:
		*err = "writing not implemented for this file";
	}
	return -1;
}
