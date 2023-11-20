/*
 File: scheduler.C
 
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

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "simple_timer.H"

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
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler::Scheduler() {
  /*Initialize the queue objects*/
  head = NULL;
  tail = NULL;
  queue_size = 0;
  
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
  Console::puts("Yield called.\n");

  /*The next thread that will be executing will be the first one in the queue*/
  Thread * next_thread = dequeue();

  Console::puts("Dispatching control to thread : ");
  Console::puti(next_thread->ThreadId());
  Console::puts("\n");
  /*Get the latest head and then dispatch the control to that thread*/
  Thread::dispatch_to(next_thread);
}

void Scheduler::resume(Thread * _thread) {
  /*Get a thread and put it to the end of the ready queue*/
  enqueue(_thread);

  Console::puts("Resuming thread : ");
  Console::puti(_thread->ThreadId());
  Console::puts("\n");
}

void Scheduler::add(Thread * _thread) {
  enqueue(_thread);
}

void Scheduler::terminate(Thread * _thread) {
  Console::puts("Thread terminate called.\n");
  /*Delete the thread and dispath the control to the next one in the queue*/
  delete _thread;
  yield();
}

/*Scheduler queue functions*/
void Scheduler::enqueue(Thread * _thread){
  scheduler_queue * new_entry = new scheduler_queue;
  new_entry->thread = _thread;
  new_entry->next = NULL;

  if(isQueueEmpty()){
    head = new_entry;
    tail = head;
  }else{
    tail->next = new_entry;
    tail = tail->next;
  }
  queue_size++;
}

Thread * Scheduler::dequeue(){
  if(isQueueEmpty()){
    Console::puts("Queue is empty. No threads available to execute\n");
    return NULL;
  }

  Thread * current_head = head->thread;
  head = head->next;

  if(head == NULL){
    tail = head;
  }
  queue_size--;
  return current_head;
}

bool Scheduler::isQueueEmpty(){
  return queue_size == 0;
}