package com.conti.happysilicon.happysilicon.activity;

import android.os.Bundle;
import android.os.Handler;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

import com.conti.happysilicon.happysilicon.MyApp;
import com.conti.happysilicon.happysilicon.R;
import com.conti.happysilicon.happysilicon.model.Car;
import com.conti.happysilicon.happysilicon.network.TcpSocketClient;

public class DiagnosisMode extends AppCompatActivity {
    int progressDCSeekBar = 0;
    private Car carModel;
    private TcpSocketClient tcpClient;
    private Button buttonStop;
    private Button buttonForward;
    private Button buttonBackward;
    private Button buttonLeftLight;
    private Button buttonRightLight;
    private SeekBar seekBarServoDirection;
    private SeekBar seekBarDcMotor;

    //reference to textViews.
    private TextView carBatteryTextView;
    private TextView carTemperatureTextView;
    private TextView carChargingTextView ;
    private TextView carSpeedTextView ;
    private TextView carDistanceTextView ;
    private TextView carVoltageTextView ;
    private TextView carCurrentTextView ;
    private TextView carMotorPwmDutyTextView ;
    private TextView carTimeElapsedTextView ;
    private final Handler handler = new Handler();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_diagnosis_mode);

        MyApp app = (MyApp) getApplicationContext();
        tcpClient = app.getTcpSocketClient();
        carModel = Car.getInstance();
        //reference to buttons
        buttonStop = (Button)findViewById(R.id.buttonStop);
        buttonForward = (Button)findViewById(R.id.buttonForward);
        buttonBackward = (Button)findViewById(R.id.buttonBackward);
        buttonLeftLight = (Button)findViewById(R.id.buttonLeftLight);
        buttonRightLight = (Button)findViewById(R.id.buttonRightLight);
        //reference to textViews.
        carBatteryTextView = (TextView) findViewById(R.id.textViewBattery);
        carTemperatureTextView = (TextView) findViewById(R.id.textViewTemp);
        carChargingTextView = (TextView) findViewById(R.id.textViewCharging);
        carSpeedTextView = (TextView) findViewById(R.id.textViewCarSpeed);
        carDistanceTextView = (TextView) findViewById(R.id.textViewDistance);
        carVoltageTextView = (TextView) findViewById(R.id.textViewVoltage);
        carCurrentTextView = (TextView) findViewById(R.id.textViewCurrent);
        carMotorPwmDutyTextView = (TextView) findViewById(R.id.textViewStepperMotorTxt);
        carTimeElapsedTextView = (TextView) findViewById(R.id.textViewTimeElapsed);
        //reference to seekbars
        seekBarServoDirection = (SeekBar) findViewById(R.id.seekBarDirection);
        seekBarDcMotor = (SeekBar) findViewById(R.id.seekBarDcMotor);

        // Set up button listeners
        buttonStop.setOnClickListener(view -> {
            carModel.setCarCommand(Car.CarCommands.STOP);
            tcpClient.sendMessage("00");
        });
        buttonForward.setOnClickListener(view -> {
            carModel.setCarCommand(Car.CarCommands.FORWARD);
            tcpClient.sendMessage("01");
        });
        buttonBackward.setOnClickListener(view -> {
            carModel.setCarCommand(Car.CarCommands.BACKWARD);
            tcpClient.sendMessage("02");
        });
        buttonLeftLight.setOnClickListener(view -> {
            tcpClient.sendMessage("03");
        });
        buttonRightLight.setOnClickListener(view -> {
            tcpClient.sendMessage("04");
        });

        // Set up seekbar listeners
        seekBarDcMotor.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {

            @Override
            public void onProgressChanged(SeekBar seekBar, int progressValue, boolean fromUser) {
                progressDCSeekBar = progressValue;
                tcpClient.sendMessage("05"+progressValue);
            }
            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }
            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

                //Toast.makeText(getApplicationContext(), "Stopped tracking seekbar", Toast.LENGTH_SHORT).show();
            }
        });
        seekBarServoDirection.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            int progressDirection = 0;
            @Override
            public void onProgressChanged(SeekBar seekBar, int progressValue, boolean b) {
                tcpClient.sendMessage("06"+progressValue);
            }
            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }
            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                //seekBar.setProgress(50);
            }
        });

        // Start the Runnable for updating UI
        MyRunnable myRunnable = new MyRunnable(handler);
        handler.post(myRunnable);

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

        private final Handler handler;

        public MyRunnable(Handler handler) {
            this.handler = handler;
        }
        @Override
        public void run() {
            // Schedule next update with a delay
            this.handler.postDelayed(this, 10);
            // Update UI elements with data from carModel
            carBatteryTextView.setText(String.valueOf(carModel.getBatteryLevel()));
            carTemperatureTextView.setText(String.valueOf(carModel.getTemperature()));
            carChargingTextView.setText(String.valueOf(carModel.getChargingState()));
            carSpeedTextView.setText(String.valueOf(carModel.getCarSpeed()));
            carDistanceTextView.setText(String.valueOf(carModel.getDistanceTraveled()));
            carVoltageTextView.setText(String.valueOf(carModel.getChargingStationVoltage()));
            carCurrentTextView.setText(String.valueOf(carModel.getCurrentDraw()));
            carMotorPwmDutyTextView.setText(progressDCSeekBar+"%");
            carTimeElapsedTextView.setText(String.valueOf(carModel.getTimeElapsed()));
        }
    }
    @Override
    protected void onDestroy() {
        super.onDestroy();
        handler.removeCallbacksAndMessages(null); // Stop the handler callbacks when the activity is destroyed
        // Consider disconnecting from the TCP client if the app is closing
    }
}
