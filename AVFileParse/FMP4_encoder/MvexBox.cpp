#include "MvexBox.h"

MvexBox::MvexBox()
{

}

MvexBox::~MvexBox()
{

}

int MvexBox::ParseAttrs(byteptr &pData)
{

}

static BaseBox* MvexBox::CreateObject()
{
	return new MvexBox();
}

