package com.conti.happysilicon.happysilicon;

import android.app.Application;
import android.content.Context;

import com.conti.happysilicon.happysilicon.network.TcpSocketClient;

public class MyApp extends Application {
    private TcpSocketClient tcpSocketClient;
    private static MyApp instance;

    public static MyApp getInstance() {
        return instance;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        instance = this;
        tcpSocketClient = new TcpSocketClient(this);
    }
    public TcpSocketClient getTcpSocketClient() {
        return tcpSocketClient;
    }
    public Context getContext() {
        return this;
    }
}
