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
  assert(false);
}

void Scheduler::resume(Thread * _thread) {
  assert(false);
}

void Scheduler::add(Thread * _thread) {
  enqueue(_thread);
}

void Scheduler::terminate(Thread * _thread) {
  assert(false);
}

/*Scheduler queue functions*/
void Scheduler::enqueue(Thread * _thread){
  if(isQueueFull()){
    Console::puts("Queue is full, cannot accept any more threads.\n");
    assert(false);
  }

  if(isQueueEmpty()){
    head = 0;
  }

  tail++;
  queue[tail] = _thread;
}

Thread * Scheduler::dequeue(){
  if(isQueueEmpty()){
    Console::puts("Queue is empty. No threads available to execute\n");
    assert(false);
  }

  Thread * thread = queue[head];

  if(head == tail){
    head = -1;
    tail = -1;
  }else{
    head++;
  }

  return thread;
}

bool Scheduler::isQueueEmpty(){
  return head == -1;
}

bool Scheduler::isQueueFull(){
  return tail == MAX_QUEUE_SIZE - 1;
}