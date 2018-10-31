
typedef struct
{
    int fd;
    PLAY_END_CALLBACK cbFunc;
    void* cbfPara;
} PLAY_TIP_CONTEX;

PLAY_TIP_CONTEX lTipCtx = {-1, NULL, NULL};


int put_data_to_AO(void *data, int size)
{

    AUDIO_STREAM_S stAudioStream;
    memset(&stAudioStream, 0, sizeof(stAudioStream));
    stAudioStream.u32Len = size;
    stAudioStream.pStream = (HLE_U8 *) data;
    HLE_S32 ret = HI_MPI_ADEC_SendStream(TALK_ADEC_CHN, &stAudioStream, HI_TRUE);
    if (ret) 
	{
        ERROR_LOG("HI_MPI_ADEC_SendStream fail: %#x!\n", ret);
    }

    return HLE_RET_OK;
}


/**************************************************************
参数：
	para：线程响应函数传入参数
**************************************************************/
static void* play_tip_proc(void* para)
{
    PLAY_TIP_CONTEX* ctx = (PLAY_TIP_CONTEX*) para;

    char buf[160 + 4];
	
	//填充海思头  
    buf[0] = 0x00;
    buf[1] = 0x01;
    buf[2] = 0x50;
    buf[3] = 0x00;

    int len;
    while ((len = read(ctx->fd, &buf[4], 160)) == 160) //160为每个音频帧数据长度
	{
       if(HLE_RET_OK != put_data_to_AO(buf, sizeof (buf))) 
			printf("talkback_put_data error!\n");

    }
    if (len < 0) 
	{
        ERROR_LOG("read fail: %d\n", errno);
    }

    close(ctx->fd);
    ctx->fd = -1;
	//函数回调
    if (ctx->cbFunc) ctx->cbFunc(ctx->cbfPara);

    return NULL;
}

/**************************************************************
参数：
	fn:音频文件名 
		（不带海思头，带海思头的文件需要修改play_tip_proc函数）
	cbFunc：回调函数名
	cbfPara：回电函数传参数
**************************************************************/
int play_tip(char* fn, PLAY_END_CALLBACK cbFunc, void* cbfPara)
{
    if (lTipCtx.fd >= 0) return HLE_RET_EBUSY;

    lTipCtx.cbFunc = cbFunc;
    lTipCtx.cbfPara = cbfPara;
    int fd = open(fn, O_RDONLY);
    if (fd < 0) 
	{
        //ERROR_LOG("open %s fail: %d\n", fn, errno);
        return HLE_RET_EIO;
    }
    lTipCtx.fd = fd;

    pthread_t tid;
    int ret = pthread_create(&tid, NULL, play_tip_proc, &lTipCtx);
    if (ret) 
	{
        ERROR_LOG("pthread_create fail: %d\n", ret);
        close(lTipCtx.fd);
        lTipCtx.fd = -1;
        return HLE_RET_ERROR;
    }
    pthread_detach(tid);

    return HLE_RET_OK;
}

/*本代码不能直接编译，只是描述其操作方法*/
int main()
{
	
	play_tip("/jffs0/001.pcm",NULL,NULL);
	return 0;
}