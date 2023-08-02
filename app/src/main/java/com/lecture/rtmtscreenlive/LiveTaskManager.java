package com.lecture.rtmtscreenlive;

import android.util.Log;

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

public class LiveTaskManager {

    private static volatile LiveTaskManager instance;
    private static final int CPU_COUNT = Runtime.getRuntime().availableProcessors();
    private static final int CORE_POOL_SIZE = Math.max(2, Math.min(CPU_COUNT - 1, 4));
    private static final int MAXIMUM_POOL_SIZE = CPU_COUNT * 2 + 1;
    private static final int KEEP_ALIVE_SECONDS = 30;
    private static final BlockingQueue<Runnable> sPoolWorkQueue =
            new LinkedBlockingQueue<Runnable>(5);
    private static final ThreadPoolExecutor THREAD_POOL_EXECUTOR;

    static {
        ThreadPoolExecutor threadPoolExecutor = new ThreadPoolExecutor(
                CORE_POOL_SIZE, MAXIMUM_POOL_SIZE, KEEP_ALIVE_SECONDS, TimeUnit.SECONDS,
                sPoolWorkQueue);
        threadPoolExecutor.allowCoreThreadTimeOut(true);
        THREAD_POOL_EXECUTOR = threadPoolExecutor;
    }

    private LiveTaskManager() {

    }

    public static LiveTaskManager getInstance() {
        if (instance == null) {
            synchronized (LiveTaskManager.class) {
                if (instance == null) {
                    instance = new LiveTaskManager();
                }
            }
        }
        return instance;
    }

    public void execute(Runnable runnable) {
        THREAD_POOL_EXECUTOR.execute(runnable);
    }

    //shutdown之后不能重启THREAD_POOL_EXECUTOR，否则java.util.concurrent.RejectedExecutionException
    public void shutdown() {
        Log.e("TAG","before isShutdown--->"+THREAD_POOL_EXECUTOR.isShutdown()+"---isTerminated--->"+THREAD_POOL_EXECUTOR.isTerminated()+"---isTerminating--->"+THREAD_POOL_EXECUTOR.isTerminating());
        Log.e("TAG","before getTaskCount--->"+THREAD_POOL_EXECUTOR.getTaskCount()+"---getCompletedTaskCount--->"+THREAD_POOL_EXECUTOR.getCompletedTaskCount()+"---getActiveCount--->"+THREAD_POOL_EXECUTOR.getActiveCount());

//        THREAD_POOL_EXECUTOR.shutdown();//有时候停止不了
        THREAD_POOL_EXECUTOR.shutdownNow();//直接停止任务，报错InterruptedException但不崩溃

        Log.e("TAG","after  isShutdown--->"+THREAD_POOL_EXECUTOR.isShutdown()+"---isTerminated--->"+THREAD_POOL_EXECUTOR.isTerminated()+"---isTerminating--->"+THREAD_POOL_EXECUTOR.isTerminating());
        Log.e("TAG","after  getTaskCount--->"+THREAD_POOL_EXECUTOR.getTaskCount()+"---getCompletedTaskCount--->"+THREAD_POOL_EXECUTOR.getCompletedTaskCount()+"---getActiveCount--->"+THREAD_POOL_EXECUTOR.getActiveCount());
    }

}