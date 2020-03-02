/*
 * Buddy Page Allocation Algorithm
 * SKELETON IMPLEMENTATION -- TO BE FILLED IN FOR TASK (2)
 */

/*
 * STUDENT NUMBER: s1532620
 */
#include <infos/mm/page-allocator.h>
#include <infos/mm/mm.h>
#include <infos/kernel/kernel.h>
#include <infos/kernel/log.h>
#include <infos/util/math.h>
#include <infos/util/printf.h>

using namespace infos::kernel;
using namespace infos::mm;
using namespace infos::util;

#define MAX_ORDER	17

/**
 * A buddy page allocation algorithm.
 */
class BuddyPageAllocator : public PageAllocatorAlgorithm
{
private:
	/**
	 * Returns the number of pages that comprise a 'block', in a given order.
	 * @param order The order to base the calculation off of.
	 * @return Returns the number of pages in a block, in the order.
	 */
	static inline constexpr uint64_t pages_per_block(int order)
	{
		/* The number of pages per block in a given order is simply 1, shifted left by the order number.
		 * For example, in order-2, there are (1 << 2) == 4 pages in each block.
		 */
		return (1 << order);
	}
	
	/**
	 * Returns TRUE if the supplied page descriptor is correctly aligned for the 
	 * given order.  Returns FALSE otherwise.
	 * @param pgd The page descriptor to test alignment for.
	 * @param order The order to use for calculations.
	 */
	static inline bool is_correct_alignment_for_order(const PageDescriptor *pgd, int order)
	{
		// Calculate the page-frame-number for the page descriptor, and return TRUE if
		// it divides evenly into the number pages in a block of the given order.
		return (sys.mm().pgalloc().pgd_to_pfn(pgd) % pages_per_block(order)) == 0;
	}
	
	/** Given a page descriptor, and an order, returns the buddy PGD.  The buddy could either be
	 * to the left or the right of PGD, in the given order.
	 * @param pgd The page descriptor to find the buddy for.
	 * @param order The order in which the page descriptor lives.
	 * @return Returns the buddy of the given page descriptor, in the given order.
	 */
	PageDescriptor *buddy_of(PageDescriptor *pgd, int order)
	{
		//mm_log.messagef(LogLevel::DEBUG, "GETTING THE BUDDY OF:%p IN ORDER:%d", pgd, order);
		// (1) Make sure 'order' is within range
		if (order >= MAX_ORDER) {
			return NULL;
		}

		// (2) Check to make sure that PGD is correctly aligned in the order
		if (!is_correct_alignment_for_order(pgd, order)) {
			return NULL;
		}
				
		// (3) Calculate the page-frame-number of the buddy of this page.
		// * If the PFN is aligned to the next order, then the buddy is the next block in THIS order.
		// * If it's not aligned, then the buddy must be the previous block in THIS order.
		uint64_t buddy_pfn = is_correct_alignment_for_order(pgd, order + 1) ?
			sys.mm().pgalloc().pgd_to_pfn(pgd) + pages_per_block(order) : 
			sys.mm().pgalloc().pgd_to_pfn(pgd) - pages_per_block(order);
		
		// (4) Return the page descriptor associated with the buddy page-frame-number.
		return sys.mm().pgalloc().pfn_to_pgd(buddy_pfn);
	}
	
	/**
	 * Inserts a block into the free list of the given order.  The block is inserted in ascending order.
	 * @param pgd The page descriptor of the block to insert.
	 * @param order The order in which to insert the block.
	 * @return Returns the slot (i.e. a pointer to the pointer that points to the block) that the block
	 * was inserted into.
	 */
	PageDescriptor **insert_block(PageDescriptor *pgd, int order)
	{
		//mm_log.messagef(LogLevel::DEBUG, "INSERTING BLOCK:%p INTO ORDER:%d", sys.mm().pgalloc().pgd_to_pfn(pgd), order);
		// Starting from the _free_area array, find the slot in which the page descriptor
		// should be inserted.

		PageDescriptor **slot = &_free_areas[order];
		// Iterate whilst there is a slot, and whilst the page descriptor pointer is numerically
		// greater than what the slot is pointing to.
		while (*slot && pgd > *slot) {
			slot = &(*slot)->next_free;
		}
		
		// Insert the page descriptor into the linked list.
		pgd->next_free = *slot;
		*slot = pgd;
		
		// Return the insert point (i.e. slot)
		return slot;
	}
	
	/**
	 * Removes a block from the free list of the given order.  The block MUST be present in the free-list, otherwise
	 * the system will panic.
	 * @param pgd The page descriptor of the block to remove.
	 * @param order The order in which to remove the block from.
	 */
	void remove_block(PageDescriptor *pgd, int order)
	{
		//mm_log.messagef(LogLevel::DEBUG, "REMOVING BLOCK:%p IN ORDER:%d",sys.mm().pgalloc().pgd_to_pfn(pgd), order);
		// Starting from the _free_area array, iterate until the block has been located in the linked-list.
		PageDescriptor **slot = &_free_areas[order];
		while (*slot && pgd != *slot) {
			slot = &(*slot)->next_free;
		}

		// Make sure the block actually exists.  Panic the system if it does not.
		assert(*slot == pgd);
		
		// Remove the block from the free list.
		*slot = pgd->next_free;
		pgd->next_free = NULL;
	}
	
	/**
	 * Given a pointer to a block of free memory in the order "source_order", this function will
	 * split the block in half, and insert it into the order below.
	 * @param block_pointer A pointer to a pointer containing the beginning of a block of free memory.
	 * @param source_order The order in which the block of free memory exists.  Naturally,
	 * the split will insert the two new blocks into the order below.
	 * @return Returns the left-hand-side of the new block.
	 */
	PageDescriptor *split_block(PageDescriptor **block_pointer, int source_order)
	{
		// Make sure there is an incoming pointer.
		//mm_log.messagef(LogLevel::DEBUG, "SPLITTING BLOCK:%p IN ORDER:%d",sys.mm().pgalloc().pgd_to_pfn(*block_pointer), source_order);
		assert(*block_pointer);
		
		// Make sure the block_pointer is correctly aligned.
		assert(is_correct_alignment_for_order(*block_pointer, source_order));

		//order to insert the blocks into
		int upper_order = source_order-1;

		// starting address of the first block is the same as the source block
		PageDescriptor *block1 = *block_pointer;

		//starting address of the second block  
		PageDescriptor *block2 = buddy_of(block1, upper_order);
		
		//remove the block from the given order
		remove_block(*block_pointer, source_order);

		//insert both the new blocks into the upper order
		insert_block(block1, upper_order);
		insert_block(block2, upper_order);

		return *block_pointer;		
	}
	
	/**
	 * Takes a block in the given source order, and merges it (and it's buddy) into the next order.
	 * This function assumes both the source block and the buddy block are in the free list for the
	 * source order.  If they aren't this function will panic the system.
	 * @param block_pointer A pointer to a pointer containing a block in the pair to merge.
	 * @param source_order The order in which the pair of blocks live.
	 * @return Returns the new slot that points to the merged block.
	 */
	PageDescriptor **merge_block(PageDescriptor **block_pointer, int source_order)
	{
		//mm_log.messagef(LogLevel::DEBUG, "MERGING BLOCK:%p IN ORDER:%d",*block_pointer,source_order);	
		
		// Make sure the area_pointer is correctly aligned.
		assert(is_correct_alignment_for_order(*block_pointer, source_order)); 

		//pointer to the buddy block
		PageDescriptor *buddy_pointer = buddy_of(*block_pointer, source_order);
		//check that buddy is also correctly aligned

		PageDescriptor* left_block = nullptr;
		PageDescriptor* right_block = nullptr;
		//whichever block address is smaller is the left hand one
		if (*block_pointer > buddy_pointer ){
			left_block = buddy_pointer;
			right_block = *block_pointer;
		}
		else{
			left_block = *block_pointer;
			right_block = buddy_pointer;
		}

		//remove block overwites the pointer you give it so we need a copy of left_block for the insertion
		PageDescriptor* left_cp = left_block;

		
		remove_block(left_block, source_order);
		remove_block(right_block, source_order);

		return insert_block(left_cp, source_order+1) ;


				
	}
	
public:
	/**
	 * Constructs a new instance of the Buddy Page Allocator.
	 */
	BuddyPageAllocator() {
		// Iterate over each free area, and clear it.
		for (unsigned int i = 0; i < ARRAY_SIZE(_free_areas); i++) {
			_free_areas[i] = NULL;
		}
	}
	
	/**
	 * Allocates 2^order number of contiguous pages
	 * @param order The power of two, of the number of contiguous pages to allocate.
	 * @return Returns a pointer to the first page descriptor for the newly allocated page range, or NULL if
	 * allocation failed.
	 */
	PageDescriptor *alloc_pages(int order) override
	{
		//mm_log.messagef(LogLevel::DEBUG, "ALLOCATING PAGES IN ORDER:%d", order);

		//container for the block to be allocated
		PageDescriptor* block_allocated;

		//check all orders starting from the given order and looking upwards to find and make a free
		for (int i = order; i < MAX_ORDER;){

			//pointer to a pointer to the address of the first free block in order i of _free_areas 
			PageDescriptor **pgd = &_free_areas[i];

			//mm_log.messagef(LogLevel::DEBUG, "LOOKING IN ORDER:%d", i);
			//if no free pages exist in the current order then move to the greater one
			if (*pgd == NULL){
				i++;
			}
			else{
				//mm_log.messagef(LogLevel::DEBUG, "FREE PAGE FOUND IN ORDER:%d", i);
				//if a free page exists in the order you want then return the pgd of that page
				if (i == order){

					//remove_block overwrites the pgd its given so i need to save it before removing
					block_allocated = *pgd;
					remove_block(*pgd, i);
					//mm_log.messagef(LogLevel::DEBUG, "ALLOCATING BLOCK:%p ORDER:%d", block_allocated, order);
					return block_allocated;
				}
				// if a free page exists in a higher order then split the block and check the lower order
				else{
					split_block(pgd, i);
					i--;
				}
			}
		}
		//if it gets to the end of this loop then return null
		return NULL;
	}
	
	/**
	 * Frees 2^order contiguous pages.
	 * @param pgd A pointer to an array of page descriptors to be freed.
	 * @param order The power of two number of contiguous pages to free.
	 */
	void free_pages(PageDescriptor *pgd, int order) override
	{
		//mm_log.messagef(LogLevel::DEBUG, "FREEING PAGES STARTING FROM:%p ORDER:%d", sys.mm().pgalloc().pgd_to_pfn(pgd), order);

		// Make sure that the incoming page descriptor is correctly aligned
		// for the order on which it is being freed, for example, it is
		// illegal to free page 1 in order-1.	
		assert(is_correct_alignment_for_order(pgd, order));

		//firstly free the given page
		insert_block(pgd, order);

		//check to see if buddy is also free
		bool buddy_free = is_buddy_free(pgd, order);
		
		//if the buddy of the given pgd is free, then merge them and check the upper orders to see if they can also be merged
		while(buddy_free && order<MAX_ORDER-1){
			
			PageDescriptor** new_block = merge_block(&pgd, order);
			pgd = *new_block;
			order++;
			buddy_free = is_buddy_free(pgd, order);
		}
	}

	/**
	 *checks to see if the buddy of the given pgd is also free and in the same order,
	 * 		returns true if yes, false otherwise
	 * assumes the pgd is in _free_areas for the given order 
	 * @param pgd pointer to the page to find the buddy off
	 * @param order order to find the buddy in
	 */
	bool is_buddy_free(PageDescriptor* pgd, int order){

		
		PageDescriptor *buddy = buddy_of(pgd, order);
		//mm_log.messagef(LogLevel::DEBUG, "PFN:%p BUDDY_PFN:%p", sys.mm().pgalloc().pgd_to_pfn(pgd), sys.mm().pgalloc().pgd_to_pfn(buddy));

		//since the buddies are next to each other we just need to check that at least
		//one of them is next to the other
		if ((pgd->next_free == buddy || buddy->next_free == pgd)){
			return true;
		}
		else{
			return false;
		}	
	}
	
	/**
	 * Reserves a specific page, so that it cannot be allocated.
	 * @param pgd The page descriptor of the page to reserve.
	 * @return Returns TRUE if the reservation was successful, FALSE otherwise.
	 */
	bool reserve_page(PageDescriptor *pgd)
	{
		//mm_log.messagef(LogLevel::DEBUG, "RESERVING PAGE:%p", sys.mm().pgalloc().pgd_to_pfn(pgd));

		// order to start searching from
		int order = MAX_ORDER-1;

		//container for pa
		PageDescriptor* block_with_page ;
		bool page_reserved = false;

		//this nested loop goes through every block in every order in _free_areas to find the block the required page is in, starting from the hightest order
		//when it finds the block the page is in, it splits it and cmoves on to the lower one
		//by the end of this the page will be in the lowest order and will be ava
		while(order >0){
			//mm_log.messagef(LogLevel::DEBUG, "CHECKING BLOCK:%p IN ORDER:%d", sys.mm().pgalloc().pgd_to_pfn(*slot), order);
			
			PageDescriptor **block = &_free_areas[order];
			
			//while there is still a free block to check 
			while(*block){
				
				//if the page is within in the block we are looking at then split the block
				if (pgd >= *block && pgd < *block+pages_per_block(order)){
					//mm_log.messagef(LogLevel::DEBUG, "PFN:%p",sys.mm().pgalloc().pgd_to_pfn(pgd));
					//mm_log.messagef(LogLevel::DEBUG, "PAGE FOUND IN BLOCK:%p", sys.mm().pgalloc().pgd_to_pfn(*slot));

					split_block(block, order);					
				}
				//else, look at the next slot
				else{
					//mm_log.messagef(LogLevel::DEBUG, "PAGE NOT FOUND, CHECKING BLOCK:%p",sys.mm().pgalloc().pgd_to_pfn(*slot) );

					block = &(*block)->next_free;
					
				}

			}
			//mm_log.messagef(LogLevel::DEBUG, "PAGE NOT FOUND IN ORDER:%d CHECKING ORDER:%d",order, order-1);

			//regardless of whether or not the block is found or not, we move on to the next order
			order--;

			//if we are now in the lowest order then the page is available to be reserved
			if (order==0) {
				page_reserved = true;
			}	
		}

		if(page_reserved = true){
			remove_block(pgd, 0);
		}
		return page_reserved;
		
	} 


	/**
	 * Initialises the allocation algorithm.
	 * @param page_descriptors pgd to start allocating from
	 * @param nr_page_descriptors number of pages in the system
	 * @return Returns TRUE if the algorithm was successfully initialised, FALSE otherwise.
	 */
	bool init(PageDescriptor *page_descriptors, uint64_t nr_page_descriptors) override
	{
		mm_log.messagef(LogLevel::DEBUG, "Buddy Allocator Initialising pd=%p, nr=0x%lx", page_descriptors, nr_page_descriptors);
		
		// Initialise the free area linked list for the maximum order
		// to initialise the allocation algorithm.

		int blocks_in_MAX_ORDER = nr_page_descriptors/pages_per_block(MAX_ORDER-1);

		//puts all the blocks into the max order that it can, ignores the left over ones
		for (int i = 0; i < blocks_in_MAX_ORDER; i++){
			insert_block(page_descriptors, MAX_ORDER-1);
			page_descriptors += pages_per_block(MAX_ORDER-1);
		}
		return true;
	}

	/**
	 * Returns the friendly name of the allocation algorithm, for debugging and selection purposes.
	 */
	const char* name() const override { return "buddy"; }
	
	/**
	 * Dumps out the current state of the buddy system
	 */
	void dump_state() const override
	{
		// Print out a header, so we can find the output in the logs.
		mm_log.messagef(LogLevel::DEBUG, "BUDDY STATE:");
		
		// Iterate over each free area.
		for (unsigned int i = 0; i < ARRAY_SIZE(_free_areas); i++) {
			char buffer[256];
			snprintf(buffer, sizeof(buffer), "[%d] ", i);
						
			// Iterate over each block in the free area.
			PageDescriptor *pg = _free_areas[i];
			while (pg) {
				// Append the PFN of the free block to the output buffer.
				snprintf(buffer, sizeof(buffer), "%s%lx ", buffer, sys.mm().pgalloc().pgd_to_pfn(pg));
				pg = pg->next_free;
			}
			
			mm_log.messagef(LogLevel::DEBUG, "%s", buffer);
		}
	}

	
private:
	PageDescriptor *_free_areas[MAX_ORDER];
};

/* --- DO NOT CHANGE ANYTHING BELOW THIS LINE --- */

/*
 * Allocation algorithm registration framework
 */
RegisterPageAllocator(BuddyPageAllocator);
