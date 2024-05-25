package com.conti.happysilicon.happysilicon.network;

import android.os.Handler;
import android.os.Looper;

import com.conti.happysilicon.happysilicon.MyApp;
import com.conti.happysilicon.happysilicon.model.Car;
import com.conti.happysilicon.happysilicon.ui.ToastManager;

import java.net.InetAddress;

import android.content.Context;
import android.net.DhcpInfo;
import android.net.wifi.WifiManager;
import android.util.Log;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.math.BigInteger;
import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.ByteOrder;

public class TcpSocketClient {

    private final Context context;
    private Socket socket;
    private BufferedReader reader;
    private OutputStream outputStream;
    private InputStream inputStream;
    private final int serverPort = 80; // Default port, change as needed

    public TcpSocketClient(Context context) {
        this.context = context;
    }

    // Connect to the server using the gateway IP as the server address
    public void connect() {
        new Thread(() -> {
            try {
                String serverIP = getGatewayIpAddress();
                if (serverIP != null) {
                    socket = new Socket(serverIP, serverPort);
                    outputStream = socket.getOutputStream();
                    inputStream = socket.getInputStream();
                    reader = new BufferedReader(new InputStreamReader(inputStream));
                    System.out.println("Connected to server at " + serverIP);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }).start();
    }

    // Send a message to the server
    public void sendMessage(final String message) {
        new Thread(() -> {
            try {
                if (socket != null && outputStream != null) {
                    Log.d("Ceva","Trimis");
                    outputStream.write((message + "\n").getBytes());
                    outputStream.flush();
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }).start();
    }

    // Receives messages from the server
    public void receiveMessage(OnMessageReceived listener) {
        new Thread(() -> {
            try {
                String line;
                while ((line = reader.readLine()) != null) {
                    listener.onMessage(line);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }).start();
    }
    // Disconnect from the server and clean up resources
    public void disconnect() {
        try {
            if (socket != null) {
                socket.close();
            }
            if (reader != null) {
                reader.close();
            }
            if (outputStream != null) {
                outputStream.close();
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    // Gets the gateway IP address when connected to the WiFi network
    private String getGatewayIpAddress() {
        WifiManager wifiManager = (WifiManager) context.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        if (wifiManager != null) {
            DhcpInfo dhcpInfo = wifiManager.getDhcpInfo();
            int gatewayIp = dhcpInfo.gateway;
            byte[] ipAddressBytes = BigInteger.valueOf(gatewayIp).toByteArray();
            if (ByteOrder.nativeOrder().equals(ByteOrder.LITTLE_ENDIAN)) {
                reverseArray(ipAddressBytes);
            }
            try {
                return InetAddress.getByAddress(ipAddressBytes).getHostAddress();
            } catch (UnknownHostException e) {
                e.printStackTrace();
            }
        }
        return null;
    }

    // Helper method to reverse byte array (for IP address conversion)
    private void reverseArray(byte[] array) {
        for (int i = 0; i < array.length / 2; i++) {
            byte temp = array[i];
            array[i] = array[array.length - 1 - i];
            array[array.length - 1 - i] = temp;
        }
    }

    // Interface for callback when message received
    public interface OnMessageReceived {
        void onMessage(String message);
    }
    private void postToast(String message) {
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                ToastManager.getInstance().showToast(MyApp.getInstance(), message);
            }
        });
    }

}
