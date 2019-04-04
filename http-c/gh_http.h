#ifndef __GH_HTTP_H__
#define __GH_HTTP_H__

#ifdef __cplusplus
extern "C" {
#endif

int http_upload_file(const char* pHost, const int nPort, const char* pServerPath,
                     const char* pMessageJson, const char* pFile);

int http_post_data(const char* pHost, const int nPort, const char* pServerPath,
                     const char* pMessageJson, int* ret_code, char** ret_buffer);

#ifdef __cplusplus
}
#endif

#endif
