package com.conti.happysilicon.happysilicon;

import android.app.Application;
import android.content.Context;

import com.conti.happysilicon.happysilicon.network.TcpSocketClient;
import com.conti.happysilicon.happysilicon.network.UdpSocketClient;
public class MyApp extends Application {
    private TcpSocketClient tcpSocketClient;
    private UdpSocketClient UdpSocketClient;
    private static MyApp instance;

    public static MyApp getInstance() {
        return instance;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        instance = this;
        //tcpSocketClient = new TcpSocketClient(this);
        UdpSocketClient = new UdpSocketClient(this);
    }
    public TcpSocketClient getTcpSocketClient() {
        return tcpSocketClient;
    }
    public UdpSocketClient getUdpSocketClient() {
        return UdpSocketClient;
    }
    public Context getContext() {
        return this;
    }
}
