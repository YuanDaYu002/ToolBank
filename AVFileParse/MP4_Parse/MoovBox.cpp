#include "MoovBox.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MoovBox
MoovBox::MoovBox()
{

}

MoovBox::~MoovBox()
{

}

BaseBox* MoovBox::CreateObject()
{
	return new MoovBox();
}
