struct c9hl_entry {
	char *name;
	uint64_t parent;
	uint32_t hl_flags;
	C9qid q;
};

enum {
	C9hlwstat = 1 << 1, /* allow wstat, useful if you want to allow truncation for example */
};

#define Qroot 0

/* user/group/modified ID for all entries */
extern char *c9hl_uid;
/* qid tree */
/* c9hl_qids[0].q.path == Qroot && c9hl_qids[0].name == "/", the rest is user defined */
extern struct c9hl_entry c9hl_qids[];
/* number of entries in c9hl_qids */
extern int c9hl_nqids;
/* file read handler */
extern int c9hl_readf(uint64_t path, unsigned char *p, size_t size, int64_t offset, char **err, struct C9aux *aux);
/* file write handler */
extern int c9hl_writef(uint64_t path, unsigned char *p, size_t size, int64_t offset, char **err, struct C9aux *aux);

struct c9hl_state {
	char *err;
	int rdonly, block;
	struct C9aux *aux;
};

int c9hl_init(struct c9hl_state *, int a_debug, int a_in, int a_out);
void c9hl_deinit(void);
int c9hl_step(struct c9hl_state *);
