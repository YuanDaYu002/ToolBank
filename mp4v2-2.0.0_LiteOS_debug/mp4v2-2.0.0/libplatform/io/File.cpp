
#include "libplatform/impl.h"
//#include "LiteOS_file_operation.cpp"




namespace mp4v2 { namespace platform { namespace io {

///////////////////////////////////////////////////////////////////////////////

namespace {
    const File::Size __maxChunkSize = 1024*1024;
}

///////////////////////////////////////////////////////////////////////////////

File::File( std::string name_, Mode mode_, FileProvider* provider_ )
    : _name     ( name_ )
    , _isOpen   ( false )
    , _mode     ( mode_ )
    , _size     ( 0 )
    , _position ( 0 )
    , _provider ( provider_ ? *provider_ : standard() )
    , name      ( _name )
    , isOpen    ( _isOpen )
    , mode      ( _mode )
    , size      ( _size )
    , position  ( _position )
{
}

///////////////////////////////////////////////////////////////////////////////

File::~File()
{
    close();
    delete &_provider;
}

///////////////////////////////////////////////////////////////////////////////

void
File::setMode( Mode mode_ )
{
    _mode = mode_;
}

void
File::setName( const std::string& name_ )
{
    _name = name_;
}

///////////////////////////////////////////////////////////////////////////////
#define use_LiteOS_file_opreation 0

//成功：返回0       失败：返回1
bool
File::open( std::string name_, Mode mode_ )
{

#if use_LiteOS_file_opreation
	char myname[64] = {0};
	strcpy(myname,name_.c_str());
	printf("myname = %s\n",myname);
	 
	return LiteOS_open(myname, (MP4FileMode)mode_ );

#else

    if( _isOpen )
        return true;
	printf("into File::open !\n");
    if( !name_.empty() )
        setName( name_ );
    if( mode_ != MODE_UNDEFINED )
        setMode( mode_ );
	printf("into _provider.open !\n");

	
    if( _provider.open( _name, _mode ))//返回0就是没问题,执行下边的
    {
    	printf("_provider.open failed!\n");
		  return true;
	}
	
	printf("_provider.open success !\n");

#if 1
    FileSystem::getFileSize( _name, _size );
#endif
		printf("into _isOpen = true !\n");
		
    _isOpen = true;		//成功打开
    return false;
#endif
}

bool
File::seek( Size pos )
{
	#if use_LiteOS_file_opreation
	
	return LiteOS_seek(&LiteOS_file_handle,pos);
	#else
    if( !_isOpen )
        return true;

    if( _provider.seek( pos ))
        return true;
    _position = pos;
    return false;
	#endif
}

bool
File::read( void* buffer, Size size, Size& nin, Size maxChunkSize )
{
#if use_LiteOS_file_opreation
	return LiteOS_read(&LiteOS_file_handle,buffer,size,&nin,maxChunkSize);
#else

    nin = 0;

    if( !_isOpen )
        return true;

    if( _provider.read( buffer, size, nin, maxChunkSize ))
        return true;

    _position += nin;
    if( _position > _size )
        _size = _position;

    return false;
	#endif
}

bool
File::write( const void* buffer, Size size, Size& nout, Size maxChunkSize )
{
	#if use_LiteOS_file_opreation
		return LiteOS_write(&LiteOS_file_handle,buffer,size,&nout,maxChunkSize);
	#else
	
	nout = 0;

    if( !_isOpen )
        return true;

    if( _provider.write( buffer, size, nout, maxChunkSize ))
        return true;

    _position += nout;
    if( _position > _size )
        _size = _position;

    return false;
	#endif
}

bool
File::close()
{
	#if use_LiteOS_file_opreation
		return LiteOS_close(&LiteOS_file_handle);
	#else
	
    if( !_isOpen )
        return false;
    if( _provider.close() )
        return true;

    _isOpen = false;
    return false;
	#endif
}

///////////////////////////////////////////////////////////////////////////////

CustomFileProvider::CustomFileProvider( const MP4FileProvider& provider )
    : _handle( NULL )
{
    memcpy( &_call, &provider, sizeof(MP4FileProvider) );
}

bool
CustomFileProvider::open( std::string name, Mode mode )
{
    MP4FileMode fm;
    switch( mode ) {
        case MODE_READ:   fm = FILEMODE_READ;   break;
        case MODE_MODIFY: fm = FILEMODE_MODIFY; break;
        case MODE_CREATE: fm = FILEMODE_CREATE; break;

        case MODE_UNDEFINED:
        default:
            fm = FILEMODE_UNDEFINED;
            break;
    }

    _handle = _call.open( name.c_str(), fm );
    return _handle == NULL;
}

bool
CustomFileProvider::seek( Size pos )
{
    return _call.seek( _handle, pos );
}

bool
CustomFileProvider::read( void* buffer, Size size, Size& nin, Size maxChunkSize )
{
    return _call.read( _handle, buffer, size, &nin, maxChunkSize );
}

bool
CustomFileProvider::write( const void* buffer, Size size, Size& nout, Size maxChunkSize )
{
    return _call.write( _handle, buffer, size, &nout, maxChunkSize );
}

bool
CustomFileProvider::close()
{
    return _call.close( _handle );
}

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::platform::io


