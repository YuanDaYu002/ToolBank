#include <stdio.h>

#include <string.h>

#if 1
int main(int argc,char *argv[])
{
	if(argc < 2)
	{
		printf("You need input 1 parameter[argv[1]](1~50)!\n ");
		return -1;
	}
	const int numTotal = 50; 
        char sign[51] = {0};
    	memset(sign, '=', numTotal);
	
	int numshow;//1~50
	printf("argv[1] = %s\n",argv[1]);
	sscanf(argv[1],"%d",&numshow);
	if(numshow < 1 || numshow > 50)
	{
		printf("argv[1] out of range(1~50)!\n");
		return -1;
	}
	printf("numshow = %d\n",numshow);
   	printf("[%-*.*s]\n", numTotal, numshow, sign);
    	fflush(stdout);

	return 0;
}
#else
int main(int argc,char *argv[])
{


	const int numTotal = 50; 
        char sign[51] = {0};
    	memset(sign, '=', numTotal);
	
	int numshow = 0;//1~50
	
	for(;;)
	{	
		numshow ++;
		if(numshow < 1 || numshow > 50)
		{
			//printf("numshow out of range(1~50)!\n");
			break;
		}
		//printf("numshow = %d\n",numshow);
   		printf("[%-*.*s]", numTotal, numshow, sign);
    		fflush(stdout);
		sleep(1);
	}

	return 0;
}

#endif


