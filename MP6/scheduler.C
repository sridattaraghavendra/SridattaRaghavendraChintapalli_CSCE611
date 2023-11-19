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
  queue = new Thread*[MAX_QUEUE_SIZE];
  head = -1;
  tail = -1;
  
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
  Console::puts("Yield called.\n");
  if(Machine::interrupts_enabled()){
    Machine::disable_interrupts();
  }

  /*The next thread that will be executing will be the first one in the queue*/
  Thread * next_thread = dequeue();

  Console::puts("Dispatching control to thread : ");
  Console::puti(next_thread->ThreadId());
  Console::puts("\n");
  /*Get the latest head and then dispatch the control to that thread*/
  Thread::dispatch_to(next_thread);

  if(!Machine::interrupts_enabled()){
    Machine::enable_interrupts();
  }
}

void Scheduler::resume(Thread * _thread) {
  if(Machine::interrupts_enabled()){
    Machine::disable_interrupts();
  }
  /*Get a thread and put it to the end of the ready queue*/
  enqueue(_thread);

  Console::puts("Resuming thread : ");
  Console::puti(_thread->ThreadId());
  Console::puts("\n");

  if(!Machine::interrupts_enabled()){
    Machine::enable_interrupts();
  }
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
  if(isQueueFull()){
    Console::puts("Queue is full, cannot accept any more threads.\n");
    assert(false);
  } else if(isQueueEmpty()){
    head = 0;
    tail = 0;
    queue[tail] = _thread;
  }else if(tail == MAX_QUEUE_SIZE - 1 && head != 0){
    tail = 0;
    queue[tail] = _thread;
  }else{
    tail++;
    queue[tail] = _thread;
  }
}

Thread * Scheduler::dequeue(){
  if(isQueueEmpty()){
    Console::puts("Queue is empty. No threads available to execute\n");
    assert(false);
  }

  Thread * thread = queue[head];
  queue[head] = NULL;
  if(head == tail){
    head = -1;
    tail = -1;
  }else if(head == MAX_QUEUE_SIZE - 1){
    head = 0;
  }else{
    head++;
  }

  return thread;
}

bool Scheduler::isQueueEmpty(){
  return head == -1;
}

bool Scheduler::isQueueFull(){
  return ((head == 0 && tail == MAX_QUEUE_SIZE - 1) || ((tail + 1)%MAX_QUEUE_SIZE == head));
}


/*Round-Robin scheduler*/
RRScheduler::RRScheduler(unsigned _end_of_quantum) {
  /*Initialize the queue objects*/
  queue = new Thread*[MAX_QUEUE_SIZE];
  head = -1;
  tail = -1;

  /*For 50ms time quantum the frequency in Hz that has to be set is 20*/
  EOQTimer * timer = new EOQTimer(1000/_end_of_quantum);
  InterruptHandler::register_handler(0,timer);


  Console::puts("Constructed Round robin Scheduler.\n");
}

void RRScheduler::yield() {
  Console::puts("Yield called.\n");
  if(Machine::interrupts_enabled()){
    Machine::disable_interrupts();
  }

  /*The next thread that will be executing will be the first one in the queue*/
  Thread * next_thread = dequeue();

  Console::puts("Dispatching control to thread : ");
  Console::puti(next_thread->ThreadId());
  Console::puts("\n");
  /*Get the latest head and then dispatch the control to that thread*/
  Thread::dispatch_to(next_thread);
  

  if(!Machine::interrupts_enabled()){
    Machine::enable_interrupts();
  }
}

void RRScheduler::resume(Thread * _thread) {
  if(Machine::interrupts_enabled()){
    Machine::disable_interrupts();
  }
  /*Get a thread and put it to the end of the ready queue*/
  enqueue(_thread);

  Console::puts("Resuming thread : ");
  Console::puti(_thread->ThreadId());
  Console::puts("\n");

  if(!Machine::interrupts_enabled()){
    Machine::enable_interrupts();
  }
}

void RRScheduler::add(Thread * _thread) {
  enqueue(_thread);
}

void RRScheduler::terminate(Thread * _thread) {
  Console::puts("Thread terminate called.\n");
  /*Delete the thread and dispath the control to the next one in the queue*/
  delete _thread;
  yield();
}

/*Scheduler queue functions*/
void RRScheduler::enqueue(Thread * _thread){
  if(isQueueFull()){
    Console::puts("Queue is full, cannot accept any more threads.\n");
    assert(false);
  } else if(isQueueEmpty()){
    head = 0;
    tail = 0;
    queue[tail] = _thread;
  }else if(tail == MAX_QUEUE_SIZE - 1 && head != 0){
    tail = 0;
    queue[tail] = _thread;
  }else{
    tail++;
    queue[tail] = _thread;
  }
}

Thread * RRScheduler::dequeue(){
  if(isQueueEmpty()){
    Console::puts("Queue is empty. No threads available to execute\n");
    assert(false);
  }

  Thread * thread = queue[head];
  queue[head] = NULL;
  if(head == tail){
    head = -1;
    tail = -1;
  }else if(head == MAX_QUEUE_SIZE - 1){
    head = 0;
  }else{
    head++;
  }
  return thread;
}

bool RRScheduler::isQueueEmpty(){
  return head == -1;
}

bool RRScheduler::isQueueFull(){
  return ((head == 0 && tail == MAX_QUEUE_SIZE - 1) || ((tail + 1)%MAX_QUEUE_SIZE == head));
}
