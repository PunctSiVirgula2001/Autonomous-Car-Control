package com.conti.happysilicon.happysilicon.network;

import android.annotation.SuppressLint;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import com.conti.happysilicon.happysilicon.MyApp;
import com.conti.happysilicon.happysilicon.model.Car;
import com.conti.happysilicon.happysilicon.ui.ToastManager;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketTimeoutException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import javax.jmdns.JmDNS;
import javax.jmdns.ServiceEvent;
import javax.jmdns.ServiceListener;

public class SocketClient {
    private static SocketClient instance;
    private DatagramSocket udpSocketsend;
    private DatagramSocket udpSocketreceive;
    private InetAddress serverAddr;
    private int udpPort = 4240;  // The port you're sending to

    private SocketClient() {
        try {
            udpSocketsend = new DatagramSocket();
            udpSocketreceive = new DatagramSocket(udpPort); // Listening on udpPort
            serverAddr = InetAddress.getByName("192.168.240.107");  // Replace with your ESP32 IP
        } catch (Exception e) {
            e.printStackTrace();
        }
    }


    // This AsyncTask listens for incoming UDP messages.
    @SuppressLint("StaticFieldLeak")
    public void startListening() {
        ExecutorService executor = Executors.newSingleThreadExecutor();
        final Handler handler = new Handler(Looper.getMainLooper());

        executor.execute(() -> {
            byte[] receiveBuffer = new byte[1024];

            DatagramPacket receivePacket = new DatagramPacket(receiveBuffer, receiveBuffer.length);
            try {

                while (true) {  // Keep listening indefinitely
                    //Log.d("UDP packet received","fasgsa");
                    if (udpSocketreceive != null && !udpSocketreceive.isClosed()) {
                        try {
                            udpSocketreceive.setSoTimeout(5000);  // Set timeout to 5000 milliseconds (5 seconds)
                            udpSocketreceive.receive(receivePacket);
                            // Process the received packet here
                            final String receivedData = new String(receivePacket.getData(), 0, receivePacket.getLength());

                            // Log the received data for debugging
                            Log.d("UDP packet received", receivedData);

                            // Update the model
                            updateModel(receivedData);
                        } catch (SocketTimeoutException e) {
                            Log.d("startListening", "Receive timed out");
                        } catch (IOException e) {
                            Log.e("startListening", "Error in receiving packet", e);
                        }
                    } else {
                        Log.e("startListening", "Socket is either null or closed");
                    }
                    // This line blocks until a packet is received


                    // If you need to update UI, you can do it here using the handler
                    handler.post(new Runnable() {
                        @Override
                        public void run() {
                            // Update your UI here, if needed
                        }
                    });
                }
            } catch (Exception e) {
                e.printStackTrace();
                // Post a toast or log the exception
                handler.post(new Runnable() {
                    @Override
                    public void run() {
                        postToast("Failed to receive: " + e.getMessage());
                    }
                });
            }
        });
    }



    public static synchronized SocketClient getInstance() {
        if (instance == null) {
            instance = new SocketClient();
        }
        return instance;
    }

    ExecutorService executor = Executors.newSingleThreadExecutor();
    Handler handler = new Handler(Looper.getMainLooper());

    public void sendString(final String message) {
        ExecutorService executor = Executors.newSingleThreadExecutor();
        Handler handler = new Handler(Looper.getMainLooper());

        executor.execute(new Runnable() {
            @Override
            public void run() {
                // Background work.
                try {
                    byte[] buf = message.getBytes();
                    DatagramPacket packet = new DatagramPacket(buf, buf.length, serverAddr, udpPort);
                    udpSocketsend.send(packet);
                } catch (Exception e) {
                    e.printStackTrace();
                    // Optionally, you can post a Toast message here using the handler
                    handler.post(new Runnable() {
                        @Override
                        public void run() {
                            postToast("Failed to send: " + e.getMessage());
                        }
                    });
                }
            }
        });
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

    static int val = 0;
}
