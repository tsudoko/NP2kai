#ifdef __linux__
#define _GNU_SOURCE
#else
#define _DEFAULT_SOURCE
#endif
#define _FILE_OFFSET_BITS 64
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <inttypes.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#if defined(__APPLE__) || defined(__NetBSD__)
#define st_atim st_atimespec
#define st_ctim st_ctimespec
#endif
#include "c9.h"
#include "c9hl.h"

#define max(a,b) ((a)>(b)?(a):(b))
#define used(x) ((void)(x))
#define nelem(x) (int)(sizeof(x)/sizeof((x)[0]))

enum
{
	Canrd = 1<<0,
	Canwr = 1<<1,
};

typedef struct
{
	char *name; /* base name */

	uint64_t diri;
	uint64_t diroffset; /* to read dirs correctly */

	C9qid qid;
	C9fid fid;
	C9mode mode; /* mode in which the file was opened */
	uint32_t iounit;
}Fid;

typedef struct
{
	C9tag tag;
}Tag;

static char *t2s[] = {
	[Tversion-Tversion] = "Tversion",
	[Tauth-Tversion] = "Tauth",
	[Tattach-Tversion] = "Tattach",
	[Tflush-Tversion] = "Tflush",
	[Twalk-Tversion] = "Twalk",
	[Topen-Tversion] = "Topen",
	[Tcreate-Tversion] = "Tcreate",
	[Tread-Tversion] = "Tread",
	[Twrite-Tversion] = "Twrite",
	[Tclunk-Tversion] = "Tclunk",
	[Tremove-Tversion] = "Tremove",
	[Tstat-Tversion] = "Tstat",
	[Twstat-Tversion] = "Twstat",
};

static char *modes[] = {
	"read", "write", "rdwr", "exec",
};

static char *Enoauth = "authentication not required";
static char *Eunknownfid = "unknown fid";
static char *Enowstat = "wstat prohibited";
static char *Eperm = "permission denied";
static char *Enowrite = "write prohibited";
static char *Enomem = "out of memory";
static char *Edupfid = "duplicate fid";
static char *Ewalknodir = "walk in non-directory";
static char *Enotfound = "file not found";
static char *Eduptag = "duplicate tag";
static char *Ebotch = "9P protocol botch";
static char *Enocreate = "create prohibited";
static char *Eisdir = "is a directory";
static char *Ebadoffset = "bad offset";

static int in, out, eof;
static C9ctx ctx;
static int debug;
static Fid **fids;
static int numfids;
static Tag **tags;
static int numtags;
static uint8_t *rdbuf;
static uint8_t *wrbuf;
static uint32_t wroff, wrend, wrbufsz;

__attribute__ ((format (printf, 1, 2)))
static void
trace(const char *fmt, ...)
{
	va_list ap;

	if (debug == 0)
		return;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

static int
canrw(int rdonly, int block)
{
	struct timeval t;
	fd_set r, w, e;
	int fl;

	block = 0;

	FD_ZERO(&r);
	FD_SET(in, &r);
	FD_ZERO(&w);
	if (rdonly == 0)
		FD_SET(out, &w);
	FD_ZERO(&e);
	FD_SET(in, &e);
	FD_SET(out, &e);
	memset(&t, 0, sizeof(t));
	t.tv_usec = 1000;
	for (;;) {
		errno = 0;
		if (select(max(in, out)+1, &r, &w, &e, block ? NULL : &t) < 0 || FD_ISSET(in, &e) || FD_ISSET(out, &e)) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		break;
	}

	fl = 0;
	if (FD_ISSET(in, &r))
		fl |= Canrd;
	if (FD_ISSET(out, &w))
		fl |= Canwr;

	return fl;
}

static int
wrsend(void)
{
	uint32_t n;
	int w;

	if (wrend == 0)
		return 0;
	for (n = 0; n < wrend; n += w) {
		errno = 0;
		if ((w = write(out, wrbuf+n, wrend-n)) <= 0) {
			if (errno == EINTR)
				continue;
			if (errno != EPIPE) /* remote end closed */
				perror("write");
			return -1;
		}
	}
	if (debug >= 2)
		trace("<- %d bytes, %d left\n", wrend, wroff-wrend);
	memmove(wrbuf, wrbuf+wrend, wroff-wrend);
	wroff = wroff - wrend;

	return 0;
}

static int
hastag(C9tag tag)
{
	int i;

	for (i = 0; i < numtags; i++) {
		if (tags[i] != NULL && tags[i]->tag == tag)
			return 1;
	}

	return 0;
}

static struct c9hl_entry *
findqid(uint64_t parent, char *name)
{
	int i;
	for (i = 0; i < c9hl_nqids; i++) {
		if (c9hl_qids[i].parent == parent && strcmp(c9hl_qids[i].name, name) == 0)
			return &c9hl_qids[i];
	}
	return NULL;
}

static Fid *
newfid(C9fid fid, C9qid *qid, char **err)
{
	Fid *f, **newfids;
	int i;

	for (i = 0; i < numfids; i++) {
		if (fids[i] != NULL && fids[i]->fid == fid) {
			*err = Edupfid;
			return NULL;
		}
	}

	if ((f = calloc(1, sizeof(*f))) == NULL) {
nomem:
		if (f != NULL) {
			free(f);
		}
		*err = Enomem;
		return NULL;
	}
	f->qid = *qid;
	f->name = c9hl_qids[qid->path].name; /* XXX legacy, remove later */
	if (f->name[0] == 0)
		fprintf(stderr, "%s -> empty file name\n", f->name);

	for (i = 0; i < numfids; i++) {
		if (fids[i] == NULL) {
			fids[i] = f;
			break;
		}
	}
	if (i >= numfids) {
		if ((newfids = realloc(fids, (numfids+1)*sizeof(*fids))) == NULL)
			goto nomem;
		fids = newfids;
		fids[numfids++] = f;
	}

	f->fid = fid;
	return f;
}

static Fid *
findfid(C9fid fid, char **err)
{
	int i;

	for (i = 0; i < numfids; i++) {
		if (fids[i] != NULL && fids[i]->fid == fid) {
			return fids[i];
		}
	}

	*err = Eunknownfid;

	return NULL;
}

static int
delfid(C9fid fid, char **err)
{
	Fid *f;
	int i;

	for (i = 0; i < numfids; i++) {
		f = fids[i];
		if (f != NULL && f->fid == fid) {
			free(f);
			fids[i] = NULL;
			return 0;
		}
	}

	*err = Eunknownfid;

	return -1;
}

static Fid *
walk(C9fid fid, C9fid nfid, char *el[], C9qid *qids[], char **err)
{
	Fid *f;
	struct c9hl_entry *entry;
	int i;

	if ((f = findfid(fid, err)) == NULL)
		return NULL;

	if (el[0] == NULL) { /* nwname = 0 */
		qids[0] = NULL;
		if (fid == nfid)
			return f;
		return newfid(nfid, &f->qid, err);
	}

	if ((f->qid.type & C9qtdir) == 0) { /* has to be a dir */
		*err = Ewalknodir;
		return NULL;
	}

	uint64_t parent = f->qid.path;
	f = NULL;
	for (i = 0; el[i] != NULL; i++) {
		entry = findqid(parent, el[i]);
		if (entry == NULL) {
			*err = Enotfound;
			break;
		}
		qids[i] = &entry->q;
		parent = entry->q.path;
	}

	qids[i] = NULL;
	if (el[i] == NULL) { /* could walk all the way */
		f = newfid(nfid, &entry->q, err);
		if (f != NULL && f->name[0] == '/' && f->name[1] == 0) /* root */
			f->name = "/";
	} else if (i != 0) { /* didn't fail on the first one */
		*err = NULL;
	}

	return f;
}

static int
openfid(Fid *f, C9mode mode, char **err)
{
	f->mode = mode;
	return 0;
}

static int
statfid(C9ctx *c, Fid *f, C9stat *stout, char **err)
{
	memset(stout, 0, sizeof(*stout));
	stout->name = f->name;
	stout->qid = f->qid;
	stout->mode = 0644;
	stout->gid = c9hl_uid;
	stout->uid = c9hl_uid;
	stout->muid = c9hl_uid;
	if(f->qid.type & C9qtdir)
		stout->mode |= 0111 | C9stdir;

	return c9hl_stat(f->qid.path, stout, err, c->aux);
}

static uint8_t *
ctxread(C9ctx *c, uint32_t size, int *err)
{
	uint32_t n;
	int r;

	used(c);
	*err = 0;
	for (n = 0; n < size; n += r) {
		errno = 0;
		if ((r = read(in, rdbuf+n, size-n)) <= 0) {
			if (r == EINTR)
				continue;
			if (r == 0) {
				eof = 1;
			} else {
				*err = 1;
				perror("ctxread");
			}
			return NULL;
		}
	}

	return rdbuf;
}

static uint8_t *
ctxbegin(C9ctx *c, uint32_t size)
{
	uint8_t *b;

	used(c);
	if (wroff + size > wrbufsz) {
		if (wrsend() != 0 || wroff + size > wrbufsz)
			return NULL;
	}
	b = wrbuf + wroff;
	wroff += size;

	return b;
}

static int
ctxend(C9ctx *c)
{
	used(c);
	wrend = wroff;
	return 0;
}

__attribute__ ((format (printf, 1, 2)))
static void
ctxerror(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

static int
s9do(C9error e, char **err)
{
	if (e == 0) {
		*err = NULL;
		return 0;
	}

	switch (e) {
	case C9Einit: *err = "c9: initialization failed"; break;
	case C9Ever: *err = "c9: protocol version doesn't match"; break;
	case C9Epkt: *err = "c9: incoming packet error"; break;
	case C9Etag: *err = "c9: no free tags or bad tag"; break;
	case C9Ebuf: *err = Enomem; break;
	case C9Epath: *err = "c9: path is too long or just invalid"; break;
	case C9Eflush: *err = "c9: limit of outstanding flushes reached"; break;
	case C9Esize: *err = "c9: can't fit data in one message"; break;
	case C9Estr: *err = "c9: bad string"; break;
	default: *err = "c9: unknown error"; break;
	}

	return -1;
}

static int
readf(C9ctx *c, C9tag tag, Fid *f, uint64_t offset, uint32_t size, char **err)
{
	void *p;
	ssize_t n;
	C9stat c9st[16], *c9stp[16];
	int i, num, res;

	if (size > c->msize - 12) /* make sure it fits */
		size = c->msize - 12;

	if ((f->qid.type & C9qtdir) == 0) { /* a file */
		if ((p = malloc(size)) == NULL) {
			*err = Enomem;
			return -1;
		}
		if ((n = c9hl_readf(f->qid.path, p, size, offset, err, c->aux)) < 0) {
			free(p);
			return -1;
		}
		res = s9do(s9read(c, tag, p, n), err);
		free(p);
		if (res != 0)
			return -1;
		trace("<- Rread tag=%d count=%zd data=...\n", tag, n);
		return 0;
	}

	/* dir */
	if (offset != f->diroffset) {
		if (offset == 0) {
			f->diri = 0;
			f->diroffset = 0;
		} else {
			*err = Ebadoffset;
			return -1;
		}
	}

	res = 0;
	num = 0;
	for (i = 0; i < nelem(c9st); i++, f->diri++) {
		if (f->diri >= c9hl_nqids) /* eof */
			break;

		if (c9hl_qids[f->diri].q.path == Qroot)
			continue;

		if (c9hl_qids[f->diri].parent != f->qid.path)
			continue;

		c9st[i].name = c9hl_qids[f->diri].name;
		c9st[i].qid = c9hl_qids[f->diri].q;
		c9st[i].mode = 0644;
		c9st[i].uid = c9hl_uid;
		c9st[i].gid = c9hl_uid;
		c9st[i].muid = c9hl_uid;
		if (c9st[i].qid.type & C9qtdir)
			c9st[i].mode |= 0111 | C9stdir;
		if(c9hl_stat(c9st[i].qid.path, &c9st[i], err, c->aux) < 0)
			return -1;

		c9st[i].name = strdup(c9st[i].name);
		c9stp[num++] = &c9st[i];
	}

	i = num;
	/* FIXME: s9readdir could send less than num entries due to size constraints, we're not handling that case at all */
	if (s9do(s9readdir(c, tag, c9stp, &num, &f->diroffset, size), err) != 0) {
		res = -1;
		goto done;
	}
	trace("<- Rread tag=%d count=%"PRIu64" data=...\n", tag, num);
	if (i < num)
		fprintf(stderr, "sent %d entries instead of the full %d, you will encounter missing entries\n", i, num);

done:
	for (i = 0; i < num; i++)
		free(c9stp[i]->name);
	return res;
}

static int
writef(C9ctx *c, C9tag tag, Fid *f, uint64_t offset, uint32_t size, void *data, char **err)
{
	void *p;
	ssize_t n;
	C9stat c9st[16], *c9stp[16];
	int i, num, res;

	/* XXX: not sure about this size, T/Rwrite do have different arguments */
	if (size > c->msize - 12) /* make sure it fits */
		size = c->msize - 12;

	if ((f->qid.type & C9qtdir) == 0) { /* a file */
		if ((n = c9hl_writef(f->qid.path, data, size, offset, err, c->aux)) < 0) {
			return -1;
		}
		res = s9do(s9write(c, tag, n), err);
		if (res != 0)
			return -1;
		trace("<- Rread tag=%d count=%zd data=...\n", tag, n);
		return 0;
	}

	*err = Ebotch;
	return 1;
}

static void
ctxt(C9ctx *c, C9t *t)
{
	Fid *f;
	C9qid *qids[C9maxpathel+1];
	char *err, *err2;
	C9stat st;
	int i;

	trace("-> %s tag=%d", t2s[t->type-Tversion], t->tag);

	err = NULL;
	if (hastag(t->tag)) {
		err = Eduptag;
	} else {
		switch (t->type){
		case Tversion:
			trace("\n");
			if (s9do(s9version(c), &err) == 0)
				trace("<- Rversion\n");
			break;
		case Tauth:
			trace(" afid=%d uname=\"%s\" aname=\"%s\"\n", t->auth.afid, t->auth.uname, t->auth.aname);
			err = Enoauth;
			break;
		case Tattach:
			trace(" afid=%d fid=%d uname=\"%s\" aname=\"%s\"\n", t->attach.afid, t->fid, t->attach.uname, t->attach.aname);
			if (t->attach.afid != C9nofid) {
				err = Eunknownfid;
			} else if ((f = newfid(t->fid, &c9hl_qids[Qroot].q, &err)) != NULL) {
				f->name = "/";
				if (s9do(s9attach(c, t->tag, &f->qid), &err) == 0)
					trace("<- Rattach\n");
			}
			break;
		case Tflush:
			trace(" oldtag=%d\n", t->flush.oldtag);
			/* FIXME flush it for realz */
			if (s9do(s9flush(c, t->tag), &err) == 0)
				trace("<- Rflush tag=%d\n", t->tag);
			break;
		case Twalk:
			trace(" fid=%d newfid=%d", t->fid, t->walk.newfid);
			for (i = 0; t->walk.wname[i] != NULL; i++)
				trace(" \"%s\"", t->walk.wname[i]);
			trace("\n");
			walk(t->fid, t->walk.newfid, t->walk.wname, qids, &err);
			if (err == NULL && s9do(s9walk(c, t->tag, qids), &err) == 0) {
				trace("<- Rwalk tag=%d ", t->tag);
				for (i = 0; qids[i] != NULL; i++)
					trace("qid=[path=%"PRIu64" type=0x%02x version=%"PRIu32"] ", qids[i]->path, qids[i]->type, qids[i]->version);
				trace("\n");
			}
			break;
		case Topen:
			trace(" fid=%d mode=0x%02x\n", t->fid, t->open.mode);
			if ((f = findfid(t->fid, &err)) != NULL) {
				/*if (t->open.mode != C9read && t->open.mode != C9exec)
					err = Eperm;
				else*/ if (t->open.mode != C9read && (f->qid.type & C9qtdir) != 0)
					err = Eisdir;
				else if (openfid(f, t->open.mode, &err) == 0 && s9do(s9open(c, t->tag, &f->qid, f->iounit), &err) == 0)
					trace("<- Ropen tag=%d qid=[path=%"PRIu64" type=0x%02x version=%"PRIu32"] iounit=%d\n", t->tag, f->qid.path, f->qid.type, f->qid.version, f->iounit);
			}
			break;
		case Tcreate:
			trace("...\n");
			err = Enocreate;
			break;
		case Tread:
			trace(" fid=%d offset=%"PRIu64" count=%"PRIu32"\n", t->fid, t->read.offset, t->read.size);
			if ((f = findfid(t->fid, &err)) != NULL) {
				if ((f->mode & 0xf) == C9write)
					err = Ebotch;
				else if (readf(c, t->tag, f, t->read.offset, t->read.size, &err) != 0)
					trace("readf failed\n");
			}
			break;
		case Twrite:
			trace(" fid=%d offset=%"PRIu64" count=%"PRIu32"\n", t->fid, t->write.offset, t->write.size);
			if ((f = findfid(t->fid, &err)) != NULL) {
				if (((f->mode & 0xf) | C9write) == 0)
					err = Ebotch;
				else if (writef(c, t->tag, f, t->write.offset, t->write.size, t->write.data, &err) != 0)
					trace("writef failed\n");
			}
			break;
		case Tclunk:
			trace(" fid=%d\n", t->fid);
			if (delfid(t->fid, &err) == 0 && s9do(s9clunk(c, t->tag), &err) == 0)
				trace("<- Rclunk tag=%d\n", t->tag);
			break;
		case Tremove:
			trace("\n");
			err = Eperm;
			break;
		case Tstat:
			trace(" fid=%d\n", t->fid);
			if ((f = findfid(t->fid, &err)) != NULL && statfid(c, f, &st, &err) == 0 && s9do(s9stat(c, t->tag, &st), &err) == 0)
				trace("<- Rstat tag=%d ...\n", t->tag);
			break;
		case Twstat:
			if ((f = findfid(t->fid, &err)) == NULL)
				break;
			if ((c9hl_qids[f->qid.path].hl_flags & C9hlwstat) == 0) {
				err = Enowstat;
				break;
			}
			if (s9do(s9wstat(c, t->tag), &err) == 0)
				trace("<- Rwstat tag=%d ...\n", t->tag);
			break;
		}
	}

	if (err != NULL) {
		if (s9do(s9error(c, t->tag, err), &err2) == 0)
			trace("<- Rerror tag=%d \"%s\"\n", t->tag, err);
		else
			fprintf(stderr, "s9error: %s\n", err2);
	}
}

static void
sigdebug(int s)
{
	Fid *f;
	int i, n;

	used(s);
	n = 0;
	for (i = 0; i < numfids; i++) {
		f = fids[i];
		if (f == NULL)
			continue;

		fprintf(stderr, "fid %u ", f->fid);

		if (f->qid.type & C9qtdir)
			fprintf(stderr, "open mode=dir ");
		else
			fprintf(stderr, "open mode=%s%s%s ", modes[(f->mode & 0xf)], (f->mode & C9trunc) ? ",trunc" : "", (f->mode & C9rclose) ? ",rclose" : "");

		fprintf(stderr, "qid=[path=%"PRIu64" type=0x%02x version=%"PRIu32"] iounit=%d ", f->qid.path, f->qid.type, f->qid.version, f->iounit);
		fprintf(stderr, " %s\n", f->name);
		n++;
	}

	fprintf(stderr, "fids\t%d\n", n);
	fprintf(stderr, "tags\t%d\n", numtags);
}

int
c9hl_step(struct c9hl_state *s)
{
	int can;
	if(eof)
		return -1;
	if ((can = canrw(s->rdonly, s->block)) < 0)
		return -1;
	if ((can & Canrd) != 0) { /* if there is data, process it */
		if (s9do(s9proc(&ctx), &s->err) != 0)
			return -1;
		/* give it a chance to receive all the data first */
		s->rdonly = 1;
		s->block = 0;
	} else if (s->block == 0) { /* got all the data */
		if (s->rdonly != 0) { /* wait until we can send OR we get more data */
			s->rdonly = 0;
			s->block = 1;
		}
	} else if (s->rdonly == 0 && (can & Canwr) != 0) { /* can send */
		if (wrsend() != 0) /* send all the data */
			return -1;
		s->rdonly = 1; /* and go back to reading */
		s->block = 1;
	}
	return 0;
}

int
c9hl_init(struct c9hl_state *c9s, int a_debug, int a_in, int a_out)
{
	debug = a_debug;

	in = a_in;
	out = a_out;
	eof = 0;
	fids = NULL;
	numfids = 0;
	tags = NULL;
	numtags = 0;

	memset(&ctx, 0, sizeof(ctx));
	ctx.msize = 64*1024;
	ctx.read = ctxread;
	ctx.begin = ctxbegin;
	ctx.end = ctxend;
	ctx.t = ctxt;
	ctx.error = ctxerror;
	ctx.aux = c9s->aux;

	rdbuf = calloc(1, ctx.msize);
	wrbufsz = ctx.msize;
	wrbuf = calloc(1, wrbufsz);
	wroff = wrend = 0;
	return 0;
}

void
c9hl_deinit(void)
{
	Fid *f;
	int i;
	for (i = 0; i < numfids; i++) {
		if ((f = fids[i]) != NULL) {
			free(f);
		}
	}

	memset(wrbuf, 0, ctx.msize);
	free(wrbuf);
	memset(rdbuf, 0, ctx.msize);
	free(rdbuf);
	free(fids);
}
