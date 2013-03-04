/*initialize buffer*/
#include <iostream>
#include <string.h>
#include <stdio.h>
using namespace std;
int main()
{	
	char buffer[1024];
	int keyCount=5;
	cout<<*((int*)(&buffer[1024-8]))<<endl;
	memcpy((int*)(&buffer[1024-8]),(int*)&(keyCount), sizeof(int));
	cout<<*((int*)(&buffer[1024-8]))<<endl;
	memcpy((int*)&(keyCount),(int*)(&buffer[1024-8]), sizeof(int));
	cout<<"keycount is"<<keyCount<<endl;
}
