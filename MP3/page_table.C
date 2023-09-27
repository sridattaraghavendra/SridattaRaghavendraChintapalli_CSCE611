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
   /*Get the page directory page from the kernel frame pool*/
   unsigned long page_directory_number = kernel_mem_pool->get_frames(1);
   page_directory = (unsigned long *)(page_directory_number * PAGE_SIZE);

   /*Get one more page table from kernel frame pool for mapping the
          Direct memory till 4MB*/
   /*To map the first 4MB we just need to have 1024 entries since there are 1024 pages till 4MB*/
   unsigned long page_table_number = kernel_mem_pool->get_frames(1);
   unsigned long *page_table = (unsigned long *)(page_table_number * PAGE_SIZE);

   /*Assign the address of the page table page to the first page directory entry*/
   page_directory[0] = ((unsigned long)page_table) | page_entry_valid_status;

   /*Mark rest of the page directory entries as invalid*/
   for(int page_directory_index = 1; page_directory_index < 1024;page_directory_index++){
      page_directory[page_directory_index] |= page_entry_not_valid_status;
   }

   /*Start address is 0 and we increment this address by the page size which is 4K*/
   unsigned long page_address = 0;

   /*Initialize the 1024 frame addresses in the page table page*/
   for(int page_table_index = 0; page_table_index < shared_size / PAGE_SIZE; page_table_index++){
     page_table[page_table_index] = page_address | page_entry_valid_status;
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

void PageTable::handle_fault(REGS * _r)
{
  unsigned long fault_address = read_cr2();
  unsigned long * page_directory_address = (unsigned long *)read_cr3();
  
  /*Extract the page directory index from the fault address
    we know that in x86 the first 10 bits indicate the page directory index, we have to extract that from the fault address*/
  unsigned long page_directory_index = fault_address >> 22;

  /*Extract the page table index from the fault address
    we know that in x86 the second 10 bits indicate the page table index*/
  unsigned long page_table_index = fault_address >> 12 & 0x3FF; //The 0x3FF mask will extract the page table index only

  /*Pointer to the page table in case if we have to create a page table, else this will be
   filled with the page table address from the page directory index*/
  unsigned long *page_table;


  if(_r->err_code & 0xFFFFFFFE){ //Extract the last bit of the error code to check for page fault error
        /*First we will check if the page directory index is invalid if so we have
          to create a new page table page and store the page table page reference in this index*/
        if((page_directory_address[page_directory_index] & 0x1) == 0){
            page_table = (unsigned long *)(kernel_mem_pool->get_frames(1) * PAGE_SIZE);
            page_directory_address[page_directory_index] = ((unsigned long) page_table) | page_entry_valid_status;
        }

        /*When extracting the page table reference, apply filter mask to exclude the page status bits.
          else the page table reference address means something else and page fault will not be handled properly*/
        page_table = ((unsigned long *)(page_directory_address[page_directory_index] & 0xFFFFF000));

        /*Now we will check if the page table entry is invalid and proceed with assigning new 
          page frame*/
        if((page_table[page_table_index] & 0x1) == 0){
          /*Get a frame from process mem pool and assign it to the page table index*/
          unsigned long new_frame_number = process_mem_pool->get_frames(1);
          unsigned long *new_frame = (unsigned long *)(new_frame_number * PAGE_SIZE);
          page_table[page_table_index] = ((unsigned long)new_frame) | page_entry_valid_status;
        }
  }

  Console::puts("handled page fault\n");
}

