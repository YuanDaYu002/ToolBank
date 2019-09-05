
#ifdef DUAL_TONE
static int DefaultCodeFrequency[] = { 15000, 15200, 15400, 15600, 15800, 16000, 16200, 16400, 16600, 16800, 17000, 17200, 17400, 17600, 17800, 18000, 18200, 18400, 18600, 18800, 19000, 19200, 19400 };
int DEFAULT_minFrequency = 14800;
int DEFAULT_maxFrequency = 19600;
#else
static int DefaultCodeFrequency[] = { 15000, 15200, 15400, 15600, 15800, 16000, 16200, 16400, 16600, 16800, 17000, 17200, 17400, 17600, 17800, 18000, 18200, 18400, 18600 };
static int DefaultCodeFrequency2[] = { 15000, 15200, 15400, 15600, 15800, 16000, 16200, 16400, 16600, 16800, 17000, 17200, 17400, 17600, 17800, 18000, 18200, 18400, 18600 };
int DEFAULT_minFrequency = 14800;
int DEFAULT_maxFrequency = 18800;
int DEFAULT_minFrequency2 = 14800;
int DEFAULT_maxFrequency2 = 18800;
#endif

const char DEFAULT_CODE_BOOK[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
int *DEFAULT_CODE_FREQUENCY = (int *)DefaultCodeFrequency;
int *DEFAULT_CODE_FREQUENCY2 = (int *)DefaultCodeFrequency2;
int DEFAULT_CODE_BOOK_SIZE = sizeof(DEFAULT_CODE_BOOK)/sizeof(char);