#include "MdiaBox.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MdiaBox
MdiaBox::MdiaBox()
{
}

MdiaBox::~MdiaBox()
{
}

BaseBox* MdiaBox::CreateObject()
{
	return new MdiaBox();
}
