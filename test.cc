#include <iostream>
#include <cstdlib>

#include "thread.h"

using namespace std;

int N = 300;
// mutex m1;

void fn3(){
	cout << "start of fn3"<<endl;
	// thread::yield();
	for(int i=0;i < N;i++){
		// cout<<"fn3 running "<<i<<endl;
		cout<<"fn3 running "<<0.1*i<<endl;
	}
	cout << "end of fn3" <<endl;
}


void fn2(){
	cout << "start of fn2"<<endl;
	// thread::yield();
	for(int i=0;i < N;i++){
		// cout<<"fn2 running "<<i<<endl;
		cout<<"fn2 running "<<-i<<endl;
	}
	cout << "end of fn2" <<endl;
}



int fn1(){
	// m1.lock();
	cout << "start of fn1"<<endl;
	// m1.unlock();
	
	// cout << "second of fn1"<<endl;
	// thread::yield();
	for(int i=0; i < N;i++) {
		//cout<<"fn1 running "<<i<<endl;
		 cout<<"fn1 running "<<i<<endl;
	}
	cout << "end of fn1"<<endl;
	return 0;
}



void parent(void *a)
{

	// while(1);
	cout << "Parent start " << endl;
    	thread t1 ( (thread_startfunc_t) fn1, (void *)1);
	thread t2 ( (thread_startfunc_t) fn2, (void *)2);
	thread t3 ( (thread_startfunc_t) fn3, (void *)3);
    	cout << "Parent finish " <<endl;
	// while(1) thread:yield();
    return;
}





int main()
{
	cout << "main start################################################################################################" <<endl;
    cpu::boot(1, (thread_startfunc_t) parent, (void *) 100, true, true, 0);
    cout << "main end" <<endl;

    // thread t1 ( (thread_startfunc_t) loop, (void *) "child thread");
}
