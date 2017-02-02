#include "thread.h"
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <iostream>

#define MEM 64000



using namespace std;

// int cpu::init(thread_startfunc_t, void * a){
//     return -1;
// }


ucontext_t T1, T2,Main;
ucontext_t a;
ucontext_t t;

int fn1()
{
 printf("this is from 1\n");
 // setcontext(&Main);
 cout<< T1.uc_stack.ss_sp << " " << a.uc_stack.ss_sp <<endl;
 // setcontext(&a);
 // swapcontext(&T1, &a);
}

void fn2()
{
 printf("this is from 2\n");
 setcontext( &a);
 printf("finished 1\n");
}

void fn3(){
	 // cout<< T1.uc_stack.ss_sp << " " << Main.uc_stack.ss_sp <<endl;


	 // setcontext(&Main);
	getcontext(&t);

	// cout<< T1.uc_stack.ss_sp << " " << t.uc_stack.ss_sp <<endl;
	getcontext(&a);
	// setcontext(&t);		
	t.uc_link=0;
	t.uc_stack.ss_sp=malloc(STACK_SIZE);
	t.uc_stack.ss_size=STACK_SIZE;
	t.uc_stack.ss_flags=0;
	cout<< T1.uc_stack.ss_sp << " " << t.uc_stack.ss_sp <<endl;	
	makecontext(&t, (void (*)())fn1, 0);
	cout<< T1.uc_stack.ss_sp << " " << t.uc_stack.ss_sp <<endl;
	// printf("this is from 3\n");
	swapcontext(&a, &t);
	// setcontext(&t);

	cout<< T1.uc_stack.ss_sp << " " << a.uc_stack.ss_sp <<endl;
	cout << "the end of fn3 " <<endl;

}




int main(int argc, char *argv[])
{
 // start();
 getcontext(&Main);


 getcontext(&T1);
 cout<< T1.uc_stack.ss_sp << " " << Main.uc_stack.ss_sp <<endl;

 T1.uc_link=0;
 T1.uc_stack.ss_sp=malloc(MEM);
 T1.uc_stack.ss_size=MEM;
 T1.uc_stack.ss_flags=0;
 makecontext(&T1, (void (*)())fn3, 0);
  cout<< T1.uc_stack.ss_sp << " " << Main.uc_stack.ss_sp <<endl;

 swapcontext(&Main, &T1);
 cout << "huilai le" <<endl;
 // getcontext(&T2);
 // T2.uc_link=0;
 // T2.uc_stack.ss_sp=malloc(MEM);
 // T2.uc_stack.ss_size=MEM;
 // T2.uc_stack.ss_flags=0;
 // makecontext(&T2, (void (*)())fn2, 0);
 // swapcontext(&Main, &T2);
 // printf("completed\n");
 // exit(0);
}





// ucontext_t *ptr;

// void fn1()
// {
// //  printf("this is from 1\n");
//     cout<< "this is from 1" <<endl;
//     puts("start f1");

// //  setcontext(&Main);
// }

// void fn2()
// {
// //  printf("this is from 2\n");
// //   setcontext(&a);
// //  printf("finished 1\n");
//     cout<< "this is from 2 "<<endl;
// }

// void start()
// {
//     getcontext(&a);
//     a.uc_link=0;
//     a.uc_stack.ss_sp=malloc(MEM);
//     a.uc_stack.ss_size=MEM;
//     a.uc_stack.ss_flags=0;
//     // makecontext(&a, (void(*)()) fn1, 0);
//     makecontext(&a, (void (*)()) fn1, 0);
// }

// void wo(){
//     getcontext(ptr);
//     // ptr = &a;
//     // getcontext(ptr);

//     // char *stack = new char [MEM];
//     ptr->uc_stack.ss_sp = malloc(MEM);
//     ptr->uc_stack.ss_size = MEM;
//     ptr->uc_stack.ss_flags = 0;
//     ptr->uc_link = nullptr;

//     // makecontext(ptr, (void (*)()) fn1, 0);
// }

// int main(){
//     cout<< "cc" <<endl;
//     start();
//     // wo();
//     getcontext(&T1);
//     T1.uc_link=0;
//     T1.uc_stack.ss_sp=malloc(MEM);
//     T1.uc_stack.ss_size=MEM;
//     T1.uc_stack.ss_flags=0;
//     // makecontext(&T1, fn1, 0);
//     makecontext(&T1, (void(*)()) fn1,0);
//     // makecontext(&T1, (void*)&fn1,0);
//     printf("hahah");
//     cout << "end"<<endl;

//     return 0;
// }