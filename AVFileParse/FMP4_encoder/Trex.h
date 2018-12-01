#pragma once
#include "BaseBox.h"



/********************************************************************************************
**							Track Extends Box  (Trex)
**
--------------------------------------------------------------------------------------------
**			字段名称						|	长度(bytes)	|		有关描述
--------------------------------------------------------------------------------------------
**			track_ID					|	   4		|		指明当前描述的 track ID
--------------------------------------------------------------------------------------------
** default_sample_description_index		|	   4		|	
--------------------------------------------------------------------------------------------
** default_sample_duration				|	   4		|	
--------------------------------------------------------------------------------------------
** default_sample_size					|	   4		|
--------------------------------------------------------------------------------------------
** default_sample_flags 				|	   4		|
********************************************************************************************/
class TrexBox : public BaseBox
{
public:
	TrexBox();
	TrexBox(unsigned int &meta_ID);
	virtual ~TrexBox();

	virtual int ParseAttrs(byteptr &pData);

	static BaseBox* CreateObject();

public:
    unsigned int track_ID;
    unsigned int default_sample_description_index;
    unsigned int default_sample_duration;
    unsigned int default_sample_size;
    unsigned int default_sample_flags; 


};



