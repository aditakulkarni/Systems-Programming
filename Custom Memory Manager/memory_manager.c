/*
 * Binghamton CS 451/551 Project "Memory manager".
 * This file needs to be turned in.	
 */


#include "memory_manager.h"

STRU_MEM_LIST * new_mem_list(size_t size);
STRU_MEM_BATCH * new_mem_batch(STRU_MEM_LIST * mem_list);
static STRU_MEM_LIST * mem_pool = NULL;

/*
 * Print out the current status of the memory manager.
 * Reading this function may help you understand how the memory manager organizes the memory.
 * Do not change the implementation of this function. It will be used to help the grading.
 */
void mem_mngr_print_snapshot(void)
{
    STRU_MEM_LIST * mem_list = NULL;

    printf("============== Memory snapshot ===============\n");

    mem_list = mem_pool; // Get the first memory list
    while(NULL != mem_list)
    {
        STRU_MEM_BATCH * mem_batch = mem_list->first_batch; // Get the first mem batch from the list 

        printf("mem_list %p slot_size %d batch_count %d free_slot_bitmap %p\n", 
                   mem_list, mem_list->slot_size, mem_list->batch_count, mem_list->free_slots_bitmap);
        bitmap_print_bitmap(mem_list->free_slots_bitmap, mem_list->bitmap_size);

        while (NULL != mem_batch)
        {
            printf("\t mem_batch %p batch_mem %p\n", mem_batch, mem_batch->batch_mem);
            mem_batch = mem_batch->next_batch; // get next mem batch
        }

        mem_list = mem_list->next_list;
    }

    printf("==============================================\n");
}

/*
 * Initialize the memory manager.
 * You may add your code related to initialization here if there is any.
 */
void mem_mngr_init(void)
{
    mem_pool = NULL;
}

/*
 * Clean up the memory manager (e.g., release all the memory allocated)
 */
void mem_mngr_leave(void)
{
	STRU_MEM_LIST * mem_list = NULL, * temp_mem_list = NULL;
	STRU_MEM_BATCH * mem_batch = NULL, * temp_mem_batch = NULL;

	mem_list = mem_pool;
	
	while(mem_list != NULL) {
		mem_batch = mem_list->first_batch;

		//Free memory allocated for memory batches
		while(mem_batch != NULL) {
			temp_mem_batch = mem_batch;
			mem_batch = mem_batch->next_batch;
			mem_list->first_batch = mem_batch;
			free(temp_mem_batch->batch_mem);
			free(temp_mem_batch);
		}
		
		//Free memory allocated for free_slots_bitmap
		//Free memory allocated for memory lists
		temp_mem_list = mem_list;
		mem_list = mem_list->next_list;
		mem_pool = mem_list;
		free(temp_mem_list->free_slots_bitmap);
		free(temp_mem_list);
	}
	printf("\nAllocated Heap Memory Freed!\n");
}

/*
 * Allocate a chunk of memory 	
 */
void * mem_mngr_alloc(size_t size)
{
	int list_found_flag = 0, first_empty_slot;
	STRU_MEM_LIST * mem_list = NULL;
	STRU_MEM_BATCH * mem_batch = NULL;
	void *mem_address = NULL;

	//Initialize mem_pool with first mem_list of SLOT_ALLINED_SIZE(size)
	if (mem_pool == NULL) {
		mem_list = new_mem_list(size);
		mem_pool = mem_list;
		list_found_flag=1;
	}
	else {
		//Check if any mem_list contains memory slots of SLOT_ALLINED_SIZE(size)
		mem_list = mem_pool;
		if(mem_list->slot_size == SLOT_ALLINED_SIZE(size)) {
			list_found_flag = 1;
		}
		else {
			while(mem_list->next_list!=NULL) {
				mem_list = mem_list->next_list;
				if(mem_list->slot_size == SLOT_ALLINED_SIZE(size)) {
					list_found_flag = 1;
					break;
				}
			}
		}	
	}

	//If no mem_list are having slots of SLOT_ALLINED_SIZE(size), create new mem_list
	if(!list_found_flag) {
		mem_list->next_list = new_mem_list(size);
		mem_list = mem_list->next_list;
		list_found_flag=1;
	}

	//In mem_list of SLOT_ALLINED_SIZE(size), find the first empty slot in memory batches using free_slots_bitmap
	if(list_found_flag) {
		mem_batch = mem_list->first_batch;
		first_empty_slot = bitmap_find_first_bit(mem_list->free_slots_bitmap,mem_list->bitmap_size,1);	

		if(first_empty_slot!=BITMAP_OP_ERROR) {
			//If empty slot is not found, create new mem_batch and obtain it's first empty slot
			if(first_empty_slot == BITMAP_OP_NOT_FOUND) {
				while(mem_batch->next_batch!=NULL) {
					mem_batch = mem_batch->next_batch;
				}
				mem_batch->next_batch = new_mem_batch(mem_list);
				mem_address = mem_batch->next_batch->batch_mem;
				first_empty_slot = bitmap_find_first_bit(mem_list->free_slots_bitmap,mem_list->bitmap_size,1);
			}
			//If empty slot is found, calculate start address of the free memory slot in the memory batch
			else {
				for(int i=0;i<first_empty_slot/MEM_BATCH_SLOT_COUNT;i++) {
					mem_batch = mem_batch->next_batch;
				}
				mem_address = mem_batch->batch_mem + (mem_list->slot_size*(first_empty_slot%MEM_BATCH_SLOT_COUNT));				
			}
		}
	}

	//Clear the bit of empty slot in free_slots_bitmap
	if(bitmap_clear_bit(mem_list->free_slots_bitmap,mem_list->bitmap_size,first_empty_slot)==BITMAP_OP_SUCCEED);
	//Return start address of empty slot
	return mem_address;
}

/*
 * Free a chunk of memory pointed by ptr
 */
void mem_mngr_free(void * ptr)
{
	STRU_MEM_LIST * mem_list = NULL;
	STRU_MEM_BATCH * mem_batch = NULL;

	//If mem_pool is NULL and free is called, return
	if(mem_pool == NULL) {
		printf("\nNo memory allocated to free!\n");
		return;
	}
	mem_list = mem_pool;

	//Traverse through all memory lists
	while(mem_list!=NULL) {
		mem_batch = mem_list->first_batch;
		int	target_bit_pos=0;

		//Traverse through all memory batches of the memory list
		while(mem_batch != NULL) {
			for(int addr = 0; addr < mem_list->slot_size*MEM_BATCH_SLOT_COUNT; addr += mem_list->slot_size) {

				//Check if ptr address is present in mem_batch and is start address of any slot
				if(ptr == (mem_batch->batch_mem + addr)) {

					//Check if address is not the starting address of an any slot
					if((uintptr_t)ptr % (uintptr_t)(mem_batch->batch_mem + addr) != 0) {
						printf("\nMemory address is not the starting address of any slot");
						return;
					}

					//Check if address is the starting address of an unassigned slot
					if(bitmap_bit_is_set(mem_list->free_slots_bitmap,mem_list->bitmap_size,target_bit_pos)) {
						printf("\nMemory address is the starting address of an unassigned slot - double freeing");
						return;
					}
					//Free the slot by setting the bit in free_slots_bitmap
					bitmap_set_bit(mem_list->free_slots_bitmap,mem_list->bitmap_size,target_bit_pos);
					return;
				}
				target_bit_pos++;
			}
			mem_batch = mem_batch->next_batch;
		}
		mem_list = mem_list->next_list;
	}
	printf("\nMemory address not found! Address outside of memory managed by the manager.");
}

/*
 * Create new mem_batch, preallocate memory slots, create free_slots_bitmap
 */
STRU_MEM_BATCH * new_mem_batch(STRU_MEM_LIST * mem_list) {
	STRU_MEM_BATCH * new_batch = (STRU_MEM_BATCH *)malloc(sizeof(STRU_MEM_BATCH));
	new_batch->batch_mem = malloc(MEM_BATCH_SLOT_COUNT*(mem_list->slot_size));
	new_batch->next_batch = NULL;

	mem_list->batch_count += 1;
	mem_list->free_slots_bitmap = (unsigned char *)realloc(mem_list->free_slots_bitmap,mem_list->batch_count*(MEM_BATCH_SLOT_COUNT/8));
	for(int i=(mem_list->batch_count - 1)*(MEM_BATCH_SLOT_COUNT/8); i<mem_list->batch_count*(MEM_BATCH_SLOT_COUNT/8); i++) {
			mem_list->free_slots_bitmap[i] = 0xFF;
	}
	mem_list->bitmap_size += (MEM_BATCH_SLOT_COUNT/8);

	return new_batch;
}

/*
 * Create new mem_list
 */
STRU_MEM_LIST * new_mem_list(size_t size) {
	STRU_MEM_LIST * mem_list = (STRU_MEM_LIST *)malloc(sizeof(STRU_MEM_LIST));
	mem_list->slot_size = SLOT_ALLINED_SIZE(size);
	mem_list->batch_count = 0;
	mem_list->bitmap_size = 0;
	mem_list->first_batch = new_mem_batch(mem_list);
	mem_list->next_list = NULL;

	return mem_list;
}

