#ifdef VOICE_RECOG_DLL
#define VOICERECOGNIZEDLL_API __declspec(dllexport)
#else
#define VOICERECOGNIZEDLL_API
#endif


#ifndef VOICE_RECOG_H
#define VOICE_RECOG_H

#ifdef __cplusplus
extern "C" {
#endif
	enum VRErrorCode
	{
		VR_SUCCESS = 0, VR_NoSignal = -1, VR_ECCError = -2, VR_NotEnoughSignal = 100
		, VR_NotHeaderOrTail = 101, VR_RecogCountZero = 102
	};

	enum DecoderPriority
	{
		CPUUsePriority = 1//不占内存，但CPU消耗比较大一些
		, MemoryUsePriority = 2//不占CPU，但内存消耗大一些
		, CryDetectPriority = 3
	};

	typedef enum {vr_false = 0, vr_true = 1} vr_bool;

	typedef void (*vr_pRecognizerStartListener)(void *_listener, float _soundTime);
	//_result如果为VR_SUCCESS，则表示识别成功，否则为错误码，成功的话_data才有数据
	typedef void (*vr_pRecognizerEndListener)(void *_listener, float _soundTime, int _result, char *_data, int _dataLen);
	struct VoiceMatch
	{
		short frequency;
		short length;
		float strength;

#ifdef _DEBUG
		////debug
		//short frequencyY;
		//char bigMatchCount, preciseMatchCount;
#endif
	};
	typedef void (*vr_pRecognizerMatchListener)(void *_listener, int _timeIdx, struct VoiceMatch *_matches, int _matchesLen);

	//创建声波识别器
	VOICERECOGNIZEDLL_API void *vr_createVoiceRecognizer(enum DecoderPriority _decoderPriority);
	VOICERECOGNIZEDLL_API void *vr_createVoiceRecognizer2(enum DecoderPriority _decoderPriority, int _sampleRate);

	//销毁识别器
	VOICERECOGNIZEDLL_API void vr_destroyVoiceRecognizer(void *_recognizer);

	//设置解码频率
	//_freqs数组是静态的，整个解码过程中不能释放
	VOICERECOGNIZEDLL_API void vr_setRecognizeFreqs(void *_recognizer, int *_freqs, int _freqCount);
	//设置第二个频段的频率
	VOICERECOGNIZEDLL_API void vr_setRecognizeFreqs2(void *_recognizer, int *_freqs, int _freqCount);

	//设置识别到信号的监听器
	VOICERECOGNIZEDLL_API void vr_setRecognizerListener(void *_recognizer, void *_listener, vr_pRecognizerStartListener _startListener, vr_pRecognizerEndListener _endListener);
	VOICERECOGNIZEDLL_API void vr_setRecognizerListener2(void *_recognizer, void *_listener, vr_pRecognizerStartListener _startListener, 
		vr_pRecognizerEndListener _endListener, vr_pRecognizerMatchListener _matchListener);
	//设置第二个频段的监听器
	VOICERECOGNIZEDLL_API void vr_setRecognizerFreq2Listener(void *_recognizer, void *_listener, vr_pRecognizerStartListener _startListener, 
		vr_pRecognizerEndListener _endListener, vr_pRecognizerMatchListener _matchListener);

	//开始识别
	//这里一般是线程，这个函数在停止识别之前不会返回
	VOICERECOGNIZEDLL_API void vr_runRecognizer(void *_recognizer);

	//暂停信号分析
	VOICERECOGNIZEDLL_API void vr_pauseRecognize(void *_recognizer, int _microSeconds);

	//停止识别，该函数调用后vr_runRecognizer会返回
	//该函数只是向识别线程发出退出信号，判断识别器是否真正已经退出要使用以下的vr_isRecognizerStopped函数
	VOICERECOGNIZEDLL_API void vr_stopRecognize(void *_recognizer);

	//判断识别器线程是否已经退出
	VOICERECOGNIZEDLL_API vr_bool vr_isRecognizerStopped(void *_recognizer);

	//要求输入数据要求为44100，单声道，16bits采样精度，小端编码的音频数据
	//小端编码不用特别处理，一般你录到的数据都是小端编码的
	VOICERECOGNIZEDLL_API int vr_writeData(void *_recognizer, char *_data, int _dataLen);	

	VOICERECOGNIZEDLL_API int vr_getVer();

#if 0
	//从voiceRecognizer获取最近_seconds秒的特征voicePattern，返回的对象要使用者负责删除
	VOICERECOGNIZEDLL_API void *vr_getVoicePattern(void *_recognizer, int _seconds);
	//把voicePattern转成base64，方便上传和下载，返回的字符串要使用者负责释放
	VOICERECOGNIZEDLL_API char *vr_voicePatternToBase64(void *_pattern);
	//把base64编码的voicePattern还原成内存对象，返回的字符串要使用者负责删除
	VOICERECOGNIZEDLL_API void *vr_base64ToVoicePattern(char *_base64);
	//删除voicePattern内存
	VOICERECOGNIZEDLL_API void vr_destoryVoicePattern(void *_pattern);

	VOICERECOGNIZEDLL_API void *vr_createVoicePatternMatcher();
	VOICERECOGNIZEDLL_API void vr_destoryVoicePatternMatcher(void *_matcher);
	//把voiceRecognizer与voiceMatcher关联起来，使voiceRecognizer产生的voicePattern自动送入voiceMatcher
	//_streamID代表该路音频流的编号，在vr_matchVoicePattern返回的音频流就是用streamID代表的
	VOICERECOGNIZEDLL_API void vr_addVoicePatternSource(void *_matcher, short _streamID, void *_sourceRecognizer);
	//手动把某一个voicePattern送入voiceMatcher
	//_streamID的意义同上
	VOICERECOGNIZEDLL_API void vr_addVoicePattern(void *_matcher, short _streamID, void *_pattern);
	//返回多个匹配程度不同的流，不同的流用_streamIDResult数组代表
	//_matchResult为每一路流匹配的百分比
	//返回是否成功匹配
	VOICERECOGNIZEDLL_API int vr_matchVoicePattern(void *_matcher, void *_pattern, short *_streamIDResult, int *_matchResult, int _resultBufLen, int *_returnLen);
#endif


	//应用层解码接口

	//int vr_decodeData(char *_hexs, int _hexsLen, int *_hexsCostLen, char *_result, int _resultLen);
	
	VOICERECOGNIZEDLL_API vr_bool vr_decodeString(int _recogStatus, char *_data, int _dataLen, char *_result, int _resultLen);

	//传输层中数据类型标志
	enum InfoType
	{
		IT_WIFI = 0//说明传输的数据为WiFi
		, IT_SSID_WIFI = 1//ssid编码的WIFI
		, IT_PHONE = 2//说明传输的数据为手机注册信息
		, IT_STRING = 3//任意字符串
		, IT_BLOCKS = 7
	};

	VOICERECOGNIZEDLL_API enum InfoType vr_decodeInfoType(char *_data, int _dataLen);

	//wifi解码
	struct WiFiInfo
	{
		char mac[8];
		int macLen;
		char pwd[80];
		int pwdLen;
	};

	VOICERECOGNIZEDLL_API vr_bool vr_decodeWiFi(int _result, char *_data, int _dataLen, struct WiFiInfo *_wifi);

	struct SSIDWiFiInfo
	{
		char ssid[33];
		int ssidLen;
		char pwd[80];
		int pwdLen;
	};

	VOICERECOGNIZEDLL_API vr_bool vr_decodeSSIDWiFi(int _result, char *_data, int _dataLen, struct SSIDWiFiInfo *_wifi);

	struct PhoneInfo
	{
		char imei[18];
		int imeiLen;
		char phoneName[20];
		int nameLen;
	};

	VOICERECOGNIZEDLL_API vr_bool vr_decodePhone(int _result, char *_data, int _dataLen, struct PhoneInfo *_phone);


	//慧视
	struct FS_SSIDWiFiInfo
	{
		char chAppType; //'0'--'F'
		char chHead[4]; //'0'--'F'
		char ssid[75];
		char pwd[33];
		int ssidLen;
		int pwdLen;
	};

	VOICERECOGNIZEDLL_API vr_bool vr_decodeFSSSIDWiFi(int _result, char *_data, int _dataLen, struct FS_SSIDWiFiInfo *_wifi);

	//竹信
	struct ZS_SSIDWiFiInfo
	{
		char phone[12];
		char ssid[33];
		int ssidLen;
		char pwd[80];
		int pwdLen;
	};

	VOICERECOGNIZEDLL_API vr_bool vr_decodeZSSSIDWiFi(int _result, char *_data, int _dataLen, struct ZS_SSIDWiFiInfo *_wifi);

	struct BlocksComposer
	{
		short blockCount;
		short dataLen;
		char blockFlags[4];
		char data[512];
	};
	VOICERECOGNIZEDLL_API void vr_bs_reset(struct BlocksComposer *_this);
	VOICERECOGNIZEDLL_API int vr_bs_composeBlock(struct BlocksComposer *_this, char *_blockData, int _blockDataLen);
	VOICERECOGNIZEDLL_API vr_bool vr_bs_isAllBlockComposed(struct BlocksComposer *_this);


	//晓舟
	struct XZ_XXX
	{
		char sn[6+1];
		unsigned char ip[4];
		int port;
	};
	VOICERECOGNIZEDLL_API vr_bool vr_decodeXZXXX(int _result, char *_data, int _dataLen, struct XZ_XXX *_xxx);

#ifdef __cplusplus
}
#endif

#endif



