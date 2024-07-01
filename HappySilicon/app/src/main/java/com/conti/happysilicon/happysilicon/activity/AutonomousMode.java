package com.conti.happysilicon.happysilicon.activity;

import android.os.Bundle;
import android.widget.Button;
import android.widget.TextView;
import android.os.Handler;

import androidx.appcompat.app.AppCompatActivity;  // Updated import

import com.conti.happysilicon.happysilicon.MyApp;
import com.conti.happysilicon.happysilicon.model.Car;
import com.conti.happysilicon.happysilicon.R;
import com.conti.happysilicon.happysilicon.network.UdpSocketClient;

public class AutonomousMode extends AppCompatActivity {
    private Car carModel;
    private UdpSocketClient udpClient;
    private Button startButton;
    private Button stopButton;
    private TextView carBatteryTextView;
    private TextView carTemperatureTextView;
    private TextView carSpeedTextView;
    private TextView carDistanceTextView;
    private final Handler handler = new Handler();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_autonomous_mode);

        MyApp app = (MyApp) getApplicationContext();
        udpClient = app.getUdpSocketClient();
        carModel = Car.getInstance();

        //reference to buttons
        startButton = findViewById(R.id.button);
        stopButton = findViewById(R.id.button2);

        //reference to textViews.
        carBatteryTextView = findViewById(R.id.textViewBattery);
        carTemperatureTextView = findViewById(R.id.textViewTemp);
        carSpeedTextView = findViewById(R.id.textViewCarSpeed);
        carDistanceTextView = findViewById(R.id.textViewDistance);

        // Set up button listeners
        startButton.setOnClickListener(view -> {
            udpClient.sendMessage("07");
        });
        stopButton.setOnClickListener(view -> {
            udpClient.sendMessage("00");
        });

        // Start the Runnable for updating UI
        MyRunnable myRunnable = new MyRunnable(handler);
        handler.post(myRunnable);
    }

    private class MyRunnable implements Runnable {
        private final Handler handler;

        MyRunnable(Handler handler) {
            this.handler = handler;
        }

        @Override
        public void run() {
            // Schedule next update with a delay
            handler.postDelayed(this, 10); // Update every second might be enough
            // Update UI elements with data from carModel
            carBatteryTextView.setText(String.valueOf(carModel.getBatteryLevel()));
            carTemperatureTextView.setText(String.valueOf(carModel.getTemperature()));
            carSpeedTextView.setText(String.valueOf(carModel.getCarSpeed()));
            carDistanceTextView.setText(String.valueOf(carModel.getDistSensFw()));
        }
    }
    @Override
    protected void onDestroy() {
        super.onDestroy();
        handler.removeCallbacksAndMessages(null); // Stop the handler callbacks when the activity is destroyed
    }
}