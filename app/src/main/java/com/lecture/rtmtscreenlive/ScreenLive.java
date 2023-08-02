package com.lecture.rtmtscreenlive;

import android.media.projection.MediaProjection;
import android.util.Log;

import java.util.concurrent.LinkedBlockingQueue;

public class ScreenLive extends Thread {

    static {
        System.loadLibrary("native-lib");
    }

    private static final String TAG = "------>dddd<--------";
    private boolean isLiving;
    private LinkedBlockingQueue<RTMPPackage> queue = new LinkedBlockingQueue<>();
    private String url;
    private MediaProjection mediaProjection;


    public void startLive(String url, MediaProjection mediaProjection) {
        this.url = url;
        this.mediaProjection = mediaProjection;
        LiveTaskManager.getInstance().execute(this);

    }


    public void addPackage(RTMPPackage rtmpPackage) {
        if (!isLiving) {
            Log.i(TAG, "addPackage: ----------->return");
            return;
        }
        queue.add(rtmpPackage);
    }

    @Override
    public void run() {
        //1推送到
        if (!connect(url)) {
            Log.i(TAG, "run: ----------->推送失败");
            return;
        }
        isLiving = true;//放这里OK

        VideoCodec videoCodec = new VideoCodec(this);
        videoCodec.startLive(mediaProjection);

        AudioCodec audioCodec = new AudioCodec(this);
        audioCodec.startLive();

        //不能放这里，因为videoCodec先开始，然后addPackage，因为isLiving还是false，就return了，导致一开始的sps和pps数据没有发送（也就是sendData-sendVideo-prepareVideo没有执行）
//        isLiving = true;
        Log.i(TAG, "run: ----------->推送开始");
        while (isLiving) {
            RTMPPackage rtmpPackage = null;
            try {
                rtmpPackage = queue.take();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            Log.i(TAG, "取出数据");
            if (rtmpPackage.getBuffer() != null && rtmpPackage.getBuffer().length != 0) {
                Log.i(TAG, "ScreenLive run: ----------->推送 " + rtmpPackage.getBuffer().length + "   type:" + rtmpPackage.getType());
                sendData(rtmpPackage.getBuffer(), rtmpPackage.getBuffer()
                        .length, rtmpPackage.getTms(), rtmpPackage.getType());
            }
        }
    }

    //连接RTMP服务器
    private native boolean connect(String url);

    //发送RTMP Data
    private native boolean sendData(byte[] data, int len, long tms, int type);
}
