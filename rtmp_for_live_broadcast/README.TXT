1.rtmpdump是主要的库，zlib 和 openssl 是依赖库
2.最上层的.c 以及 .h 文件是最终的应用程序源文件，编译时除了librtmp.a外，仍然需要链接libz.a 以及 libssl.a相关库
