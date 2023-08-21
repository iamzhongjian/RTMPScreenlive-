#include <jni.h>
#include <string>
#include <android/log.h>
#include <malloc.h>

extern "C" {
#include "librtmp/rtmp.h"
#include "script_metadata.h"
}
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"DDDDDD",__VA_ARGS__)
typedef struct {
    int16_t sps_len;
    int16_t pps_len;
    int8_t *sps;
    int8_t *pps;
    RTMP *rtmp;
} Live;
Live *live = nullptr;

int sendPacket(RTMPPacket *packet) {
    int r = RTMP_SendPacket(live->rtmp, packet, 1);
    RTMPPacket_Free(packet);
    free(packet);
    packet = nullptr;
    return r;
}

/**
 * https://blog.csdn.net/leixiaohua1020/article/details/42105049
 * 最简单的基于librtmp的示例：发布H.264（H.264通过RTMP发布）
 * SourceForge：https://sourceforge.net/projects/simplestlibrtmpexample/
 * Github：https://github.com/leixiaohua1020/simplest_librtmp_example
 * 开源中国：http://git.oschina.net/leixiaohua1020/simplest_librtmp_example
 *
 * https://github.com/leixiaohua1020/simplest_librtmp_example/blob/master/simplest_librtmp_send264/librtmp_send264.cpp
 * 上面的代码并没有发送MetaData示例，下面有
 * https://blog.csdn.net/firehood_/article/details/8783589
 * https://blog.csdn.net/C1033177205/article/details/106966862/
 */
/**
 * script metadata数据封包---不需要script tag的tag header那11个字节（即TagType,DataSize,TimeStamp,TimestampExtended,StreamId）
 * MetaData这个消息可以发也可以不发，不发感觉拉流很慢
 */
RTMPPacket *createMetadataPackage(int width, int height, int framerate,
                                  int audioSampleRate, int audioSampleSize, int audioChannels, Live *live) {
    RTMPMetadata metaData;//ok
    LOGI("createMetadataPackage()==========1==========sizeof(RTMPMetadata): %lu, sizeof(metaData): %lu", sizeof(RTMPMetadata),sizeof(metaData));//64---打印的是结构体里面类型长度的总字节数（struct内存对齐）
    /**
     * void *memset(void *s, int ch, size_t n);
     * 函数解释：将s中当前位置后面的n个字节 （typedef unsigned int size_t ）用 ch 替换并返回 s
     * memset：作用是在一段内存块中填充某个给定的值，它是对较大的结构体或数组进行清零操作的一种最快方法
     */
    memset(&metaData,0,sizeof(RTMPMetadata));//测试发现可以不需要memset
    LOGI("createMetadataPackage()==========memset==========sizeof(RTMPMetadata): %lu, sizeof(metaData): %lu", sizeof(RTMPMetadata),sizeof(metaData));//64---打印的是结构体里面类型长度的总字节数（struct内存对齐）

    //初始化结构体为0时，使用{}或者{0}或者memset
//    RTMPMetadata metaData = {};//ok
//    RTMPMetadata metaData = {0};//ok
//    LOGI("createMetadataPackage()==========1==========sizeof(RTMPMetadata): %lu, sizeof(metaData): %lu", sizeof(RTMPMetadata),sizeof(metaData));//64---打印的是结构体里面类型长度的总字节数（struct内存对齐）

    metaData.nWidth = width;
    metaData.nHeight = height;
    metaData.nFrameRate = framerate;
    metaData.nAudioSampleRate = audioSampleRate;
    metaData.nAudioSampleSize = audioSampleSize;
    metaData.nAudioChannels = audioChannels;
    LOGI("createMetadataPackage()==========2==========sizeof(RTMPMetadata): %lu, sizeof(metaData): %lu", sizeof(RTMPMetadata),sizeof(metaData));//64---打印的是结构体里面类型长度的总字节数（struct内存对齐）

//    char *buf = CreateMetadata(&metaData);
    ReturnValue returnValue = CreateMetadata(&metaData);
    LOGI("createMetadataPackage()==========3==========sizeof(RTMPMetadata): %lu, sizeof(metaData): %lu", sizeof(RTMPMetadata),sizeof(metaData));//64---打印的是结构体里面类型长度的总字节数（struct内存对齐）

    int body_size = returnValue.size;
    int len = body_size;
    LOGI("createMetadataPackage()====================body_size: %d, len: %d", body_size,len);
    RTMPPacket *packet = (RTMPPacket *) malloc(sizeof(RTMPPacket));
    RTMPPacket_Alloc(packet, body_size);
    memcpy(&packet->m_body[0], returnValue.buf, len);

    packet->m_packetType = RTMP_PACKET_TYPE_INFO;
    packet->m_nChannel = 0x03;
    packet->m_nBodySize = body_size;
    packet->m_nTimeStamp = 0;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet->m_nInfoField2 = live->rtmp->m_stream_id;
    return packet;
}

/**
 * C语言跟内存申请相关的函数主要有 alloca、calloc、malloc、free、realloc等.
    <1>alloca是向栈申请内存,因此无需释放.
    <2>malloc分配的内存是位于堆中的,并且没有初始化内存的内容,因此基本上malloc之后,调用函数memset来初始化这部分的内存空间.
    <3>calloc则将初始化这部分的内存,设置为0.
    <4>realloc则对malloc申请的内存进行大小的调整.
    <5>申请的内存最终需要通过函数free来释放.
    当程序运行过程中malloc了,但是没有free的话,会造成内存泄漏.一部分的内存没有被使用,
    但是由于没有free,因此系统认为这部分内存还在使用,造成不断的向系统申请内存,使得系统可用内存不断减少.
    但是内存泄漏仅仅指程序在运行时,程序退出时,OS将回收所有的资源.因此,适当的重起一下程序,有时候还是有点作用.
 */
void prepareVideo(int8_t *data, int len, Live *live) {
    for (int i = 0; i < len; i++) {
        //0x00 0x00 0x00 0x01
        if (i + 4 < len) {
            if (data[i] == 0x00 && data[i + 1] == 0x00
                && data[i + 2] == 0x00
                && data[i + 3] == 0x01) {
                //0x00 0x00 0x00 0x01 7 sps 0x00 0x00 0x00 0x01 8 pps
                //将sps pps分开
                //找到pps
                if (data[i + 4] == 0x68) {
                    //去掉界定符
                    live->sps_len = i - 4;
                    live->sps = static_cast<int8_t *>(malloc(live->sps_len));
                    memcpy(live->sps, data + 4, live->sps_len);

                    live->pps_len = len - (4 + live->sps_len) - 4;
                    live->pps = static_cast<int8_t *>(malloc(live->pps_len));
                    memcpy(live->pps, data + 4 + live->sps_len + 4, live->pps_len);
                    LOGI("sps:%d pps:%d", live->sps_len, live->pps_len);
                    break;
                }
            }
        }
    }
}

/**
 * 视频数据封包
 */
RTMPPacket *createVideoPackage(int8_t *buf, int len, const long tms, Live *live) {
//    分隔符00 00 00 01被抛弃了      --buf指的是651
    buf += 4;//偏移4个int8_t长度，即4字节
    len -= 4;
    int body_size = len + 9;//9是packet->m_body[0]到packet->m_body[8]
    RTMPPacket *packet = (RTMPPacket *) malloc(sizeof(RTMPPacket));
    RTMPPacket_Alloc(packet, len + 9);

    packet->m_body[0] = 0x27;
    if (buf[0] == 0x65) { //关键帧
        packet->m_body[0] = 0x17;
        LOGI("发送关键帧 data");
    }
    packet->m_body[1] = 0x01;
    packet->m_body[2] = 0x00;
    packet->m_body[3] = 0x00;
    packet->m_body[4] = 0x00;
    //h264 nalu裸数据长度
    packet->m_body[5] = (len >> 24) & 0xff;
    packet->m_body[6] = (len >> 16) & 0xff;
    packet->m_body[7] = (len >> 8) & 0xff;
    packet->m_body[8] = (len) & 0xff;

    //数据
    memcpy(&packet->m_body[9], buf, len);


    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = body_size;
    packet->m_nChannel = 0x04;
    packet->m_nTimeStamp = tms;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet->m_nInfoField2 = live->rtmp->m_stream_id;
    return packet;

}

/**
 * Rtmp 协议格式
 * 关键帧    0x17 0x01 0x00 0x00 0x00 4字节数据长度 h264裸数据
 * 非关键帧  0x27 0x01 0x00 0x00 0x00 4字节数据长度 h264裸数据
 * sps与pps 0x17 0x01 0x00 0x00 0x00 sps+pps数据
 */
/**
 * 视频信息，1个字节。
前4位为帧类型Frame Type

值	类型
1	keyframe (for AVC, a seekable frame) 关键帧
2	inter frame (for AVC, a non-seekable frame)
3	disposable inter frame (H.263 only)
4	generated keyframe (reserved for server use only)
5	video info/command frame

后4位为编码ID (CodecID)
值	类型
1	JPEG (currently unused)
2	Sorenson H.263
3	Screen video
4	On2 VP6
5	On2 VP6 with alpha channel
6	Screen video version 2
7	AVC

特殊情况

视频的格式(CodecID)是AVC（H.264）的话，VideoTagHeader会多出4个字节的信息，AVCPacketType 和CompositionTime。

AVCPacketType 占1个字节
值	类型
0	AVCDecoderConfigurationRecord(AVC sequence header)
1	AVC NALU
2	AVC end of sequence (lower level NALU sequence ender is not required or supported)

AVCDecoderConfigurationRecord.包含着是H.264解码相关比较重要的sps和pps信息，再给AVC解码器送数据流之前一定要把sps和pps信息送出，否则的话解码器不能正常解码。而且在解码器stop之后再次start之前，如seek、快进快退状态切换等，都需要重新送一遍sps和pps的信息.AVCDecoderConfigurationRecord在FLV文件中一般情况也是出现1次，也就是第一个video tag.

CompositionTime 占3个字节
条件	            值
AVCPacketType ==1	Composition time offset
AVCPacketType !=1	0

作者：第八区
链接：https://www.jianshu.com/p/7ffaec7b3be6
来源：简书
著作权归作者所有。商业转载请联系作者获得授权，非商业转载请注明出处。
 */

/**
 * SPS PPS数据包
 */
RTMPPacket *createVideoPackage(Live *live) {
    int body_size = 13 + live->sps_len + 3 + live->pps_len;//13+3是下面packet->m_body[i++]个数
    LOGI("createVideoPackage------body_size: %d",body_size);
    RTMPPacket *packet = (RTMPPacket *) malloc(sizeof(RTMPPacket));
    RTMPPacket_Alloc(packet, body_size);
    int i = 0;
    //AVC sequence header 与IDR一样
    packet->m_body[i++] = 0x17;
    //AVC sequence header 设置为0x00
    packet->m_body[i++] = 0x00;//AVCPacketType
    //CompositionTime
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;
    //AVC sequence header
    packet->m_body[i++] = 0x01;   //configurationVersion 版本号 1
    LOGI("createVideoPackage------live->sps[1]: %d,%c,%x",live->sps[1],live->sps[1],live->sps[1]);
    packet->m_body[i++] = live->sps[1]; //profile 如baseline、main、 high

    packet->m_body[i++] = live->sps[2]; //profile_compatibility 兼容性
    packet->m_body[i++] = live->sps[3]; //profile level
    packet->m_body[i++] = 0xFF; // reserved（111111） + lengthSizeMinusOne（2位 nal 长度） 总是0xff
    //sps
    packet->m_body[i++] = 0xE1; //reserved（111） + lengthSizeMinusOne（5位 sps 个数） 总是0xe1
    //sps length 2字节
    packet->m_body[i++] = (live->sps_len >> 8) & 0xff; //第0个字节
    packet->m_body[i++] = live->sps_len & 0xff;        //第1个字节
    memcpy(&packet->m_body[i], live->sps, live->sps_len);
    i += live->sps_len;

    /*pps*/
    packet->m_body[i++] = 0x01; //pps number
    //pps length 2字节
    packet->m_body[i++] = (live->pps_len >> 8) & 0xff;
    packet->m_body[i++] = live->pps_len & 0xff;
    memcpy(&packet->m_body[i], live->pps, live->pps_len);

    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = body_size;
    packet->m_nChannel = 0x04;
    packet->m_nTimeStamp = 0;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet->m_nInfoField2 = live->rtmp->m_stream_id;
    return packet;
}

int sendVideo(int8_t *buf, int len, long tms) {
    int ret;
    LOGI("sendVideo======buf[0]:%x, buf[1]:%x, buf[2]:%x, buf[3]:%x, buf[4]:%x, buf[5]:%x",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);//十六进制打印
//    LOGI("sendVideo======buf[0]:%d, buf[1]:%d, buf[2]:%d, buf[3]:%d, buf[4]:%d, buf[5]:%d",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);//101十六进制为65，代表关键帧；103十六进制为67，代表SPS、PPS
//    LOGI("sendVideo======buf[0]:%c, buf[1]:%c, buf[2]:%c, buf[3]:%c, buf[4]:%c, buf[5]:%c",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
    if (buf[4] == 0x67) {//sps pps
        LOGI("sendVideo======sps pps");
        if (live && (!live->pps || !live->sps)) {
            prepareVideo(buf, len, live);

            //放到这里也行---只发一次
            RTMPPacket *packet = createVideoPackage(live);//发送sps pps
            if (!(ret = sendPacket(packet))) {
            }
        }
    } else {
        if (buf[4] == 0x65) {//关键帧 I 帧
            LOGI("sendVideo======关键帧 I 帧");

            //放到这里也行---遇到关键帧就再发一次sps pps
//            RTMPPacket *packet = createVideoPackage(live);//发送sps pps
//            if (!(ret = sendPacket(packet))) {
//            }
        }
        RTMPPacket *packet = createVideoPackage(buf, len, tms, live);
        ret = sendPacket(packet);
    }
    return ret;
}

/**
 * AACAUDIODATA

字段	        字段类型	字段含义
AACPacketType	UI8	    0: AAC sequence header 1: AAC raw
Data	        UI8[n]	如果AACPacketType为0，则为AudioSpecificConfig 如果AACPacketType为1，则为AAC帧数据

作者：第八区
链接：https://www.jianshu.com/p/7ffaec7b3be6
来源：简书
著作权归作者所有。商业转载请联系作者获得授权，非商业转载请注明出处。
 */

/**
 * AUDIO_HEADER_SIZE为2个bytes：
 * soundFormat (4 bits) 声音类型 参见 FlvAudio
 * soundRate (2 bits) 声音采样频率 参加 FlvAudioSampleRate
 * soundSize (1 bit) 声音采样大小 参加 FlvAudioSampleSize
 * soundType (1 bit) 声音的类别 参加 FlvAudioSampleType
 * AACPacketType (1 byte) 0 ＝ AAC sequence header   1 = AAC raw
 */
/**
 *
 字段	        字段类型	字段含义
 SoundFormat	UB[4]	音频格式，重点关注 10 = AAC
                         0 = Linear PCM, platform endian
                         1 = ADPCM
                         2 = MP3
                         3 = Linear PCM, little endian
                         4 = Nellymoser 16-kHz mono
                         5 = Nellymoser 8-kHz mono
                         6 = Nellymoser
                         7 = G.711 A-law logarithmic PCM
                         8 = G.711 mu-law logarithmic PCM
                         9 = reserved
                         10 = AAC
                         11 = Speex
                         14 = MP3 8-Khz
                         15 = Device-specific sound
 SoundRate	    UB[2]	采样率，对AAC来说，永远等于3
                         0 = 5.5-kHz
                         1 = 11-kHz
                         2 = 22-kHz
                         3 = 44-kHz
 SoundSize	   UB[1]	采样精度，对于压缩过的音频，永远是16位
                         0 = snd8Bit
                         1 = snd16Bit
 SoundType	   UB[1]	声道类型，对Nellymoser来说，永远是单声道；对AAC来说，永远是双声道；
                         0 = sndMono 单声道
                         1 = sndStereo 双声道

 作者：第八区
 链接：https://www.jianshu.com/p/7ffaec7b3be6
 来源：简书
 著作权归作者所有。商业转载请联系作者获得授权，非商业转载请注明出处。
 */

//AudioCodec先发送 audio Special config
RTMPPacket *createAudioPacket(int8_t *buf, const int len, const int type, const long tms,
                              Live *live) {

//    组装音频包  两个字节    是固定的   af    如果是第一次发  你就是 01       如果后面   00  或者是 01  aac
    int body_size = len + 2;
    RTMPPacket *packet = (RTMPPacket *) malloc(sizeof(RTMPPacket));
    RTMPPacket_Alloc(packet, body_size);
//         音频头
    packet->m_body[0] = 0xAF;//10101111---SoundFormat:1010=10,SoundRate:11=3,SoundSize:1=1,SoundType:1=1
    if (type == 1) {//自己定义的RTMPPackage.RTMP_PACKET_TYPE_AUDIO_HEAD
//        头
        packet->m_body[1] = 0x00;//如果AACPacketType为0，则为AudioSpecificConfig 如果AACPacketType为1，则为AAC帧数据
    } else {
        packet->m_body[1] = 0x01;//如果AACPacketType为0，则为AudioSpecificConfig 如果AACPacketType为1，则为AAC帧数据
    }
    memcpy(&packet->m_body[2], buf, len);
    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nChannel = 0x05;
    packet->m_nBodySize = body_size;
    packet->m_nTimeStamp = tms;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet->m_nInfoField2 = live->rtmp->m_stream_id;
    return packet;
}

int sendAudio(int8_t *buf, int len, int type, int tms) {
//    创建音频包   如何组装音频包
    RTMPPacket *packet = createAudioPacket(buf, len, type, tms, live);
    int ret = sendPacket(packet);
    return ret;
}


extern "C" JNIEXPORT jstring JNICALL
Java_com_lecture_rtmtscreenlive_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_lecture_rtmtscreenlive_ScreenLive_connect(JNIEnv *env, jobject thiz, jstring url_) {

    // 首先 Java 的转成 C 的字符串，不然无法使用
    const char *url = env->GetStringUTFChars(url_, 0);
    int ret;
    do {
        live = (Live *) malloc(sizeof(Live));
        memset(live, 0, sizeof(Live));
        live->rtmp = RTMP_Alloc();// Rtmp 申请内存
        RTMP_Init(live->rtmp);
        live->rtmp->Link.timeout = 10;// 设置 rtmp 初始化参数，比如超时时间、url
        LOGI("connect %s", url);
        if (!(ret = RTMP_SetupURL(live->rtmp, (char *) url))) break;
        RTMP_EnableWrite(live->rtmp);// 开启 Rtmp 写入
        LOGI("RTMP_Connect");
        if (!(ret = RTMP_Connect(live->rtmp, 0))) break;
        LOGI("RTMP_ConnectStream ");
        if (!(ret = RTMP_ConnectStream(live->rtmp, 0))) break;
        LOGI("connect success");
    } while (0);
    LOGI("\n---------------------connect ret:%d---------------------\n",ret);
    if (!ret && live) {
        free(live);
        live = nullptr;
    }
    LOGI("\n---------------------connect end---------------------\n");

    env->ReleaseStringUTFChars(url_, url);
    return ret;

}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_lecture_rtmtscreenlive_ScreenLive_sendMetaData(JNIEnv *env, jobject thiz,
                                                        jint width,jint height,jint framerate,
                                                        jint audio_sample_rate,jint audio_sample_size,jint audio_channels) {
    RTMPPacket *packet = createMetadataPackage(width,height,framerate,
                                               audio_sample_rate,audio_sample_size,audio_channels,live);
    int ret = sendPacket(packet);
    return ret;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_lecture_rtmtscreenlive_ScreenLive_sendData(JNIEnv *env, jobject thiz, jbyteArray data_,
                                                    jint len, jlong tms, jint type) {
    int ret;
    jbyte *data = env->GetByteArrayElements(data_, NULL);
    switch (type) {
        case 0: //video---//自己定义的RTMP_PACKET_TYPE_VIDEO
            LOGI("send video  lenght :%d", len);
            ret = sendVideo(data, len, tms);
            break;
        default: //audio---//自己定义的RTMPPackage.RTMP_PACKET_TYPE_AUDIO_HEAD和RTMP_PACKET_TYPE_AUDIO_DATA
            ret = sendAudio(data, len, type, tms);
            LOGI("send Audio  lenght :%d", len);
            break;
    }
    env->ReleaseByteArrayElements(data_, data, 0);
    return ret;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_lecture_rtmtscreenlive_ScreenLive_closeRTMP(JNIEnv *env, jobject thiz) {
    LOGI("native-lib========================================================RTMP_Close");

    if (live) {
        if (live->rtmp != NULL) {
            LOGI("native-lib========================================================RTMP_Close 方便二次推流");
            RTMP_Close(live->rtmp);
            RTMP_Free(live->rtmp);
            live->rtmp = NULL;
        }
        LOGI("native-lib========================================================free(live)");
        free(live);
        live = nullptr;
    }

    return TRUE;//记得return返回值
}