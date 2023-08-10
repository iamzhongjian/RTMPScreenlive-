package com.lecture.rtmtscreenlive;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.media.MediaRecorder;
import android.util.Log;

import java.io.IOException;
import java.nio.ByteBuffer;

public class AudioCodec extends Thread {

    private final ScreenLive screenLive;
    MediaCodec mediaCodec;
    private AudioRecord audioRecord;
    boolean isRecording;
    private int minBufferSize;
    long startTime;

    public AudioCodec(ScreenLive screenLive) {
        this.screenLive = screenLive;
    }

    public void startLive() {
        /**
         * 1、准备编码器
         */
        /**
         * 修改了编码器参数记得修改两个地方：
         * 1.AudioSpecificConfig------audioObjectType,samplingFrequencyIndex,channelConfiguration,AOT Specific Config
         * 2.packet->m_body[0]------SoundFormat,SoundRate,SoundSize,SoundType
         */
        int sampleRateInHz = 44100;
        int channelCount = 2;
        //最小缓冲区大小
        minBufferSize = AudioRecord.getMinBufferSize(sampleRateInHz, channelCount == 2 ? AudioFormat.CHANNEL_IN_STEREO : AudioFormat.CHANNEL_IN_MONO,//由于改了编码器的channelCount为2，这里也要改
                AudioFormat.ENCODING_PCM_16BIT);
        Log.e("TAG","minBufferSize--->"+minBufferSize);

        try {
            MediaFormat mediaFormat = MediaFormat.createAudioFormat(MediaFormat.MIMETYPE_AUDIO_AAC,
                    sampleRateInHz, channelCount);
            //编码规格，可以看成质量
            mediaFormat.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC);
            //码率
            mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, 64_000);
            mediaFormat.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, minBufferSize);//编码器要设置KEY_MAX_INPUT_SIZE，不然很容易会BufferOverflowException
            mediaCodec = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_AUDIO_AAC);
            mediaCodec.configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
        } catch (IOException e) {
            e.printStackTrace();
        }

        //创建AudioRecord 录音
        audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, sampleRateInHz, channelCount == 2 ? AudioFormat.CHANNEL_IN_STEREO : AudioFormat.CHANNEL_IN_MONO,//由于改了编码器的channelCount为2，这里也要改
                AudioFormat.ENCODING_PCM_16BIT, minBufferSize);
        audioRecord.startRecording();
        start();
    }

    @Override
    public void run() {
        isRecording = true;

        mediaCodec.start();

        /**
         * AudioSpecificConfig：
         * 十六进制：12       08
         * 二进制：  00010010 00001000
         * audioObjectType: 2  (5bits)编码器类型，比如2表示AAC-LC------00010
         * samplingFrequencyIndex: 4  (4bits)采样率索引值，比如4表示44100------0100
         * channelConfiguration: 1  (4bits)声道配置，比如1代表单声道，front-center------0001
         * AOT Specific Config: (var bits)
         */
        //在获取播放的音频数据之前，先发送 audio Special config
        RTMPPackage rtmpPackage = new RTMPPackage();
//        byte[] audioSpec = {0x12, 0x08};
        byte[] audioSpec = {0x12, 0x10};//由于改了编码器的channelCount为2，这里也要改
        rtmpPackage.setBuffer(audioSpec);
        rtmpPackage.setType(RTMPPackage.RTMP_PACKET_TYPE_AUDIO_HEAD);
        rtmpPackage.setTms(0);
        screenLive.addPackage(rtmpPackage);


        byte[] buffer = new byte[minBufferSize];
        MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
        while (isRecording) {
            //得到采集的声音数据
            int len = audioRecord.read(buffer, 0, buffer.length);
            Log.e("TAG","buffer.length--->"+buffer.length+"---len--->"+len);
            if (len <= 0) {
                continue;
            }

            // 交给编码器编码

            //获取输入队列中能够使用的容器的下表
            int index = mediaCodec.dequeueInputBuffer(0);
            if (index >= 0) {
                ByteBuffer byteBuffer = mediaCodec.getInputBuffer(index);
                Log.e("TAG","remaining--->"+byteBuffer.remaining()+"---limit--->"+byteBuffer.limit()+"---capacity--->"+byteBuffer.capacity());
                byteBuffer.clear();
                //把输入塞入容器
                byteBuffer.put(buffer, 0, len);//编码器要设置KEY_MAX_INPUT_SIZE，不然很容易会BufferOverflowException

                //通知容器我们使用完了，你可以拿去编码了
                // 时间戳： 微秒， nano纳秒/1000
                mediaCodec.queueInputBuffer(index, 0, len, System.nanoTime() / 1000, 0);
            }


            // 获取编码之后的数据
            index = mediaCodec.dequeueOutputBuffer(bufferInfo, 0);
            // 每次从编码器取完，再往编码器塞数据
            while (index >= 0 && isRecording) {
                ByteBuffer outputBuffer = mediaCodec.getOutputBuffer(index);
                byte[] data = new byte[bufferInfo.size];
                outputBuffer.get(data);

                if (startTime == 0) {
                    startTime = bufferInfo.presentationTimeUs / 1000;
                }


                //todo 送去推流
                rtmpPackage = new RTMPPackage();
                rtmpPackage.setBuffer(data);
                rtmpPackage.setType(RTMPPackage.RTMP_PACKET_TYPE_AUDIO_DATA);
                //相对时间
                rtmpPackage.setTms(bufferInfo.presentationTimeUs/1000 - startTime);
                screenLive.addPackage(rtmpPackage);
                // 释放输出队列，让其能存放新数据
                mediaCodec.releaseOutputBuffer(index, false);

                index = mediaCodec.dequeueOutputBuffer(bufferInfo, 0);
            }

        }
        Log.e("TAG", "AudioCodec========================================================end");
        isRecording = false;
        startTime = 0;
        mediaCodec.stop();
        mediaCodec.release();
        mediaCodec = null;
        audioRecord.stop();
        audioRecord.release();
        audioRecord = null;
    }

    public void stopLive(){
        isRecording = false;
    }
}
