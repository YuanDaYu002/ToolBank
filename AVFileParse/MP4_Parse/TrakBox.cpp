#include "TrakBox.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TrakBox
TrakBox::TrakBox()
{
}

TrakBox::~TrakBox()
{
}

BaseBox* TrakBox::CreateObject()
{
	return new TrakBox();
}
