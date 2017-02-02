#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

using namespace std;

int fn()
{
	cout << "end" << endl;
}

int main()
{
	fn(0);
	cout << "pass" << endl;
}
