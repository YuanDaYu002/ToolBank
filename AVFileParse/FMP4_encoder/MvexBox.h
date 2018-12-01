#pragma once
#include "BaseBox.h"




/********************************************************************************************
**							Movie Extends Box (mvex)
**
--------------------------------------------------------------------------------------------
**		字段名称			|	长度(bytes)	|		有关描述
--------------------------------------------------------------------------------------------
**	fragment_duration	|	   4		|		
********************************************************************************************/
class MvexBox : public BaseBox
{
public:
	MvexBox();
	virtual ~MvexBox();

	virtual int ParseAttrs(byteptr &pData);

	static BaseBox* CreateObject();

public:
	unsigned int			m_iFragmentDuration;

};



