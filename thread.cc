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



class mutex::impl{
// private:
//     bool busy;
//     std::queue<ucontext_t> waiting_mutex;

public:
 	impl()
 	{
 		busy = false;
 	};
 	~impl();

    bool busy;
    std::queue<ucontext_t> waiting_mutex;

};



void exec(thread_startfunc_t func, void* a){
	cout<<"exec start"<<endl;

	MainThreadUp = true;

	func(a);
	if(!running_list.empty()){
		cout<< " run done " <<endl;
		ucontext_t curt = running_list.front();

		running_list.pop();
		finish_list.push(curt);
	}
	cout << ready_list.size()<<endl;
	if(!ready_list.empty()){
		setcontext(&mt);
	}
	cout<<"exec end"<<endl;
}

thread::thread(thread_startfunc_t func, void* a){
	// cpu::interrupt_disable();
	ucontext_t t;
	getcontext(&t);
	cout<<"before initial thread create address: "<<t.uc_stack.ss_sp<<endl;
	t.uc_link=nullptr;
	t.uc_stack.ss_sp=malloc(STACK_SIZE);
	t.uc_stack.ss_size=STACK_SIZE;
	t.uc_stack.ss_flags=0;
	makecontext(&t, (void (*)()) exec, 2,  func, a);		//cannot pass whole a.
	cout<<"thread create address: "<<t.uc_stack.ss_sp<<endl;



	if(!running_list.empty()){
		ucontext_t curt;
		getcontext(&curt);
		ready_list.push(curt);

		running_list.pop();
		running_list.push(t);
	}else{
		running_list.push(t);
	}
	swapcontext(&ready_list.back(), &t);

	cout<<"end of created"<<endl;
	// cpu::interrupt_enable();
}



thread::~thread(){
	// cpu::interrupt_disable();
	cout<<"kill thread obj"<<endl;
}





void thread::yield(){

	ucontext_t next = ready_list.front();
	ready_list.pop();

	if(!running_list.empty()){     //remove current thread
		running_list.pop();

		ucontext_t curt;
		getcontext(&curt);
		ready_list.push(curt);

		running_list.push(next);
		swapcontext(&ready_list.back(), &next);
	}else{
		setcontext(&next);
	}

}




void Timer_handle(int sig){
	cout<<"in timer"<<endl;

	if(MainThreadUp){
		ucontext_t curt;
		getcontext(&curt);
		ready_list.push(curt);
		swapcontext(&ready_list.back(),&mt);		
	}

	cout<<"timer return"<<endl;
}





void cpu::init(thread_startfunc_t func, void * a){

	interrupt_vector_table[cpu::TIMER] = (interrupt_handler_t) Timer_handle;

	ucontext_t mf;
	getcontext(&mf);
	interrupt_enable();
	mf.uc_link = 0;
	mf.uc_stack.ss_sp = malloc(STACK_SIZE);
	mf.uc_stack.ss_size = STACK_SIZE;
	mf.uc_stack.ss_flags = 0;

	makecontext(&mf, (void (*)()) exec, 2,  func, a);

	running_list.push(mf);

	swapcontext(&mt, &mf);

	cout << "go back to Main thread" <<endl;
	while(!ready_list.empty() | !running_list.empty()){
		if(!ready_list.empty()){
			thread::yield();
		}
	}
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
	if(impl_ptr->busy){

		running_list.pop();
		ucontext_t curt;
		getcontext(&curt);
		impl_ptr->waiting_mutex.push(curt);

		if (!ready_list.empty()){
			ucontext_t next = ready_list.front();
			ready_list.pop();
			running_list.push(next);
			// setcontext(&next);
			cpu::interrupt_enable();
			swapcontext(&(impl_ptr->waiting_mutex.back()),&next);
		}else{
			//assert ? something wrong
			cpu::interrupt_enable();
		}
	}else{
		impl_ptr->busy = true;
		cpu::interrupt_enable();
	}

	
}


void mutex::unlock(){
	cpu::interrupt_disable();

	if(!impl_ptr->waiting_mutex.empty()){
		//move one TCB from waiting to ready
		ready_list.push(impl_ptr->waiting_mutex.front());
		impl_ptr->waiting_mutex.pop();
	}else{
		impl_ptr->busy = false;
	}
	cpu::interrupt_enable();
}


void cv::wait(mutex&){


}

void cv::signal(){


}

void cv::broadcast(){

}





