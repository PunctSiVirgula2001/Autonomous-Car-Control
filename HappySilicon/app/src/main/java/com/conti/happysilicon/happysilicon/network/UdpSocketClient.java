package com.conti.happysilicon.happysilicon.network;

import android.content.Context;
import android.net.wifi.WifiManager;
import android.net.DhcpInfo;
import java.math.BigInteger;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.nio.ByteOrder;

public class UdpSocketClient {

    private Context context;
    private DatagramSocket socket;
    private InetAddress serverAddress;
    private int serverPort = 80; // Default port, change as needed

    public UdpSocketClient(Context context) {
        this.context = context;
    }

    // Initialize the UDP socket and server address
    public void init() {
        new Thread(() -> {
            try {
                String serverIP = getGatewayIpAddress();
                if (serverIP != null) {
                    serverAddress = InetAddress.getByName(serverIP);
                    socket = new DatagramSocket(); // UDP socket
                    System.out.println("UDP Socket initialized for server at " + serverIP);
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
                if (socket != null && serverAddress != null) {
                    byte[] data = message.getBytes();
                    DatagramPacket packet = new DatagramPacket(data, data.length, serverAddress, serverPort);
                    socket.send(packet);
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
                byte[] buffer = new byte[1024];
                while (true) {
                    DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
                    socket.receive(packet);
                    String line = new String(packet.getData(), 0, packet.getLength());
                    listener.onMessage(line);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }).start();
    }

    // Close the socket and release resources
    public void close() {
        if (socket != null) {
            socket.close();
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
}
