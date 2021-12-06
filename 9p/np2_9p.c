#include <stddef.h>
#include <stdint.h>
#include "compiler.h"
#include "cpumem.h"
#include "c9.h"
#include "c9hl.h"

#define nelem(x) (int)(sizeof(x)/sizeof((x)[0]))

enum {
	Qmem = 1,
	Qtestdir,
	Qass,
};

char *c9hl_uid = "neko";

struct c9hl_entry c9hl_qids[] = {
	[Qroot] = {"/",   0,     {Qroot, 0, C9qtdir}},
	[Qmem]  = {"mem", Qroot, {Qmem,  0, C9qtfile}},
	[Qtestdir] = {"dir",   Qroot,     {Qtestdir, 0, C9qtdir}},
	[Qass] = {"ass",   Qtestdir,     {Qass, 0, C9qtfile}},
};
int c9hl_nqids = nelem(c9hl_qids);

int
c9hl_readf(uint64_t path, unsigned char *p, size_t size, int64_t offset, char **err)
{
	switch(path) {
	case Qmem:
		if(offset != (UINT32)offset) {
			*err = "offset too large";
			return -1;
		}
		if(size != (UINT)size) {
			*err = "size too large";
			return -1;
		}
		MEMP_READS((UINT32)offset, p, (UINT)size);
		return size;
	default:
		*err = "reading not implemented for this file";
	}
	return -1;
}

int
c9hl_writef(uint64_t path, unsigned char *p, size_t size, int64_t offset, char **err)
{
	switch(path) {
	case Qmem:
		if(offset != (UINT32)offset) {
			*err = "offset too large";
			return -1;
		}
		if(size != (UINT)size) {
			*err = "size too large";
			return -1;
		}
		MEMP_WRITES((UINT32)offset, p, (UINT)size);
		return size;
	default:
		*err = "writing not implemented for this file";
	}
	return -1;
}
