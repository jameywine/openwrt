
#ifndef BLOCK_STATE_PROC_H_BFHWW6AM
#define BLOCK_STATE_PROC_H_BFHWW6AM

struct block_state {
	int block_num;
	int bad_block_num;
};

extern struct block_state nand_block_state;

int block_state_init(void);
#endif /* end of include guard: BLOCK_STATE_PROC_H_BFHWW6AM */
