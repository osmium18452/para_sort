#include <iostream>
#include <cstdio>
#include <ctime>
using namespace std;
int main(int argc,char** argv){
	FILE *inFile=fopen(argv,"r");
	int n;
	fscanf(inFile,"%d",&n);
	int *a=(int *) malloc(sizeof(int)*n);
	for (int i=0;i<n;i++){
		fscanf(inFile,"%d",a+i);
	}

}