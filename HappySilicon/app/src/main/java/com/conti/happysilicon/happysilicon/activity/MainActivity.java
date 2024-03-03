package com.conti.happysilicon.happysilicon.activity;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

import com.conti.happysilicon.happysilicon.MyApp;
import com.conti.happysilicon.happysilicon.R;
import com.conti.happysilicon.happysilicon.network.TcpSocketClient;
import com.conti.happysilicon.happysilicon.network.UdpSocketClient;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

public class MainActivity extends AppCompatActivity {
    private Button diagnosis_mode_button;
    private Button autonomous_mode_button;
    private Button connect_to_esp_button;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main_page);

        // Initialize TCP Socket Client
        MyApp app = (MyApp) getApplicationContext();
        UdpSocketClient instance = app.getUdpSocketClient();

        connect_to_esp_button = findViewById(R.id.connectESP);
        connect_to_esp_button.setOnClickListener(view -> {
            instance.init();
            runOnUiThread(() -> {
                TextView ack = findViewById(R.id.textACK); // Replace with your actual TextView ID
                ack.setText(instance.STATUS);
            });
        });


        //reference to buttons
        diagnosis_mode_button = findViewById(R.id.diagnosis_button);
        diagnosis_mode_button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //sendUDPMessage("Entering Diagnosis Mode");
                openDiagnosisMode();
            }
        });

        autonomous_mode_button = findViewById(R.id.autonomous_button);
        autonomous_mode_button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //sendUDPMessage("Entering Autonomous Mode");
                openAutonomousMode();
            }
        });
    }
    public void openDiagnosisMode() {
        Intent intent = new Intent(MainActivity.this, DiagnosisMode.class);
        startActivity(intent);
    }

    public void openAutonomousMode() {
        Intent intent = new Intent(MainActivity.this, AutonomousMode.class);
        startActivity(intent);
    }


}
