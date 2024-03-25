package com.conti.happysilicon.happysilicon.activity;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

import com.conti.happysilicon.happysilicon.MyApp;
import com.conti.happysilicon.happysilicon.R;
import com.conti.happysilicon.happysilicon.Utilities.RollingSMA;
import com.conti.happysilicon.happysilicon.model.Car;
import com.conti.happysilicon.happysilicon.network.TcpSocketClient;
import com.conti.happysilicon.happysilicon.network.UdpSocketClient;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

public class MainActivity extends AppCompatActivity {
    private Button diagnosis_mode_button;
    private Button autonomous_mode_button;
    private Button connect_to_esp_button;
    private Button pid_settings_button;
    private Car carModel;
    RollingSMA sma;
    static double sumSMA = 0;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main_page);
        sma = new RollingSMA(5); // For a SMA of period 5
        // Initialize TCP Socket Client
        MyApp app = (MyApp) getApplicationContext();
        UdpSocketClient instance = app.getUdpSocketClient();
        carModel = Car.getInstance();
        connect_to_esp_button = findViewById(R.id.connectESP);
        connect_to_esp_button.setOnClickListener(view -> {
            instance.init(() -> {
                runOnUiThread(() -> {
                    TextView ack = findViewById(R.id.textACK);
                    ack.setText(instance.STATUS);
                    instance.tx++;
                });
                // Start listening for messages after initialization is successful
                instance.receiveMessage(message -> {
                    // Update UI with the received message
                    runOnUiThread(() -> {
                        instance.rx++; // count each incomming message from esp32

                        if(message.equals("OKPID!")) {
                            carModel.setActualKP(getSaveKP());
                            carModel.setActualKI(getSaveKI());
                            carModel.setActualKD(getSaveKD());
                        }
                        if((message).startsWith("MEASURED_VALUE"))
                        {
                            String[] splitedValue = message.split(" ");
                            int receivedValue = Integer.parseInt(splitedValue[1]);
                            if(receivedValue != 0)
                                carModel.setCarSpeed(sma.next(receivedValue));
                            else
                                carModel.setCarSpeed(0);
                        }
                        if((message).startsWith("I_TERM_VALUE"))
                        {
                            String[] splitedValue = message.split(" ");
                            int receivedValue = Integer.parseInt(splitedValue[1]);
                            if(receivedValue != 0)
                                carModel.setIntegralValue(receivedValue);
                            else
                                carModel.setIntegralValue(0);
                        }
                        if((message).startsWith("ACTUAL_TIME_OF_SEND"))
                        {
                            String[] splitedValue = message.split(" ");
                            int receivedValue = Integer.parseInt(splitedValue[1]);
                            Log.d("Time ", String.valueOf(receivedValue));
                        }


                    });
                });
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

        pid_settings_button = findViewById(R.id.pid_settings);
        pid_settings_button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //sendUDPMessage("Entering Autonomous Mode");
                openPIDSettings();
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

    public void openPIDSettings() {
        Intent intent = new Intent(MainActivity.this, PIDConfig.class);
        startActivity(intent);
    }

    public float getSaveKP() {
        SharedPreferences sharedPreferences = getSharedPreferences("MySharedPref", MODE_PRIVATE);
        return Float.parseFloat(sharedPreferences.getString("plainTextKP", "DefaultKP"));
    }

    public float getSaveKI() {
        SharedPreferences sharedPreferences = getSharedPreferences("MySharedPref", MODE_PRIVATE);
        return Float.parseFloat(sharedPreferences.getString("plainTextKI", "DefaultKI"));
    }

    public float getSaveKD() {
        SharedPreferences sharedPreferences = getSharedPreferences("MySharedPref", MODE_PRIVATE);
        return Float.parseFloat(sharedPreferences.getString("plainTextKD", "DefaultKD"));
    }

}
