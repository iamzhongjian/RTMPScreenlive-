package com.lecture.rtmtscreenlive;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.graphics.BitmapFactory;
import android.hardware.display.VirtualDisplay;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.Build;
import android.os.IBinder;
import android.util.DisplayMetrics;
import android.util.Log;

import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;

import java.util.Objects;

/**
 * @author ZhongJian
 * @time 2021/6/23 20:36
 *
 * Android在8.0开始限制后台服务，启动后台服务需要设置通知栏，使服务变成前台服务。同时在Android 9.0开始注册权限android.permission.FOREGROUND_SERVICE，并授权权限。
 * Android10使用MediaProjection录屏需要在前台服务中进行，需要权限android.permission.FOREGROUND_SERVICE
 * Android10使用MediaProjection录屏需要Service，否则报错Caused by: java.lang.SecurityException: Media projections require a foreground service of type ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PROJECTION
 * https://blog.csdn.net/qq_36332133/article/details/96485285
 * https://zhuanlan.zhihu.com/p/360356420
 *
 * 如要从前台移除服务，请调用 stopForeground()。此方法采用布尔值，指示是否需同时移除状态栏通知。此方法不会停止服务。但是，如果您在服务仍运行于前台时将其停止，则通知也会随之移除。
 * https://developer.android.google.cn/guide/components/services.html
 */

/**
 * Service与Activity之间通信的几种方式
 * 1.通过Binder对象+ServiceConnection（需要使用bindService(Intent,ServiceConnection,int)，startService(Intent)无法传入ServiceConnection）
 * 2.通过BroadcastReceiver(广播)的形式
 */
@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
public class ScreenRecordService extends Service {

    private static final String TAG = ScreenRecordService.class.getSimpleName();
    private MediaProjectionManager mediaProjectionManager;
    private MediaProjection mediaProjection;
    private VirtualDisplay virtualDisplay;
    private ScreenLive screenLive;

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        Log.e(TAG,"onBind--->");
        return null;
    }

    @Override
    public boolean onUnbind(Intent intent) {
        Log.e(TAG,"onUnbind--->");
        return super.onUnbind(intent);
    }

    @Override
    public void onRebind(Intent intent) {
        Log.e(TAG,"onRebind--->");
        super.onRebind(intent);
    }

    @Override
    public void onCreate() {
        super.onCreate();
        Log.e(TAG,"onCreate--->");
    }

    @Override
    public void onDestroy() {//stopService会回调onDestroy方法
        super.onDestroy();
        Log.e(TAG,"onDestroy--->");
        release();
    }

    public void initMediaProjection(Context context, int resultCode, Intent resultData) {
        mediaProjectionManager = (MediaProjectionManager) context.getSystemService(Context.MEDIA_PROJECTION_SERVICE);
        mediaProjection = mediaProjectionManager.getMediaProjection(resultCode, Objects.requireNonNull(resultData));
        Log.e(TAG, "mediaProjection created: " + mediaProjection);
    }

    public void release(){
        if (mediaProjection != null) {
            mediaProjection.stop();
            mediaProjection = null;
        }
        if (virtualDisplay != null) {
            virtualDisplay.release();
            virtualDisplay = null;
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.e(TAG,"onStartCommand flags--->"+flags+"---startId--->"+startId);
        createNotificationChannel();

        int mResultCode = intent.getIntExtra("code", -1);
        Intent mResultData = intent.getParcelableExtra("data");
        String mUrl = intent.getStringExtra("url");
        Log.e(TAG,"onStartCommand mResultCode--->"+mResultCode+"---mResultData--->"+mResultData+"---mUrl--->"+mUrl);

        DisplayMetrics metrics = getResources().getDisplayMetrics();
        int screenWidth = metrics.widthPixels;
        int screenHeight = metrics.heightPixels;
        int screenDensity = metrics.densityDpi;
        Log.e(TAG,"screenWidth--->"+screenWidth+"---screenHeight--->"+screenHeight+"---screenDensity--->"+screenDensity);

        initMediaProjection(this,mResultCode,mResultData);
        screenLive = new ScreenLive();
        screenLive.startLive(mUrl, mediaProjection);

        /**
         * START_STICKY: 粘性的，被意外中止后自动重启，但丢失原来激活它的Intent
         * START_NO_STICKY: 非粘性的，被意外中止后不会重新启动
         * START_REDELIVER_INTENT: 粘性的且重新发送Intent,即被意外中止后自动重启，且该Service组件将得到原来用于激活它的Intent对象
         * START_STICKY_COMPATIBILITY:START_STICKY的兼容版本，并不担保onStartCommand()会被重新调用
         */
        int startResult = super.onStartCommand(intent, flags, startId);//默认是START_STICKY
//        int startResult = Service.START_NO_STICKY;
//        int startResult = Service.START_REDELIVER_INTENT;
        Log.e(TAG,"startResult--->"+startResult);
        return startResult;
    }

    private void createNotificationChannel() {
        Notification.Builder builder = new Notification.Builder(this.getApplicationContext()); //获取一个Notification构造器
        Intent nfIntent = new Intent(this, MainActivity.class); //点击后跳转的界面，可以设置跳转数据

        builder.setContentIntent(PendingIntent.getActivity(this, 0, nfIntent, 0)) // 设置PendingIntent
                .setLargeIcon(BitmapFactory.decodeResource(this.getResources(), R.mipmap.ic_launcher)) // 设置下拉列表中的图标(大图标)
                //.setContentTitle("SMI InstantView") // 设置下拉列表里的标题
                .setSmallIcon(R.mipmap.ic_launcher) // 设置状态栏内的小图标
                .setContentText("is running......") // 设置上下文内容
                .setWhen(System.currentTimeMillis()); // 设置该通知发生的时间

        /*以下是对Android 8.0的适配*/
        //普通notification适配
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            builder.setChannelId("notification_id");
        }
        //前台服务notification适配
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationManager notificationManager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
            NotificationChannel channel = new NotificationChannel("notification_id", "notification_name", NotificationManager.IMPORTANCE_LOW);
            notificationManager.createNotificationChannel(channel);
        }

        Notification notification = builder.build(); // 获取构建好的Notification
        notification.defaults = Notification.DEFAULT_SOUND; //设置为默认的声音
        startForeground(110, notification);//id设置0，前台无效，改成其他数字就好了

    }

    public static class ScreenRecordReceiver extends BroadcastReceiver {

        private static final String TAG = ScreenRecordReceiver.class.getSimpleName();
        public static final String ACTION = "com.zj.zjgrafika.screenrecord.XXX";

        @Override
        public void onReceive(Context context, Intent intent) {
            Log.e(TAG,"onReceive--->");
        }
    }

}
