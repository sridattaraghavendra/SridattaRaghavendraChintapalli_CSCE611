/*
 File: vm_pool.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) {
    this->base_address = _base_address;
    this->size = _size;
    this->frame_pool = _frame_pool;
    this->page_table = _page_table;

    page_table->register_pool(this);

    pool_info *pool_locations = (pool_info *)base_address;
    pool_locations[0].start_address = _base_address;
    pool_locations[0].size = Machine::PAGE_SIZE;
    for(int index = 1; index < 512; index++){
        pool_locations[index].start_address = 0;
        pool_locations[index].size = 0;
    }

    Console::puts("Initialized pool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size) {
   /*If the required size is less than 4k, then allocate a page*/
   /*If the required size is a multiple of 4k then simply divide the size/4k and multiply with the page size*/
   /*If the required size is between 4k - 5k or 6k, and if the remainder is not 0 then assign a full page + quotiet * Page_size*/
   unsigned long size_to_be_allocated = _size <= Machine::PAGE_SIZE ? Machine::PAGE_SIZE : (_size/Machine::PAGE_SIZE) * Machine::PAGE_SIZE + (_size%Machine::PAGE_SIZE == 0 ? 0 : Machine::PAGE_SIZE);

   Console::puts("Determined size to be allocated as : ");
   Console::puti(size_to_be_allocated);
   Console::puts("\n");

   pool_info * pool_locations = (pool_info *)base_address;

   int index;
   for(index = 1; index < 512; index++){
      if(pool_locations[index].start_address == 0 && pool_locations[index].size == 0){
         pool_locations[index].start_address = pool_locations[index - 1].start_address + pool_locations[index - 1].size;
         pool_locations[index].size = size_to_be_allocated;
         break;
      }
   }

   Console::puts("Start address : ");
   Console::puti(pool_locations[index].start_address);
   Console::puts("\n");

   return pool_locations[index].start_address;
}

void VMPool::release(unsigned long _start_address) {
    Console::puts("Received request to release : ");
    Console::puti(_start_address);
    Console::puts("\n");

    /*Find out the start address of the region in the pool_locations*/
    int index;
    bool location_found;
    pool_info * pool_locations = (pool_info *)base_address;
    for(index = 1; index < 512; index++){
        Console::puts("Address for pool : ");
        Console::puti(pool_locations[index].start_address);
        Console::puts("\n");
        if(pool_locations[index].start_address == _start_address){
            location_found = true;
            break;
        }
    }

    if(!location_found){
        Console::puts("Error : Cannot find location to be freed.\n");
        assert(false);
    }

    /*Based on the size of the allocated memory, free the memory in the increments of page size
      Address of the page = start_address+pagesize etc...*/
    for(int size = 0;size < pool_locations[index].size; size += Machine::PAGE_SIZE){
        page_table->free_page(pool_locations[index].start_address + size);
    }

    /*Finally shift the pool_locations array*/
    for(;index<511;index++){
        if(pool_locations[index + 1].size > 0){
            pool_locations[index].start_address = pool_locations[index + 1].start_address;
            pool_locations[index].size = pool_locations[index + 1].size;
        }else{
            /*Reset the values*/
            pool_locations[index].start_address = 0;
            pool_locations[index].size = 0;
        }
    }
}

bool VMPool::is_legitimate(unsigned long _address) {
    /*The first frame will be used for storing the vm_pool's startaddress and size, 
        if we remove this line, the system will forever be page faulting*/
    if(_address == base_address){
        return true;
    }

    pool_info * pool_locations = (pool_info *)base_address;
    for(int index=0;index < 512;index++){
        if((_address >= pool_locations[index].start_address) && (_address <= pool_locations[index].start_address + pool_locations[index].size)){
            return true;
        }
    }
    return false;
}

