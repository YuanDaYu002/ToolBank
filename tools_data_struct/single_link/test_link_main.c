
#include <stdio.h>
#include "link.h"



int main(void)
{

	char* str1 = "11111";
	char* str2 = "22222";
	char* str3 = "33333";
	char* str4 = "44444";

	link_t*heade = CreateLink();
	if(NULL == heade)
	{
		printf("CreateLink error!\n");
		return -1;
	}

	InsertLinkItem(heade,str1,HEAD);
	InsertLinkItem(heade,str2,HEAD);
	InsertLinkItem(heade,str3,HEAD);
	InsertLinkItem(heade,str4,HEAD);

	link_t*tmp = heade->pNext;

	do
	{
		printf("->pNext:%s\n",tmp->data);
		if(tmp->pNext != NULL)
		{
			tmp = tmp->pNext;			
		}
		else
		{
			break;
		}
		
	}while(1);

	char* p = NULL;
	DelLinkItem(heade,&p,HEAD);
	printf("p = %s\n",p);
	p = NULL;
	
	DelLinkItem(heade,&p,TAIL);
	printf("p = %s\n",p);

	
	DestroyLink(heade,0);

	return 0;

}





