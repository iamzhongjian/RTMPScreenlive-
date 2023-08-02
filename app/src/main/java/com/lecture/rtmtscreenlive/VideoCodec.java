package com.lecture.rtmtscreenlive;

import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.media.projection.MediaProjection;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Surface;

import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.ByteBuffer;

public class VideoCodec extends Thread {

    private static final String TAG = "------>dddd<---------";
    private final ScreenLive screenLive;

    private MediaProjection mediaProjection;

    private MediaCodec mediaCodec;//硬编
    private boolean isLiving;
    private VirtualDisplay virtualDisplay;//虚拟画布
    private long timeStamp;
    private long startTime;

    public VideoCodec(ScreenLive screenLive) {
        this.screenLive = screenLive;
    }

    public void startLive(MediaProjection mediaProjection) {
        this.mediaProjection = mediaProjection;

        MediaFormat format = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_AVC,
                720,
                1280);
        format.setInteger(MediaFormat.KEY_COLOR_FORMAT,
                MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
        //码率，帧率，分辨率，关键帧间隔
        format.setInteger(MediaFormat.KEY_BIT_RATE, 400_000);
        format.setInteger(MediaFormat.KEY_FRAME_RATE, 15);
        format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1);
        try {
            mediaCodec = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_VIDEO_AVC);//手机
            mediaCodec.configure(format, null, null,
                    MediaCodec.CONFIGURE_FLAG_ENCODE);
            Surface surface = mediaCodec.createInputSurface();

            virtualDisplay = mediaProjection.createVirtualDisplay(
                    "screen-codec",
                    720, 1280, 1,
                    DisplayManager.VIRTUAL_DISPLAY_FLAG_PUBLIC,
                    surface, null, null);

        } catch (IOException e) {
            e.printStackTrace();
        }
//        LiveTaskManager.getInstance().execute(this);
        start();
    }

    @Override
    public void run() {
        isLiving = true;
        mediaCodec.start();
        MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
        while (isLiving) {
            //2000毫秒 手动触发输出关键帧
            if (System.currentTimeMillis() - timeStamp >= 2000) {
                Log.e(TAG, "VideoCodec========手动触发输出关键帧 timeStamp: " + timeStamp);
                Bundle params = new Bundle();
                //立即刷新 让下一帧是关键帧
                params.putInt(MediaCodec.PARAMETER_KEY_REQUEST_SYNC_FRAME, 0);
                mediaCodec.setParameters(params);
                timeStamp = System.currentTimeMillis();
            }
            int index = mediaCodec.dequeueOutputBuffer(bufferInfo, 100000);
            Log.i(TAG, "VideoCodec run: " + index);
            if (index >= 0) {
                // flags 利用位操作，定义的 flag 都是 2 的倍数
                if ((bufferInfo.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) { // 配置相关的内容，也就是 SPS、PPS
                    Log.e(TAG, "VideoCodec========BUFFER_FLAG_CODEC_CONFIG===配置相关的内容，也就是 SPS、PPS");
                } else if ((bufferInfo.flags & MediaCodec.BUFFER_FLAG_KEY_FRAME) != 0) { // 关键帧
                    Log.e(TAG, "VideoCodec========BUFFER_FLAG_KEY_FRAME===关键帧");
                } else {// 非关键帧和SPS、PPS，可能是B帧或者P帧
                    Log.e(TAG, "VideoCodec========可能是B帧或者P帧");
                }
                ByteBuffer buffer = mediaCodec.getOutputBuffer(index);
                MediaFormat mediaFormat = mediaCodec.getOutputFormat(index);
                Log.i(TAG, "VideoCodec========mediaFormat: " + mediaFormat.toString());
                Log.e(TAG, "VideoCodec========bufferInfo.size: " + bufferInfo.size);
                byte[] outData = new byte[bufferInfo.size];
                buffer.get(outData);
                if (startTime == 0) {
                    // 微妙转为毫秒
                    startTime = bufferInfo.presentationTimeUs / 1000;
                }
//                writeContent(outData);
//                writeBytes(outData);
//                包含   分隔符
                RTMPPackage rtmpPackage = new RTMPPackage(outData, (bufferInfo.presentationTimeUs / 1000) - startTime);
                rtmpPackage.setType(RTMPPackage.RTMP_PACKET_TYPE_VIDEO);
                screenLive.addPackage(rtmpPackage);
                mediaCodec.releaseOutputBuffer(index, false);
            }else if(index == MediaCodec.INFO_TRY_AGAIN_LATER){
                Log.e(TAG, "VideoCodec========INFO_TRY_AGAIN_LATER");
            }else if(index == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED){
                Log.e(TAG, "VideoCodec========INFO_OUTPUT_FORMAT_CHANGED");
            }else if(index == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED){
                Log.e(TAG, "VideoCodec========INFO_OUTPUT_BUFFERS_CHANGED");
            }
        }
        Log.e(TAG, "VideoCodec========================================================end");
        isLiving = false;
        startTime = 0;
        mediaCodec.stop();
        mediaCodec.release();
        mediaCodec = null;
        virtualDisplay.release();
        virtualDisplay = null;
        mediaProjection.stop();
        mediaProjection = null;
    }

    public void stopLive(){
        isLiving = false;

        //ScreenLive里的stopLive()是必须的；已经关闭了线程池，加不加这句都行
//        LiveTaskManager.getInstance().shutdown();
    }

    public void writeBytes(byte[] array) {
        FileOutputStream writer = null;
        try {
            // 打开一个写文件器，构造函数中的第二个参数true表示以追加形式写文件
            writer = new FileOutputStream(Environment.getExternalStorageDirectory() + "/codec.h264", true);
            writer.write(array);
            writer.write('\n');


        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                if (writer != null) {
                    writer.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }


    public String writeContent(byte[] array) {
        char[] HEX_CHAR_TABLE = {
                '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
        };
        StringBuilder sb = new StringBuilder();
        for (byte b : array) {
            sb.append(HEX_CHAR_TABLE[(b & 0xf0) >> 4]);
            sb.append(HEX_CHAR_TABLE[b & 0x0f]);
        }

        FileWriter writer = null;
        try {
            // 打开一个写文件器，构造函数中的第二个参数true表示以追加形式写文件
            writer = new FileWriter(Environment.getExternalStorageDirectory() + "/codec.txt", true);
            writer.write(sb.toString());
            writer.write("\n");
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                if (writer != null) {
                    writer.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        return sb.toString();
    }
}
