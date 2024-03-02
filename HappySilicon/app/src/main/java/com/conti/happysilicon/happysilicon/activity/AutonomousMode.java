package com.conti.happysilicon.happysilicon.activity;

import android.os.Bundle;
import android.widget.Button;
import android.widget.TextView;
import android.os.Handler;

import androidx.appcompat.app.AppCompatActivity;  // Updated import

import com.conti.happysilicon.happysilicon.model.Car;
import com.conti.happysilicon.happysilicon.R;
import com.conti.happysilicon.happysilicon.network.SocketClient;

import java.nio.charset.StandardCharsets;

public class AutonomousMode extends AppCompatActivity {
    private Car carModel;
    private SocketClient socketClient;
    private Button startButton;
    private Button stopButton;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Get the car model instance
        carModel = Car.getInstance();
        socketClient=SocketClient.getInstance();
        setContentView(R.layout.activity_autonomous_mode);
        startButton = (Button)findViewById(R.id.button);
        stopButton = (Button)findViewById(R.id.button2);
        final TextView carBatteryTextView = (TextView) findViewById(R.id.textViewBattery);
        final TextView carTemperatureTextView = (TextView) findViewById(R.id.textViewTemp);
        final TextView carChargingTextView = (TextView) findViewById(R.id.textViewCharging);
        final TextView carSpeedTextView = (TextView) findViewById(R.id.textViewCarSpeed);
        final TextView carDistanceTextView = (TextView) findViewById(R.id.textViewDistance);

        final Handler handler = new Handler();
        class MyRunnable implements Runnable {

            private final Handler handler;

            public MyRunnable(Handler handler) {
                this.handler = handler;
            }
            @Override
            public void run() {
                this.handler.postDelayed(this, 100);

                carBatteryTextView.setText(String.valueOf(carModel.getBatteryLevel()));
                carTemperatureTextView.setText(String.valueOf(carModel.getTemperature()));
                carChargingTextView.setText(String.valueOf(carModel.getChargingState()));
                carSpeedTextView.setText(String.valueOf(carModel.getCarSpeed()));
                carDistanceTextView.setText(String.valueOf(carModel.getDistanceTraveled()));

                startButton.setOnClickListener(view -> {
                    carModel.setCarCommand(Car.CarCommands.START);
                    socketClient.sendString("07");
                });
                stopButton.setOnClickListener(view -> {
                    carModel.setCarCommand(Car.CarCommands.STOP);
                    socketClient.sendString("00");
                });
            }
        }
        handler.post(new MyRunnable(handler));
    }
}
