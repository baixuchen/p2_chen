#ifdef DEBUG
#define _(x) x
#else
#define _(x)  
#endif


#include "thread.h"
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <iostream>
#include <vector>
#include <queue>
#include <atomic>
#include <unistd.h>
#include <sys/time.h>
using namespace std;

queue<ucontext_t> ready_list;
queue<ucontext_t> running_list;
queue<ucontext_t> finish_list;
queue<ucontext_t> waiting_list;



ucontext_t mt;

bool MainThreadUp = false;
bool needEnableInterrupt = false;

class cv::impl{
public:
	impl(){

	};
	~impl();

	std::queue<ucontext_t> waiting_cv;
};


class mutex::impl{
public:
 	impl(){
 		busy = false;
 	};
 	~impl();

    bool busy;
    std::queue<ucontext_t> waiting_mutex;

};



void exec(thread_startfunc_t func, void* a){
	// cpu::interrupt_disable();

	cpu::interrupt_enable();
	MainThreadUp = true;

	func(a);
	while(guard.exchange(true));
	if(!running_list.empty()){
		finish_list.push(running_list.front());
		// getcontext(&curt);

		running_list.pop();
	}
	guard.store(false);
	while(1);
}

thread::thread(thread_startfunc_t func, void* a){
	// cpu::interrupt_disable();
	ucontext_t t;
	getcontext(&t);
	t.uc_link=nullptr;
	t.uc_stack.ss_sp=malloc(STACK_SIZE);
	t.uc_stack.ss_size=STACK_SIZE;
	t.uc_stack.ss_flags=0;
	makecontext(&t, (void (*)()) exec, 2,  func, a);		//cannot pass whole a.
	while(guard.exchange(true));
	ready_list.push(t);
	guard.store(false);
	// cout<<"end of created"<<endl;
}



thread::~thread(){
	// cpu::interrupt_disable();
	// cout<<"kill thread obj"<<endl;
}





void thread::yield(){
	//any queue operation should be atomic
	//before context switch, interrupt should be disable
	//change nothing while no threads are avaiable
	if(ready_list.empty()){
		return;
	}

	ucontext_t next = ready_list.front();

	while(guard.exchange(true));
	ready_list.pop();
	guard.store(false);
	//before swapping thread, interrupt must be disable

	if(!running_list.empty()){

		running_list.pop();
		ucontext_t curt;
		getcontext(&curt);
		while(guard.exchange(true));
		ready_list.push(curt);
		running_list.push(next);
		guard.store(false);
		swapcontext(&ready_list.back(), &next);
	}else{
		while(guard.exchange(true));
		running_list.push(next);
		guard.store(false);
		setcontext(&next);
	}
	//back from other thread. now the interrupt is disable, we got to enable here.
}


//timer_interrupt is stop during the interrupt, in this scope, timer interrupt can not be called again
void Timer_handle(int sig){
	cpu::interrupt_disable();
	// cout<<"in timer"<<endl;
	if(!ready_list.empty()) thread::yield();
	else{
		if(ready_list.empty() && running_list.empty()) cpu::interrupt_enable_suspend();
		// cout <<"do nothing" <<endl;
		// cout<<"stuck here"<<endl;
	}
	// cout<<"out timer"<<endl;
	cpu::interrupt_enable();

}

void cpu::init(thread_startfunc_t func, void * a){

	interrupt_vector_table[cpu::TIMER] = (interrupt_handler_t) Timer_handle;

	//swapcontext only in the interrupt
	if(func!=0)
		thread(func, a);
	cpu::interrupt_enable();

	//loop until all job finished
	while(!ready_list.empty() || !running_list.empty());
	cpu::interrupt_disable();
	cpu::interrupt_enable_suspend();

	cout << "This is the end of mt(Main thread) " << endl;
}

mutex::mutex(){
	impl_ptr = new impl();
}
	
mutex::~mutex(){
	// free(waiting_mutex);
	// delete impl_ptr;
}



void mutex::lock(){
	cpu::interrupt_disable();
	//use guard to avoid mutual exclusion within multiple processors
	// cout<<"in lock"<<endl;
	while(guard.exchange(true))	cout<<"stuck lock"<<endl;;
	
	//if lock is free: acquire the lock and return to the 
	if(!impl_ptr->busy)	impl_ptr->busy = true;
	//if lock is busy. store the current thread into lock waiting list. and get it out of the running list
	else{
		if (!ready_list.empty()){
			// cout<<"lock swap"<<endl;
			running_list.pop();
			ucontext_t curt;
			getcontext(&curt);
			impl_ptr->waiting_mutex.push(curt);
			ucontext_t next = ready_list.front();
			ready_list.pop();
			running_list.push(next);
			guard.store(false);
			swapcontext(&curt,&next);
			// cout<<"back to lock"<<endl;
		}else{
			//assert ? something wrong
			guard.store(false);
			cpu::interrupt_enable();
			while(1);
		}
	}
	guard.store(false);
	
	cpu::interrupt_enable();
	
}


void mutex::unlock(){
	cpu::interrupt_disable();
	while(guard.exchange(true)) cout<<"stuck unlock"<<endl;
	// cout << "Into mutex unlock() " <<endl;
	//release the lock
	impl_ptr->busy = false;

	if(!impl_ptr->waiting_mutex.empty()){
		//move one TCB from waiting to ready_list
		ready_list.push(impl_ptr->waiting_mutex.front());
		impl_ptr->waiting_mutex.pop();
	}
	guard.store(false);
	cpu::interrupt_enable();
}


cv::cv(){
	impl_ptr = new impl();
}

cv::~cv(){

}

void cv::wait(mutex& m){
	// release mutex
	// move curt thread to waiting_cv
	// pop from waiting_mutex to ready list

	cpu::interrupt_disable();
	cout <<"Into cv wait"<<endl;
	m.impl_ptr->busy = false;

	running_list.pop();
	ucontext_t curt;
	getcontext(&curt);
	impl_ptr->waiting_cv.push(curt);
	
	ready_list.push(m.impl_ptr->waiting_mutex.front());
	m.impl_ptr->waiting_mutex.pop();

	needEnableInterrupt = true;
	// swapcontext(&impl_ptr->waiting_cv.back(),&(m.impl_ptr->waiting_mutex.front()));
	swapcontext(&impl_ptr->waiting_cv.back(),&mt);
}

void cv::signal(){
	cpu::interrupt_disable();
	cout <<"Into cv signal() "<<endl;
	if (impl_ptr->waiting_cv.size() > 0){
		ready_list.push(impl_ptr->waiting_cv.front());
		impl_ptr->waiting_cv.pop();
	}	
	cout <<"End of cv signal() "<<endl;
	cpu::interrupt_enable();
}

void cv::broadcast(){

}




