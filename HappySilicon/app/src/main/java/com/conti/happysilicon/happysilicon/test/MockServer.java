package com.conti.happysilicon.happysilicon.test;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;

public class MockServer extends Thread {
    private final int port;
    public volatile boolean running = true;
    private ServerSocket serverSocket;

    public MockServer(int port) {
        this.port = port;
    }

    @Override
    public void run() {
        try {
            serverSocket = new ServerSocket(port);
            while (running) {
                Socket socket = serverSocket.accept();
                // Handle the client socket, if necessary...
            }
        } catch (SocketException e) {
            // Expected when the socket gets closed
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void stopServer() {
        running = false;
        try {
            serverSocket.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}

