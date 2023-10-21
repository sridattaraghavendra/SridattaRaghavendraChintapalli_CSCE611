#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;
const unsigned int page_entry_valid_status = 3;
const unsigned int page_entry_not_valid_status = 2;
VMPool * PageTable::vm_pools[];


void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
  kernel_mem_pool = _kernel_mem_pool;
  process_mem_pool = _process_mem_pool;
  shared_size = _shared_size;
}

PageTable::PageTable()
{
   /*Get the page directory page from the process frame pool*/
   unsigned long page_directory_number = process_mem_pool->get_frames(1);
   page_directory = (unsigned long *)(page_directory_number * PAGE_SIZE);

   /*Get one more page table from kernel frame pool for mapping the
          Direct memory till 4MB*/
   /*To map the first 4MB we just need to have 1024 entries since there are 1024 pages till 4MB*/
   unsigned long page_table_number = process_mem_pool->get_frames(1);
   unsigned long *page_table_page = (unsigned long *)(page_table_number * PAGE_SIZE);

   /*Assign the address of the page table page to the first page directory entry*/
   page_directory[0] = ((unsigned long)page_table_page) | page_entry_valid_status;

   /*Mark rest of the page directory entries as invalid*/
   for(int page_directory_index = 1; page_directory_index < 1024;page_directory_index++){
      page_directory[page_directory_index] |= page_entry_not_valid_status;
   }

   /*Point the page table end to point to the page directory address*/
   page_directory[1023] = ((unsigned long)page_directory) | page_entry_valid_status;

   /*Start address is 0 and we increment this address by the page size which is 4K*/
   unsigned long page_address = 0;

   /*Initialize the 1024 frame addresses in the page table page*/
   for(int page_table_index = 0; page_table_index < shared_size / PAGE_SIZE; page_table_index++){
     page_table_page[page_table_index] = page_address | page_entry_valid_status;
     page_address = page_address + PAGE_SIZE;
   }

   Console::puts("Page table setup successfully.\n");
}


void PageTable::load()
{
   current_page_table = this;
   write_cr3((unsigned long)page_directory);
   Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
   write_cr0(read_cr0() | 0x1 << 31);
   paging_enabled = true;
   Console::puts("Enabled paging\n");
}

unsigned long * PageTable::PDE_address(unsigned long addr){
  /*For getting the right page directory entry*/
  /*As per the following recursive page table look up | 1023 | 1023 | x | 00 */
  /*Fixed logical start address of the page directory entry can there fore be 0xFFFFF000*/
  /*The page fault address has to be shifted by 20 bits to the right to get the 10 bit index into the page table*/
  return (unsigned long *)((0xFFFFF000 | addr >> 20) & 0xFFFFFFFC);
}


unsigned long * PageTable::PTE_address(unsigned long addr){
  /*For getting the right page table entry*/
  /*As per the following recursive page table look up | 1023 | x | y | 00 */
  /*Fixed logical start address of the page table entry can there fore be 0xFFC00000*/
  /*The page fault address has to be shifted by 10 bits to the right to get the 10 bit X value and mask with 0x003FF000*/
  /*The page fault address has to be shifted by 10 bits to the right to get the 10 bit Y value and mask with 0x00000FFC*/
  return (unsigned long *)((0xFFC00000 | (((addr >> 10) & 0x003FF000) | ((addr >> 10) & 0x00000FFC))) & 0xFFFFFFFC);
}

void PageTable::handle_fault(REGS * _r)
{
  
  unsigned long fault_address = read_cr2();
  unsigned long * page_directory_address = (unsigned long *)read_cr3();
  unsigned long * page_directory_entry;
  unsigned long * page_table_entry;
  unsigned int vm_pool_entry_count = 0;
  bool isValid = false;

  /*Determine the page directory index entry*/
  page_directory_entry = PDE_address(fault_address);

  /*Determine the page table index entry*/
  page_table_entry = PTE_address(fault_address);

  /*Pointer to the page table in case if we have to create a page table, else this will be
   filled with the page table address from the page directory index*/
  unsigned long *page_table;

  for(int index = 0; index < 512; index++){
    if(vm_pools[index] != 0){
      vm_pool_entry_count++;
    }
  }

  Console::puts("VM Pool entry count : ");
  Console::puti(vm_pool_entry_count);
  Console::puts("\n");

  for(int index=0; index < vm_pool_entry_count; index++){
    if(vm_pools[index]->is_legitimate(fault_address) == true){
      isValid = true;
      break;
    }
  }

  if(!isValid){
    Console::puts("Invalid memory reference\n");
    assert(false);
  }

  if(_r->err_code ^ 0x1){ //Extract the last bit value bit 0 which represents the page not found if bit 0 is 0 then we handle the exception if present bit is 1 then we don't handle the exception.
        /*First we will check if the page directory index is invalid if so we have
          to create a new page table page and store the page table page reference in this index*/
        if((*page_directory_entry & 0x1) == 0){
            page_table = (unsigned long *)(process_mem_pool->get_frames(1) * PAGE_SIZE);
            *page_directory_entry = ((unsigned long) page_table) | page_entry_valid_status;
        }

        /*Now we will check if the page table entry is invalid and proceed with assigning new 
          page frame*/
        if((*page_table_entry & 0x1) == 0){
          /*Get a frame from process mem pool and assign it to the page table index*/
          unsigned long new_frame_number = process_mem_pool->get_frames(1);
          unsigned long *new_frame = (unsigned long *)(new_frame_number * PAGE_SIZE);
          *page_table_entry = ((unsigned long)new_frame) | page_entry_valid_status;
        }
  }

  Console::puts("handled page fault\n");
}

void PageTable::register_pool(VMPool * _vm_pool)
{
   int index = 0;
   for(index = 0; index < 512; index++){
      if(vm_pools[index] == 0){
        break;
      }
   }
   vm_pools[index] = _vm_pool;
   Console::puts("Registered vm pool.\n");
}

void PageTable::free_page(unsigned long _page_no) {
   /*Convert page table number to page table entry*/
   unsigned long * page_table_entry = (unsigned long *)((0xFFC00000 | (((_page_no >> 10) & 0x003FF000) | ((_page_no >> 10) & 0x00000FFC))) & 0xFFFFFFFC);
   unsigned long frame_number = *page_table_entry >> 12;
   if((*page_table_entry & 0x1)){
      process_mem_pool->release_frames(frame_number);
      *page_table_entry = 0 | page_entry_not_valid_status; 
   }
   write_cr3((unsigned long)page_directory);
   Console::puts("Free page.\n");
}