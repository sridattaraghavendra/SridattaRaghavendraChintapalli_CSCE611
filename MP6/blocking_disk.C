/*
     File        : blocking_disk.c

     Author      : 
     Modified    : 

     Description : 

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "blocking_disk.H"
#include "scheduler.H"
#include "thread.H"

extern Scheduler * SYSTEM_SCHEDULER;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size) 
  : SimpleDisk(_disk_id, _size) {
    /*Initialize a queue for this blocking disk*/
  head = NULL;
  tail = NULL;
}

/*Override the wait until ready functionality*/
void BlockingDisk::wait_until_ready(){
  /*If the disk is not ready, then put the current thread in the queue*/
  while (!is_ready())
  {
    Console::puts("Disk is not ready, yield till the disk is ready.\n");
    //enqueue(Thread::CurrentThread());
    SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
    SYSTEM_SCHEDULER->yield(); /*Yield the CPU to the next thread in the queue*/
  }
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void BlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {
  SimpleDisk::read(_block_no,_buf);
}


void BlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {
  SimpleDisk::write(_block_no,_buf);
}

/*Enqueue function implementation*/
void BlockingDisk::enqueue(Thread * _thread){
  if(head == NULL || tail == NULL){
    disk_queue *new_entry = new disk_queue;
    new_entry->thread_id = _thread;
    head = tail = new_entry;
    new_entry->next = NULL;
  }else{
    disk_queue *new_entry = new disk_queue;
    new_entry->thread_id = _thread;
    tail->next = new_entry;
    tail = new_entry;
    new_entry->next = NULL;
  }
}

/*Dequeue function implementation*/
Thread * BlockingDisk::dequeue(){
  if(head != NULL){
    disk_queue *disk_queue_entry = head;
    Thread * next_thread_id = disk_queue_entry->thread_id;

    head = head->next;

    if(head == NULL){
      tail = NULL;
    }

    delete disk_queue_entry;
    return next_thread_id;
  }
}

/*Register interrupt handler for handling exception number 14*/
void BlockingDisk::handle_interrupt(REGS * _regs){
  Thread * next_in_queue = dequeue();
  Console::puts("Resuming thread with id : ");
  Console::puti(next_in_queue->ThreadId());
  Console::puts("\n");
  SYSTEM_SCHEDULER->resume(next_in_queue->CurrentThread());
}