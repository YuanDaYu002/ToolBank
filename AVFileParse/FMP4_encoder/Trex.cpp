#include "Trex.h"

TrexBox::TrexBox()
{

}

TrexBox::TrexBox(unsigned int &meta_ID)
{
      
		/*
     data = new Uint8 Array([
        0x00, 0x00, 0x00, 0x00, // version(0) + flags
        (trackId >> 24) & 0xFF, // track_ID
        (trackId >> 16) & 0xFF,
        (trackId >> 8) & 0xFF,
        (trackId) & 0xFF,
        0x00, 0x00, 0x00, 0x01, // default_sample_description_index
        0x00, 0x00, 0x00, 0x00, // default_sample_duration
        0x00, 0x00, 0x00, 0x00, // default_sample_size
        0x00, 0x01, 0x00, 0x01 // default_sample_flags
    ]);
    */
    track_ID = meta_ID;
    default_sample_description_index = 0x01;
    default_sample_duration = 0x00;
    default_sample_size = 0x00;
    default_sample_flags = 0x01<<16||0x01; 
   
}



TrexBox::~TrexBox()
{

}

int TrexBox::ParseAttrs(byteptr &pData)
{

}

static BaseBox* TrexBox::CreateObject()
{
		return new TrexBox();
}



