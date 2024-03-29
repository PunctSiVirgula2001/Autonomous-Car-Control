package com.conti.happysilicon.happysilicon.network;

import android.content.Context;
import android.net.wifi.WifiManager;
import android.net.DhcpInfo;
import java.math.BigInteger;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import java.nio.ByteOrder;

public class UdpSocketClient {

    private Context context;
    private DatagramSocket socket;
    private InetAddress serverAddress;
    private int serverPort = 80; // Default port
    public String STATUS;
    public Integer rx = 0;
    public Integer tx = 0;

    public UdpSocketClient(Context context) {
        this.context = context;
    }

    // Initialize the UDP socket and server address
    public void init(OnInitializationComplete onInitializationComplete) {
        new Thread(() -> {
            try {
                tx =0;
                rx =0;
                String serverIP = getGatewayIpAddress();
                if (serverIP != null) {
                    serverAddress = InetAddress.getByName(serverIP);
                    socket = new DatagramSocket(); // UDP socket
                    System.out.println("UDP Socket initialized for server at " + serverIP);
                    // Prepare buffer for receiving confirmation
                    this.sendMessage("ACK 9999");
                    byte[] buf = new byte[1024];
                    DatagramPacket packet = new DatagramPacket(buf, buf.length);
                    // Attempt to receive the confirmation packet
                    socket.receive(packet);
                    String received = new String(packet.getData(), 0, packet.getLength());
                    // Handle the received confirmation
                    if (received.equals("YES")) {
                        STATUS = "UDP Socket initialized for server at " + serverIP;
                    } else {
                        STATUS = "Not ACKNOWLEDGED";
                    }
                    // Callback for successful initialization
                    if (onInitializationComplete != null) {
                        onInitializationComplete.onComplete();
                    }
                }
            } catch (SocketTimeoutException e) {
                System.err.println("Timeout: No confirmation received.");
                STATUS = "Timeout: No confirmation received.";
            } catch (Exception e) {
                e.printStackTrace();
                STATUS = "Initialization Failed";
            }
        }).start();
    }

    // Interface for initialization completion callback
    public interface OnInitializationComplete {
        void onComplete();
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
