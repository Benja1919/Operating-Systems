#include "os.h"
#include <stdint.h>

/*********************************Some Explainations about the code************************************/
/*
											id 208685784
				when we use << 12 we are actually moving left due to the flags in the adress.
				the remaining bits actually present the desired frame.
				The string 0x1ff represents by hexadecimal representation the desired mask
				which is ..0111111111 
				The virtual adress consists of VPN parts as described in the lecture.
				Every such "part" is 9 bits, so in order to find the desired entry we need
				to find the correct PTE matching the current level
								                                                                      */
/******************************************************************************************************/

#define PT_SIZE 64 /*By definition*/
#define FLAGS 12 /*By definition*/
#define SGN_EXT 7 /*By definition*/
#define CELLS 9 /*log at the base of 2 of 512 is 9, meaning every symbol is 9 bits*/

void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn);
uint64_t page_table_query(uint64_t pt, uint64_t vpn);


int num_of_levels()
/*A function to determine the depth of the Trie and the number of levels through a page-walk*/
{
	int vpn_size, levels;
	vpn_size = PT_SIZE - FLAGS - SGN_EXT;
	levels = vpn_size / 9;
	return levels;

}
int vpn_index_ent(int i, uint64_t vpn)
/*A function to determine the PTE part in the virtual address matching the curr level */
{
	vpn = (vpn >> (36 - 9*i)) & 0x1ff;
	return vpn;
}

void last_level_update(int i, uint64_t* PTBR, uint64_t vpn, uint64_t ppn)
{
/*A function to handle the case in the last level of the table-update (where i==4) */
	if (ppn != NO_MAPPING) 
		{
			PTBR[vpn_index_ent(i,vpn)] = (ppn << 12) + 1; /*we should update*/
		}
	else 
		{
			PTBR[vpn_index_ent(i,vpn)] = 0; /*we should destroy*/
		}
	return;
}

void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn)
/*A function to create/destroy virtual memory mappings in a page table*/
{
	int i, levels;
	uint64_t frame;
	uint64_t* PTBR;

	frame = pt << 12; //we remove the flags
	i = 0;
	levels = num_of_levels();
	while (i < levels)
	{
		PTBR = phys_to_virt(frame);
		if (i == 4) /*last level = leaves*/
			{
				return last_level_update(i, PTBR, vpn, ppn);
			}
		frame = PTBR[vpn_index_ent(i,vpn)]; /*find the current part in the address*/
		if ((frame & 1) != 0) /*valid bit turned on*/
		{
			frame = frame - 1;
		}
		else /*valid bit turned off*/
		{
			if (ppn == NO_MAPPING)
			{
				return;
			}
			frame = alloc_page_frame() << 12; /*we need to create*/
			PTBR[vpn_index_ent(i,vpn)] = frame + 1;
		}
		i++;
	}

}

uint64_t page_table_query(uint64_t pt, uint64_t vpn)
/*A function to query the mapping of a virtual page number in a page table*/
{
	int i, levels;
	uint64_t frame;
	uint64_t* PTBR;

	frame = pt << 12; //we remove the flags
	i = 0;
	levels = num_of_levels();  
	while (i < levels)
	{
		PTBR = phys_to_virt(frame); 
		frame =  PTBR[vpn_index_ent(i,vpn)]; /*find the current part in the address*/
		if ((frame & 1) == 0) /*meaning the valid bit is turned off*/
		{
			return NO_MAPPING;
		}
		frame = frame - 1;
		i++;
	}
	return frame >> 12;
}
