struct c9hl_entry {
	char *name;
	uint64_t parent;
	C9qid q;
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
extern int c9hl_readf(uint64_t path, unsigned char *p, size_t size, int64_t offset, char **err);
/* file write handler */
extern int c9hl_writef(uint64_t path, unsigned char *p, size_t size, int64_t offset, char **err);

struct c9hl_state {
	char *err;
	int rdonly, block;
};

int c9hl_init(int a_debug, int a_in, int a_out);
void c9hl_deinit(void);
int c9hl_step(struct c9hl_state *);
