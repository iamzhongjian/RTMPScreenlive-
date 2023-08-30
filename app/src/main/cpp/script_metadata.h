//
// Created by ZJ on 2023/8/18.
//

#ifndef RTMTSCREENLIVE_SCRIPT_METADATA_H
#define RTMTSCREENLIVE_SCRIPT_METADATA_H

/**
 * https://github.com/leixiaohua1020/simplest_librtmp_example/blob/master/simplest_librtmp_send264/librtmp_send264.cpp
 * https://blog.csdn.net/firehood_/article/details/8783589
 * https://blog.csdn.net/C1033177205/article/details/106966862/
 */
#include "librtmp/amf.h"
#include "librtmp/rtmp.h"
#include <stdint.h>

#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"DDDDDD",__VA_ARGS__)

//defined by zj
typedef struct _ReturnValue
{
    int size;
    char * buf;
}ReturnValue;

/**
 * _RTMPMetadata
 * 内部结构体。该结构体主要用于存储和传递元数据信息
 */
typedef struct _RTMPMetadata
{
    // video, must be h264 type
    unsigned int    nWidth;
    unsigned int    nHeight;
    unsigned int    nFrameRate;
    unsigned int    nSpsLen;
    unsigned char   *Sps;
    unsigned int    nPpsLen;
    unsigned char   *Pps;

    // audio, must be aac type
    bool	        bHasAudio;
    unsigned int	nAudioSampleRate;
    unsigned int	nAudioSampleSize;
    unsigned int	nAudioChannels;
    char		    pAudioSpecCfg;
    unsigned int	nAudioSpecCfgLen;
} RTMPMetadata,*LPRTMPMetadata;

enum
{
    FLV_CODECID_H264 = 7,
    FLV_CODECID_AAC = 10,
};

//网络字节序转换
char * put_byte( char *output, uint8_t nVal )
{
    output[0] = nVal;
    return output+1;
}

char * put_be16(char *output, uint16_t nVal )
{
    output[1] = nVal & 0xff;
    output[0] = nVal >> 8;
    return output+2;
}

char * put_be24(char *output,uint32_t nVal )
{
    output[2] = nVal & 0xff;
    output[1] = nVal >> 8;
    output[0] = nVal >> 16;
    return output+3;
}
char * put_be32(char *output, uint32_t nVal )
{
    output[3] = nVal & 0xff;
    output[2] = nVal >> 8;
    output[1] = nVal >> 16;
    output[0] = nVal >> 24;
    return output+4;
}
char *  put_be64( char *output, uint64_t nVal )
{
    output=put_be32( output, nVal >> 32 );
    output=put_be32( output, nVal );
    return output;
}

char * put_amf_string( char *c, const char *str )
{
    uint16_t len = strlen( str );
    c=put_be16( c, len );
    memcpy(c,str,len);
    return c+len;
}
char * put_amf_double( char *c, double d )
{
    *c++ = AMF_NUMBER;  /* type: Number */
    {
        unsigned char *ci, *co;
        ci = (unsigned char *)&d;
        co = (unsigned char *)c;
        co[0] = ci[7];
        co[1] = ci[6];
        co[2] = ci[5];
        co[3] = ci[4];
        co[4] = ci[3];
        co[5] = ci[2];
        co[6] = ci[1];
        co[7] = ci[0];
    }
    return c+8;
}
char * put_amf_boolean( char *c,unsigned char *b )
{
    *c++ = AMF_BOOLEAN;//AMF_BOOLEAN占1 Byte，AMF_NUMBER占8 Byte
    {
        unsigned char *ci, *co;
        ci = (unsigned char *)&b;
        co = (unsigned char *)c;
        co[0] = ci[0];//值占1 Byte
    }
    return c+1;
}

//打印char指针
void print_char_pointer(char *c,int len){
    LOGI("print_char_pointer()--->%p,%p,%p,%p,%p,%zu",c,&c,&c[0],(void *)c,(char *)(void *)c,strlen(c));

    LOGI("遍历打印:");
    for (int i = 0; i < len; ++i) {
        LOGI("%c",c[i]);
    }
}

void print_char_pointer_size_strlen(char * p){
//    //64位机测试
//    LOGI("print_char_pointer_size_strlen()====================sizeof(char): %lu", sizeof(char));//1
//    LOGI("print_char_pointer_size_strlen()====================sizeof(char *): %lu", sizeof(char *));//8
//    LOGI("print_char_pointer_size_strlen()====================sizeof(unsigned char *): %lu", sizeof(unsigned char *));//8
//    LOGI("print_char_pointer_size_strlen()====================sizeof(unsigned long): %lu", sizeof(unsigned long));//8

    size_t buf_size = sizeof(p);
    size_t buf_size0 = sizeof(&p);
    size_t buf_size1 = sizeof(&p[0]);
    LOGI("print_char_pointer_size_strlen()------>sizeof(p): %zu, sizeof(&p): %zu, sizeof(&p[0]): %zu\n",buf_size,buf_size0,buf_size1);
    size_t buf_size2 = strlen(p);
    size_t buf_size3 = strlen(&p[0]);
    size_t buf_size4 = strlen((const char*)&p);
    size_t buf_size5 = strlen(reinterpret_cast<const char *const>(&p));
    LOGI("print_char_pointer_size_strlen()------>strlen(p): %zu, strlen(&p[0]): %zu, strlen((const char*)&p): %zu, strlen(reinterpret_cast<const char *const>(&p)): %zu\n",buf_size2,buf_size3,buf_size4,buf_size5);

    int length = 0;
    while (*p)//*p的内容为0时，跳出循环
    {
        p++;
        length++;
    }
    LOGI("print_char_pointer_size_strlen()------>length: %d\n",length);
}

/**
 * 结构体默认初始化的规则
在C语言中，如果我们没有手动初始化结构体的成员，编译器会使用默认规则进行初始化。结构体默认初始化的规则如下：

如果结构体成员是基本数据类型（如int、float等），则默认初始化为0。
如果结构体成员是指针类型，则默认初始化为NULL。
如果结构体成员是数组类型，则默认初始化为数组元素类型的默认值。
如果结构体成员是嵌套的结构体类型，则按照上述规则递归进行初始化。
 */
void test_struct(){
    ReturnValue returnValue1 = {0};
    LOGI("test_struct()---1--->returnValue.size: %d\n",returnValue1.size);//int初始化为0，不报错崩溃
    print_char_pointer_size_strlen(returnValue1.buf);//char *未初始化，不能用strlen，报错崩溃

    ReturnValue returnValue2 = {};
    LOGI("test_struct()---2--->returnValue2.size: %d\n",returnValue2.size);//int初始化为0，不报错崩溃
    print_char_pointer_size_strlen(returnValue2.buf);//char *未初始化，不能用strlen，报错崩溃

    ReturnValue returnValue3;
    LOGI("test_struct()---3--->returnValue3.size: %d\n",returnValue3.size);//int未初始化，为一个负数比如-356013056，不报错崩溃
    print_char_pointer_size_strlen(returnValue3.buf);//char *未初始化，不能用strlen，报错崩溃
}

//bool SendMetadata(LPRTMPMetadata lpMetaData)
//char * CreateMetadata(LPRTMPMetadata lpMetaData)
ReturnValue CreateMetadata(LPRTMPMetadata lpMetaData)
{
    if(lpMetaData == NULL)
    {
//        return false;
//        return NULL;
        return ReturnValue{};
    }

    /**
     * 返回这个body局部变量会警告Address of stack memory associated with local variable 'body' returned
     * 声明为栈区的变量在超出程序块范围后会被自动地释放
     *
     * 解决方法:
     * 1.Declare variable(str) as static: static char * str; - this will allocate space for str in the static space and not on the stack.
     * 用到关键字static，将变量声明在静态存储区而不是栈区
     * static 存储类指示编译器在程序的生命周期内保持局部变量的存在，而不需要在每次它进入和离开作用域时进行创建和销毁。因此，使用 static 修饰局部变量可以在函数调用之间保持局部变量的值。
     *
     * 2.Allocate space in the called function on heap (using malloc, calloc, and friends) and pass the ownership to the caller function, which will have to deallocate this space when not needed anymore (using free).
     * 在堆上的被调用函数中分配空间（使用 malloc、calloc 和friends）并将所有权传递给调用函数，调用函数必须在不再需要时释放该空间（使用 free）
     *
     * ————————————————
     * 版权声明：本文为CSDN博主「the_subobo」的原创文章，遵循CC 4.0 BY-SA版权协议，转载请附上原文出处链接及本声明。
     * 原文链接：https://blog.csdn.net/m0_54045571/article/details/116373896
     */
    static char body[1024] = {0};;

    char * p = (char *)body;

    //这一段可以不要
//    p = put_byte(p, AMF_STRING );
//    p = put_amf_string(p , "@setDataFrame" );

    p = put_byte( p, AMF_STRING );
    p = put_amf_string( p, "onMetaData" );

    //这一段可以不要，加了这个AMF_OBJECT拉流端可能拉不到流
//    p = put_byte(p, AMF_OBJECT );
//    p = put_amf_string( p, "copyright" );
    //这一段可以不要
//    p = put_byte(p, AMF_STRING );
//    p = put_amf_string( p, "zj" );

    p = put_byte(p, AMF_ECMA_ARRAY );
    p = put_be32(p, 8 );//占4个字节。下面的属性有几个就写几个

    p = put_amf_string( p, "width");
    p = put_amf_double( p, lpMetaData->nWidth);
    p = put_amf_string( p, "height");
    p = put_amf_double( p, lpMetaData->nHeight);
    p = put_amf_string( p, "framerate" );
    p = put_amf_double( p, lpMetaData->nFrameRate);
    p = put_amf_string( p, "videocodecid" );
    p = put_amf_double( p, FLV_CODECID_H264 );

    p = put_amf_string( p, "audiosamplerate" );
    p = put_amf_double( p, lpMetaData->nAudioSampleRate );
    p = put_amf_string( p, "audiosamplesize" );
    p = put_amf_double( p, lpMetaData->nAudioSampleSize );

    p = put_amf_string( p, "stereo" );
//    p = put_amf_double( p, lpMetaData->nAudioChannels == 1 ? false : true );//double也行，拉流端解析为double就行
//    p = put_amf_boolean( p, lpMetaData->nAudioChannels == 1 ? (unsigned char *)0 : (unsigned char *)1 );//ok
    *p++ = AMF_BOOLEAN;
//    *p++ = lpMetaData->nAudioChannels == 1 ? false : true;//ok
//    *p++ = lpMetaData->nAudioChannels == 1 ? 0 : 1;//ok
    *p++ = lpMetaData->nAudioChannels == 1 ? 0x00 : 0x01;

    p = put_amf_string( p, "audiocodecid" );
    p = put_amf_double( p, FLV_CODECID_AAC );

    /* end of object - 0x00 0x00 0x09 */
    p = put_amf_string( p, "" );//这一句要，这是两个0x00
    p = put_byte( p, AMF_OBJECT_END  );//这只是一个字节0x09

    int index = p-body;//指针与指针相减，得出的结果为：两者之间的距离！

    //无法打印
//    printf("CreateMetadata()====================index: %d\n",index);
//    fflush(stdout);

    LOGI("CreateMetadata()====================index: %d\n",index);

    //保存为文件查看二进制数据进行分析
    const char *path = "/storage/emulated/0/metadata.txt";
    FILE* pFile = fopen(path, "wb");
    fwrite(body, 1, index, pFile);
    fclose(pFile);

//    //测试发现，body和p不是一般的字符串
//    print_char_pointer(body,index);//0x7efe396790,0x7edabfeb50,0x7efe396790,0x7efe396790,0x7efe396790,1
//    print_char_pointer(p,index);//0x7efe39684d,0x7edabfeb50,0x7efe39684d,0x7efe39684d,0x7efe39684d,0
//
//    print_char_pointer_size_strlen(body);//sizeof:8,8,8---strlen:1,1,5,5---length: 1
//    print_char_pointer_size_strlen(p);//sizeof:8,8,8---strlen:0,0,5,5---length: 0
//
//    //测试
//    char *buf_test = "123";
////    char *buf_test;//char *未初始化，不能用strlen，报错崩溃
//    size_t size = sizeof(buf_test);
//    LOGI("buf_test====================size: %zu", size);//8
//    size_t str_len = strlen(buf_test);
//    LOGI("buf_test====================str_len: %zu", str_len);//3
//    print_char_pointer(buf_test, strlen(buf_test));//0x7ee806c827,0x7edafce500,0x7ee806c827,0x7ee806c827,0x7ee806c827,3
//    print_char_pointer_size_strlen(buf_test);//sizeof:8,8,8---strlen:3,3,5,5---length: 3

//    test_struct();

//    return p;//返回p无效
//    return (char*)body;//但是调用函数还要计算长度
//    return ReturnValue{index,p};//返回p无效
    return ReturnValue{index,(char*)body};//返回多个变量可以用结构体

//    SendPacket(RTMP_PACKET_TYPE_INFO,(unsigned char*)body,p-body,0);



//    int i = 0;
//    body[i++] = 0x17; // 1:keyframe  7:AVC
//    body[i++] = 0x00; // AVC sequence header
//
//    body[i++] = 0x00;
//    body[i++] = 0x00;
//    body[i++] = 0x00; // fill in 0;
//
//    // AVCDecoderConfigurationRecord.
//    body[i++] = 0x01; // configurationVersion
//    body[i++] = lpMetaData->Sps[1]; // AVCProfileIndication
//    body[i++] = lpMetaData->Sps[2]; // profile_compatibility
//    body[i++] = lpMetaData->Sps[3]; // AVCLevelIndication
//    body[i++] = 0xff; // lengthSizeMinusOne
//
//    // sps nums
//    body[i++] = 0xE1; //&0x1f
//    // sps data length
//    body[i++] = lpMetaData->nSpsLen>>8;
//    body[i++] = lpMetaData->nSpsLen&0xff;
//    // sps data
//    memcpy(&body[i],lpMetaData->Sps,lpMetaData->nSpsLen);
//    i= i+lpMetaData->nSpsLen;
//
//    // pps nums
//    body[i++] = 0x01; //&0x1f
//    // pps data length
//    body[i++] = lpMetaData->nPpsLen>>8;
//    body[i++] = lpMetaData->nPpsLen&0xff;
//    // sps data
//    memcpy(&body[i],lpMetaData->Pps,lpMetaData->nPpsLen);
//    i= i+lpMetaData->nPpsLen;
//
//    return SendPacket(RTMP_PACKET_TYPE_VIDEO,(unsigned char*)body,i,0);

}

#endif //RTMTSCREENLIVE_SCRIPT_METADATA_H
