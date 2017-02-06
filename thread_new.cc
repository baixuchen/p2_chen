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
#include <cassert>
using namespace std;

queue<ucontext_t*> ready_list;
queue<ucontext_t*> running_list;
queue<thread::impl*> finish_list;
queue<ucontext_t*> waiting_list;

// when cpu suspended, put its id into queue. 
//When need to active the cpu, using its id to call IPI.

queue<cpu*> suspended_list;
// vector<cpu*> cpu_list;

// ucontext_t *suspend_func = new ucontext_t;

int num_cpus = 0;
size_t active_cpus = 0;



class cv::impl {
public:
	impl() {

	};
	~impl();
	mutex::impl *mutex_ptr;
	std::queue<ucontext_t*> waiting_cv;
};

class mutex::impl {
public:
	impl() {
		busy = false;
	};
	~impl();

	bool busy;
	std::queue<ucontext_t*> waiting_mutex;
};

class thread::impl {
public:
	impl() {
		done = false;

		self = new ucontext_t;
		getcontext(self);
		self->uc_link = nullptr;
		self->uc_stack.ss_sp = malloc(STACK_SIZE);
		self->uc_stack.ss_size = STACK_SIZE;
		self->uc_stack.ss_flags = 0;
	};
	~impl() {
	};
	bool done;
	std::queue<ucontext_t*> waiting_join;
	ucontext_t *self;
};

void dumpy_func()
{
	while (true)
		cpu::interrupt_enable_suspend();
}

ucontext_t *dumpy_thread = new ucontext_t;
getcontext(dumpy_thread);
dumpy_thread->uc_link = nullptr;
dumpy_thread->uc_stack.ss_sp = malloc(STACK_SIZE);
dumpy_thread->uc_stack.ss_size = STACK_SIZE;
dumpy_thread->uc_stack.ss_flags = 0;
makecontext(dumpy_thread, (void(*)()) dumpy_func, 0);


void Interrupt_yield();
void delete_finish_job();



//timer_interrupt is stop during the interrupt, in this scope, timer interrupt can not be called again
void Timer_handle() {

	cpu::interrupt_disable();

	while (guard.exchange(true)) { _(cout << "stuck lock" << endl;) };

	delete_finish_job();

	_(cout << "Into Timer" << endl;)
		// if(!ready_list.empty()){
		// 	thread::yield();
		// } 
		// thread::yield();
		if (!ready_list.empty()) {
			Interrupt_yield();
		}
	_(cout << "End of Timer" << endl;)
		guard.store(false);
	cpu::interrupt_enable();
}

void IPI_handle() {
	cpu::interrupt_disable();
	while (guard.exchange(true)); {_(cout << "stuck lock" << endl;)};
	_(cout << "Into IPI " << endl;)

		// assert(ready_list.size() > 0);
		if (!ready_list.empty()) {
			ucontext_t *next = ready_list.front();
			ready_list.pop();
			running_list.push(next);
			_(cout << "CPU waking up : " << cpu::self() << "  alive cpus: " << active_cpus << endl;)
				setcontext(next);
			guard.store(false);
			cpu::interrupt_enable();
		}
		else {
			suspended_list.push(cpu::self());
			active_cpus--;
			_(cout << "CPU sleeping : " << cpu::self() << "  alive cpus: " << active_cpus << endl;)
				guard.store(false);
			cpu::interrupt_enable_suspend();
		}

}

void delete_finish_job() {
	while (!finish_list.empty()) {
		thread::impl *fj = finish_list.front();
		finish_list.pop();
		// cout<<endl<<"end5 fj sp: "<<(fj->self->uc_stack.ss_sp)<<endl;
		free(fj->self->uc_stack.ss_sp);
		delete fj->self;
		delete fj;
		// cout<<endl<<"end5 fj sp: "<<(fj->self->uc_stack.ss_sp)<<endl;
	}
}


void exec(thread_startfunc_t func, void* a, thread::impl* impl_ptr) {
	// cpu::interrupt_disable();
	_(cout << "Into Exec()" << endl;)
		guard.store(false);
	cpu::interrupt_enable();

	_(cout << "interrupt_enable exec" << endl;)
		func(a);


	// while(guard.exchange(true))	{_(cout<<"stuck lock"<<endl;)};
	cpu::interrupt_disable();
	while (guard.exchange(true)) { _(cout << "stuck lock" << endl;) };

	// if(!running_list.empty()){
	// 	finish_list.push(running_list.front());
	// 	// getcontext(&curt);
	// 	running_list.pop();
	// }

	assert(!running_list.empty());

	_(cout << "finish Exec()" << endl;)
		finish_list.push(impl_ptr);

	running_list.pop();



	// about join()
	impl_ptr->done = true;
	while (!impl_ptr->waiting_join.empty()) {
		_(cout << "func done, release locker on join() " << endl;)
			ready_list.push(impl_ptr->waiting_join.front());
		impl_ptr->waiting_join.pop();
	}

	// delete impl_ptr;

	_(cout << "exec: ready_list " << ready_list.size() << endl;)
		// if(ready_list.empty() && running_list.empty()){
		if (ready_list.empty()) {
			suspended_list.push(cpu::self());
			active_cpus--;
			guard.store(false);
			_(cout << "CPU sleeping : " << cpu::self() << num_cpus << endl;)
				cpu::interrupt_enable_suspend();
			// setcontext(suspend_func);
		}
		else {
			// jump to the ready, the curt will be dropped forever.
			ucontext_t *next = ready_list.front();
			ready_list.pop();
			running_list.push(next);
			setcontext(next);
		}

		assert(false);
		// call destructor?
		_(cout << "wodetian" << endl;)
}




thread::thread(thread_startfunc_t func, void* a) {
	// while(guard.exchange(true))	{_(cout<<"stuck lock"<<endl;)};
	cpu::interrupt_disable();
	while (guard.exchange(true)) { _(cout << "stuck lock" << endl;) };
	_(cout << "interrupt_disable thread()" << endl;)

		impl_ptr = new impl();

	makecontext(impl_ptr->self, (void(*)()) exec, 3, func, a, impl_ptr);		//cannot pass whole a.
	_(cout << "thread create address: " << impl_ptr->self->uc_stack.ss_sp << endl;)

		if (!running_list.empty()) {
			ucontext_t *curt = new ucontext_t;
			getcontext(curt);
			ready_list.push(curt);

			// delete running_list.front();
			running_list.pop();

			running_list.push(impl_ptr->self);
			swapcontext(ready_list.back(), impl_ptr->self);
		}
		else {
			running_list.push(impl_ptr->self);
			setcontext(impl_ptr->self);
		}
		guard.store(false);
		_(cout << "interrupt_enable thread()" << endl;)
			_(cout << "end of created" << endl;)
			cpu::interrupt_enable();
}



thread::~thread() {
	_(cout << "kill thread obj" << cpu::self() << endl;)
		// delete impl_ptr;
}


// void thread::happy(){
// 	cout<<"happy here"<<this->impl_ptr<<endl;
// }

void thread::join() {
	// new obj for waiting_join
	// move curt to waiting_join
	// while(guard.exchange(true))	{_(cout<<"stuck lock"<<endl;)};
	cpu::interrupt_disable();
	while (guard.exchange(true)) { _(cout << "stuck lock" << endl;) };
	_(cout << "interrupt_disable join()" << endl;)
		_(cout << "Into Join" << endl;)

		// //while(guard.exchange(true))	{_(cout<<"stuck lock"<<endl;)};
		running_list.pop();
	// assert(ready_list.size() > 0);
	if (ready_list.empty()) {
		suspended_list.push(cpu::self());
		active_cpus--;
		guard.store(false);
		_(cout << "CPU sleeping : " << cpu::self() << endl;)
			cpu::interrupt_enable_suspend();
	}
	else {
		ucontext_t *next = ready_list.front();
		running_list.push(next);
		ready_list.pop();

		ucontext_t *curt = new ucontext_t;
		getcontext(curt);
		impl_ptr->waiting_join.push(curt);
		swapcontext(impl_ptr->waiting_join.back(), next);
		guard.store(false);
		cpu::interrupt_enable();
	}

}



void thread::yield() {
	//any queue operation should be atomic
	//before context switch, interrupt should be disable
	//change nothing while no threads are avaiable
	// while(guard.exchange(true))	{_(cout<<"stuck lock"<<endl;)};
	cpu::interrupt_disable();
	while (guard.exchange(true)) { _(cout << "stuck lock" << endl;) };
	_(cout << "interrupt_disable yield()" << endl;)
		_(cout << "Into thread yield()" << endl;)


		assert(ready_list.size() > 0);
	_(cout << "ready_list size : " << ready_list.size() << "  " << active_cpus << endl;)
		if (ready_list.size()>active_cpus + 1 && !suspended_list.empty()) {
			suspended_list.front()->interrupt_send();
			suspended_list.pop();
			active_cpus++;
		}

	ucontext_t *next = ready_list.front();

	ready_list.pop();
	//before swapping thread, interrupt must be disable
	if (!running_list.empty()) {
		// delete running_list.front();
		running_list.pop();

		ucontext_t *curt = new ucontext_t;
		getcontext(curt);
		ready_list.push(curt);
		running_list.push(next);
		swapcontext(ready_list.back(), next);
	}
	else {
		running_list.push(next);
		setcontext(next);
	}
	_(cout << "End of yield()" << endl;)

		guard.store(false);
	cpu::interrupt_enable();
	//while(guard.exchange(true))	{_(cout<<"stuck lock"<<endl;)};
	_(cout << "interrupt_enable yield()" << endl;)
		//back from other thread. now the interrupt is disable, we got to enable here.
}




void Interrupt_yield() {
	//any queue operation should be atomic
	//before context switch, interrupt should be disable
	//change nothing while no threads are avaiable
	assert(ready_list.size() > 0);
	_(cout << "ready_list size : " << ready_list.size() << "  " << active_cpus << endl;)
		if (ready_list.size() >= active_cpus  && !suspended_list.empty()) {
			suspended_list.front()->interrupt_send();
			suspended_list.pop();
			active_cpus++;
		}

	ucontext_t *next = ready_list.front();
	ready_list.pop();
	//before swapping thread, interrupt must be disable
	if (!running_list.empty()) {
		// delete running_list.front();
		running_list.pop();

		ucontext_t *curt = new ucontext_t;
		getcontext(curt);
		ready_list.push(curt);
		running_list.push(next);
		swapcontext(ready_list.back(), next);
	}
	else {
		running_list.push(next);
		setcontext(next);
	}

	//back from other thread. now the interrupt is disable, we got to enable here.
}



void cpu::init(thread_startfunc_t func, void * a) {
	interrupt_vector_table[cpu::TIMER] = (interrupt_handler_t)Timer_handle;
	interrupt_vector_table[cpu::IPI] = (interrupt_handler_t)IPI_handle;


	_(cout << "interrupt_enable init()" << endl;)
		//swapcontext only in the interrupt
		if (func != 0) {
			num_cpus++;
			active_cpus++;

			// suspend_func = new ucontext_t;


			guard.store(false);
			cpu::interrupt_enable();
			thread(func, a);
		}
		else {
			num_cpus++;
			suspended_list.push(cpu::self());
			// cpu::interrupt_disable();
			guard.store(false);
			_(cout << "CPU sleeping : " << cpu::self() << endl;)
				cpu::interrupt_enable_suspend();
		}
}

mutex::mutex() {
	impl_ptr = new impl();
}

mutex::~mutex() {
	// free(waiting_mutex);
	// delete impl_ptr;
}



void mutex::lock() {
	// guard, interrupt
	cpu::interrupt_disable();
	while (guard.exchange(true));
	_(cout << "Into Lock()" << impl_ptr->waiting_mutex.size() << cpu::self() << endl);
	if (impl_ptr->busy) {
		// add thread to queue of threads waiting for lock
		// switch to next ready thread
		running_list.pop();
		ucontext_t *curt = new ucontext_t;
		impl_ptr->waiting_mutex.push(curt);
		// assert(ready_list.size()>0);
		if (ready_list.empty()) {
			suspended_list.push(cpu::self());
			active_cpus--;
			_(cout << "CPU sleeping : " << cpu::self() << endl;)
				guard.store(false);
			swapcontext(impl_ptr->waiting_mutex.back(), dumpy_thread);
			// cpu::interrupt_enable_suspend();			
		}
		else {
			ucontext_t *next = ready_list.front();
			ready_list.pop();
			running_list.push(next);
			swapcontext(impl_ptr->waiting_mutex.back(), next);
		}
	}
	else {
		impl_ptr->busy = true;
	}

	_(cout << "End of lock" << impl_ptr->waiting_mutex.size() << endl;)
		guard.store(false);
	cpu::interrupt_enable();
}

void mutex::unlock() {
	cpu::interrupt_disable();
	while (guard.exchange(true));
	_(cout << "Into unlock()" << impl_ptr->waiting_mutex.size() << endl;)

		impl_ptr->busy = false;
	if (!impl_ptr->waiting_mutex.empty()) {
		ready_list.push(impl_ptr->waiting_mutex.front());
		impl_ptr->waiting_mutex.pop();
		impl_ptr->busy = true;
	}
	_(cout << "End of unlock" << impl_ptr->waiting_mutex.size() << endl;)
		guard.store(false);
	cpu::interrupt_enable();
}



cv::cv() {
	impl_ptr = new impl();
}

cv::~cv() {

}

void cv::wait(mutex &m) {

	// cpu::interrupt_disable();
	// while(guard.exchange(true)){}
	// _(cout<<"Into wait"<<endl;)

	// // call unlock()
	// guard.store(false);
	// cpu::interrupt_enable();
	m.unlock();


	cpu::interrupt_disable();
	while (guard.exchange(true));
	_(cout << "Into wait" << endl;)

		running_list.pop();
	ucontext_t *curt = new ucontext_t;
	getcontext(curt);
	impl_ptr->waiting_cv.push(curt);

	// assert(ready_list.size()>0);
	if (ready_list.empty()) {
		ucontext_t *suspend_func = new ucontext_t;
		getcontext(suspend_func);
		suspend_func->uc_link = nullptr;
		suspend_func->uc_stack.ss_sp = malloc(STACK_SIZE);
		suspend_func->uc_stack.ss_size = STACK_SIZE;
		suspend_func->uc_stack.ss_flags = 0;
		makecontext(suspend_func, (void(*)()) cpu::interrupt_enable_suspend, 0);


		suspended_list.push(cpu::self());
		active_cpus--;
		_(cout << "CPU sleeping : " << cpu::self() << endl;)
			guard.store(false);
		swapcontext(impl_ptr->waiting_cv.back(), suspend_func);
	}
	else {
		ucontext_t *next = ready_list.front();
		ready_list.pop();
		running_list.push(next);
		swapcontext(impl_ptr->waiting_cv.back(), next);
	}

	guard.store(false);
	cpu::interrupt_enable();

	m.lock();
	_(cout << "End of wait()" << endl;)
}


void cv::signal() {

	cpu::interrupt_disable();
	while (guard.exchange(true));

	if (!impl_ptr->waiting_cv.empty()) {
		ready_list.push(impl_ptr->waiting_cv.front());
		impl_ptr->waiting_cv.pop();
	}

	guard.store(false);
	cpu::interrupt_enable();
}

void cv::broadcast() {

	cpu::interrupt_disable();
	while (guard.exchange(true));

	while (!impl_ptr->waiting_cv.empty()) {
		ready_list.push(impl_ptr->waiting_cv.front());
		impl_ptr->waiting_cv.pop();
	}

	guard.store(false);
	cpu::interrupt_enable();
}


// void mutex::lock(){
// 	// while(guard.exchange(true))	{_(cout<<"stuck lock"<<endl;)};
// 	cpu::interrupt_disable();
// 	while(guard.exchange(true))	{_(cout<<"stuck lock"<<endl;)};
// 	_(cout<<"interrupt_disable lock()"<<endl;)
// 	_(cout<<"Into Lock()"<<endl);
// 	//if lock is busy. store the current thread into lock waiting list. and get it out of the running list
// 	if (impl_ptr->busy){
// 		// assert(!ready_list.empty());

// 		running_list.pop();

// 		ucontext_t *curt = new ucontext_t;
// 		getcontext(curt);
// 		impl_ptr->waiting_mutex.push(curt);

// 		if(ready_list.empty()){
// 			suspended_list.push(cpu::self());
// 			active_cpus--;
// 			guard.store(false);
// 			_(cout<<"CPU sleeping : "<<cpu::self()<<endl;)
// 			cpu::interrupt_enable_suspend();			
// 		}else{
// 			ucontext_t *next = ready_list.front();
// 			ready_list.pop();
// 			running_list.push(next);
// 			swapcontext(impl_ptr->waiting_mutex.back(),next);			
// 		}
// 	}

// 	impl_ptr->busy = true;
// 	// while(guard.exchange(true))	{_(cout<<"stuck lock"<<endl;)};
// 	_(cout<<"End of Lock()"<<endl;)
// 	guard.store(false);
// 	cpu::interrupt_enable();	
// 	//while(guard.exchange(true))	{_(cout<<"stuck lock"<<endl;)};
// 	_(cout<<"interrupt_enable lock()"<<endl;)
// }


// void mutex::unlock(){
// 	// while(guard.exchange(true))	{_(cout<<"stuck lock"<<endl;)};
// 	cpu::interrupt_disable();
// 	while(guard.exchange(true))	{_(cout<<"stuck lock"<<endl;)};
// 	_(cout<<"interrupt_disable unlock()"<<endl;)
// 	_(cout<<"Into unlock()"<<endl;)
// 	// //while(guard.exchange(true)) {_(cout<<"stuck unlock"<<endl;)};
// 	// cout << "Into mutex unlock() " <<endl;
// 	//release the lock

// 	_(cout<<ready_list.size()<<endl;)
// 	if(!impl_ptr->waiting_mutex.empty()){
// 		//move one TCB from waiting to ready_list
// 		ready_list.push(impl_ptr->waiting_mutex.front());
// 		impl_ptr->waiting_mutex.pop();
// 	}else{
// 		impl_ptr->busy = false;
// 	}
// 	_(cout<<ready_list.size()<<endl;)
// 	// while(guard.exchange(true))	{_(cout<<"stuck lock"<<endl;)};
// 	_(cout<<"End of unlock()"<<endl;)
// 	guard.store(false);
// 	cpu::interrupt_enable();
// 	//while(guard.exchange(true))	{_(cout<<"stuck lock"<<endl;)};
// 	_(cout<<"interrupt_enable unlock()"<<endl;)
// }


// void cv::wait(mutex& m){
// 	// release mutex
// 	// move curt thread to waiting_cv
// 	// pop from waiting_mutex to ready list
// 	// After being woken, call lock()
// 	cpu::interrupt_disable();
// 	while(guard.exchange(true))	{_(cout<<"stuck lock"<<endl;)};
// 	_(cout<<"interrupt_disable wait()"<<endl;)
// 	_(cout <<"Into cv wait"<<endl;)

// 	impl_ptr->mutex_ptr = m.impl_ptr;
// 	ucontext_t *next = new ucontext_t;
// 	if(m.impl_ptr->waiting_mutex.empty()){
// 		m.impl_ptr->busy = false;
// 		assert(ready_list.size() > 0);
// 		next = ready_list.front();
// 		ready_list.pop();
// 	}else{
// 		// next = m.impl_ptr->waiting_mutex.front();
// 		next = ready_list.front();
// 		ready_list.push(m.impl_ptr->waiting_mutex.front());
// 		ready_list.pop();
// 		m.impl_ptr->waiting_mutex.pop();
// 	}

// 	// delete running_list.front();
// 	running_list.pop();
// 	running_list.push(next);

// 	ucontext_t *curt = new ucontext_t;
// 	getcontext(curt);
// 	impl_ptr->waiting_cv.push(curt);

// 	// ready_list.push(m.impl_ptr->waiting_mutex.front());
// 	swapcontext(impl_ptr->waiting_cv.back(),next);


// 	guard.store(false);
// 	cpu::interrupt_enable();
// 	_(cout<<"interrupt_enable wait()"<<endl;)
// }

// void cv::signal(){
// 	// while(guard.exchange(true))	{_(cout<<"stuck lock"<<endl;)};
// 	cpu::interrupt_disable();
// 	while(guard.exchange(true))	{_(cout<<"stuck lock"<<endl;)};
// 	_(cout<<"interrupt_disable signal()"<<endl;)
// 	_(cout <<"Into cv signal() "<<endl;)
// 	if (impl_ptr->waiting_cv.size() > 0){
// 		// //while(guard.exchange(true)) {_(cout<<"stuck unlock"<<endl;)};
// 		if(impl_ptr->mutex_ptr->busy){
// 			impl_ptr->mutex_ptr->waiting_mutex.push(impl_ptr->waiting_cv.front());
// 			impl_ptr->waiting_cv.pop();
// 		}else{
// 			impl_ptr->mutex_ptr->busy = true;
// 			ready_list.push(impl_ptr->waiting_cv.front());
// 			impl_ptr->waiting_cv.pop();
// 		}
// 		// while(guard.exchange(true))	{_(cout<<"stuck lock"<<endl;)};
// 	}
// 	_(cout <<"End of cv signal() "<<endl;)
// 	guard.store(false);
// 	cpu::interrupt_enable();
// 	//while(guard.exchange(true))	{_(cout<<"stuck lock"<<endl;)};
// 	_(cout<<"interrupt_enable signal()"<<endl;)
// }

// void cv::broadcast(){
// 	// move all from waiting_cv to ready_list
// 	// while(guard.exchange(true))	{_(cout<<"stuck lock"<<endl;)};
// 	cpu::interrupt_disable();
// 	while(guard.exchange(true))	{_(cout<<"stuck lock"<<endl;)};
// 	_(cout<<"interrupt_disable broadcast()"<<endl;)
// 	_(cout <<"Into broadcast()"<<endl;)

// 	while(impl_ptr->waiting_cv.size() > 0){
// 		if(impl_ptr->mutex_ptr->busy){
// 			impl_ptr->mutex_ptr->waiting_mutex.push(impl_ptr->waiting_cv.front());
// 			impl_ptr->waiting_cv.pop();
// 		}else{
// 			impl_ptr->mutex_ptr->busy = true;
// 			ready_list.push(impl_ptr->waiting_cv.front());
// 			impl_ptr->waiting_cv.pop();
// 		}
// 	}

// 	guard.store(false);
// 	cpu::interrupt_enable();

// 	_(cout<<"interrupt_enable"<<endl;)
// }