package com.conti.happysilicon.happysilicon.test;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import static org.junit.Assert.*;

import com.conti.happysilicon.happysilicon.network.SocketClient;

import java.io.IOException;
import java.net.Socket;

public class SocketTest {
    private MockServer mockServer;
    private SocketClient socketClient;

    @Before
    public void setUp() throws IOException {
        mockServer = new MockServer(65432);
        mockServer.start();  // Or any available port
        assertTrue(mockServer.running);
    }

    @After
    public void tearDown() throws IOException {
        mockServer.stopServer();
        try {
            mockServer.join(2000); // Wait for the server thread to terminate
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
}
