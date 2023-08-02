package com.lecture.rtmtscreenlive;

import androidx.appcompat.app.AppCompatActivity;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.EditText;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.

    //String url = "rtmp://192.168.10.224/live/livestream";

    EditText editText;
    private MediaProjectionManager mediaProjectionManager;
    private MediaProjection mediaProjection;
    private ScreenLive screenLive;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        editText = findViewById(R.id.test_url);
        checkPermission();

    }

    public boolean checkPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && checkSelfPermission(
                Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{
                    Manifest.permission.READ_EXTERNAL_STORAGE,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE,
                    Manifest.permission.RECORD_AUDIO
            }, 1);

        }
        return false;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        Log.e("TAG","onActivityResult resultCode--->"+resultCode+"---data--->"+data);

        if (requestCode == 100 && resultCode == Activity.RESULT_OK) {
//         mediaProjection--->产生录屏数据
            String mUrl = editText.getText().toString();
            if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q){
                Intent service = new Intent(this, ScreenRecordService.class);
                service.putExtra("code", resultCode);
                service.putExtra("data", data);
                service.putExtra("url", mUrl);

                startService(service);
            }else{
                mediaProjection = mediaProjectionManager.getMediaProjection(resultCode,data);
                screenLive = new ScreenLive();
                screenLive.startLive(mUrl, mediaProjection);
            }
        }
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    public void startLive(View view) {
        this.mediaProjectionManager = (MediaProjectionManager) getSystemService(Context.MEDIA_PROJECTION_SERVICE);
        Intent captureIntent = mediaProjectionManager.createScreenCaptureIntent();
        startActivityForResult(captureIntent, 100);
    }

    public void stopLive(View view) {
        Log.e("TAG","\n\n\n\n\n\n=====================================stopLive=====================================\n\n\n\n\n\n");
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q){
            Intent service = new Intent(this, ScreenRecordService.class);
            stopService(service);//在service的onDestroy方法调用screenLive.stopLive()
        }else{
            screenLive.stopLive();
        }
    }

    /**
     * 测试结果：外部子线程的循环停了，但内部子线程还在运行
     * @param view
     */
    public void test_thread_in_thread(View view) {
        boolean isInnerThreadStop = false;

        //外部子线程
        new Thread(new Runnable() {
            @Override
            public void run() {
                //内部子线程
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        //内部子线程的循环
                        while (!isInnerThreadStop){
                            Log.e("TAG","InnerThread currentThread().name:"+Thread.currentThread().getName());
                            try {
                                Thread.sleep(1000);
                            } catch (InterruptedException e) {
                                throw new RuntimeException(e);
                            }
                        }
                    }
                }).start();

                //外部子线程的循环
                int outerThreadLoopCount = 0;
                boolean isOuterThreadStop = false;
                while (!isOuterThreadStop){
                    outerThreadLoopCount++;
                    if(outerThreadLoopCount > 5){//退出循环
                        isOuterThreadStop = true;
                    }
                    Log.e("TAG","OuterThread currentThread().name:"+Thread.currentThread().getName()+", outerThreadLoopCount:"+outerThreadLoopCount);
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        throw new RuntimeException(e);
                    }
                }
            }
        }).start();
    }
}
