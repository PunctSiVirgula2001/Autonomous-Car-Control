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

    private void updateModel(String data){
        Car carModel = Car.getInstance();
        int command;
        float commandValue;
        try{
            command = Integer.parseInt(data.substring(0,2));
            commandValue = Float.parseFloat(data.substring(2));
        }
        catch (NumberFormatException | NullPointerException e){
            // if the format of the received data is not good just ignore it
            return;
        }
        switch(command){
            case 0: //Charging pad voltage
                carModel.setChargingStationVoltage(commandValue);
                break;
            case 1:
            case 2:
            case 4: //Current
                carModel.setCurrentDraw(commandValue);
                break;
            case 5:
            case 6:
            case 7: //Time elapsed
                carModel.setTimeElapsed((int)commandValue);
                break;
            case 8: //Speed
                carModel.setCarSpeed(commandValue);
                break;
            case 9: //Charging state
                carModel.setChargingState((int)commandValue == 1);
                break;
            case 10: //Distance traveled
                carModel.setDistanceTraveled(commandValue);
                break;
            case 11: //Battery level
                carModel.setBatteryLevel(commandValue);
                break;
            case 12: //Temperature
                carModel.setTemperature(commandValue);
                break;
            case 13:
            case 14:
            case 15: //Moving direction
                if((int)commandValue==0){
                    carModel.setMovingDirection(Car.CarDirectionStatus.FORWARD);
                }else if((int)commandValue==1){
                    carModel.setMovingDirection(Car.CarDirectionStatus.BACKWARD);
                }else if((int)commandValue==2){
                    carModel.setMovingDirection(Car.CarDirectionStatus.LEFT);
                }else{
                    carModel.setMovingDirection(Car.CarDirectionStatus.RIGHT);
                }
            case 16: //Signs/Traffic Lights detection
                //int detection = ((valueBytes[0] & 0xFF) << 8) | (valueBytes[1] & 0xFF);

                //carModel.setChargingTime(chargingTimeInt);
                break;
            case 17: //Charging remaining time
                carModel.setChargingTime((int)commandValue);
                break;
            default:
                break;
        }
    }
}
