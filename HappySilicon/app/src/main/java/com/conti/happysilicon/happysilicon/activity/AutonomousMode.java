package com.conti.happysilicon.happysilicon.activity;

import android.os.Bundle;
import android.widget.Button;
import android.widget.TextView;
import android.os.Handler;

import androidx.appcompat.app.AppCompatActivity;  // Updated import

import com.conti.happysilicon.happysilicon.MyApp;
import com.conti.happysilicon.happysilicon.model.Car;
import com.conti.happysilicon.happysilicon.R;
import com.conti.happysilicon.happysilicon.network.TcpSocketClient;

import java.nio.charset.StandardCharsets;

public class AutonomousMode extends AppCompatActivity {
    private Car carModel;
    private TcpSocketClient tcpClient;
    private Button startButton;
    private Button stopButton;
    private TextView carBatteryTextView;
    private TextView carTemperatureTextView;
    private TextView carChargingTextView;
    private TextView carSpeedTextView;
    private TextView carDistanceTextView;
    private Handler handler = new Handler();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_autonomous_mode);

        carModel = Car.getInstance();
        MyApp app = (MyApp) getApplicationContext();
        tcpClient = app.getTcpSocketClient();

        startButton = findViewById(R.id.button);
        stopButton = findViewById(R.id.button2);
        carBatteryTextView = findViewById(R.id.textViewBattery);
        carTemperatureTextView = findViewById(R.id.textViewTemp);
        carChargingTextView = findViewById(R.id.textViewCharging);
        carSpeedTextView = findViewById(R.id.textViewCarSpeed);
        carDistanceTextView = findViewById(R.id.textViewDistance);

        // Set up button listeners
        startButton.setOnClickListener(view -> {
            carModel.setCarCommand(Car.CarCommands.START);
            tcpClient.sendMessage("07");
        });

        stopButton.setOnClickListener(view -> {
            carModel.setCarCommand(Car.CarCommands.STOP);
            tcpClient.sendMessage("00");
        });

        // Start the Runnable for updating UI


        // Setup listener for receiving messages
        tcpClient.receiveMessage(new TcpSocketClient.OnMessageReceived() {
            @Override
            public void onMessage(final String message) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        //
                    }
                });
            }
        });
    }

    private class MyRunnable implements Runnable {
        @Override
        public void run() {
            // Update UI elements with data from carModel
            carBatteryTextView.setText(String.valueOf(carModel.getBatteryLevel()));
            carTemperatureTextView.setText(String.valueOf(carModel.getTemperature()));
            carChargingTextView.setText(String.valueOf(carModel.getChargingState()));
            carSpeedTextView.setText(String.valueOf(carModel.getCarSpeed()));
            carDistanceTextView.setText(String.valueOf(carModel.getDistanceTraveled()));

            // Schedule the next update
            handler.postDelayed(this, 100); // Update every second instead of every 100ms
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        handler.removeCallbacksAndMessages(null); // Stop the handler callbacks when the activity is destroyed
        // Consider disconnecting from the TCP client if the app is closing
    }
}