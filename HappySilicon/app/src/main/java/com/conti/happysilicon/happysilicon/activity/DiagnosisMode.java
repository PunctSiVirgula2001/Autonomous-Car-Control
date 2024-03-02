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
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        MyApp app = (MyApp) getApplicationContext();
        TcpSocketClient instance = app.getTcpSocketClient();

        // Get the car model instance
        carModel = Car.getInstance();
        setContentView(R.layout.activity_diagnosis_mode);

        //reference to buttons
        buttonStop = (Button)findViewById(R.id.buttonStop);
        buttonForward = (Button)findViewById(R.id.buttonForward);
        buttonBackward = (Button)findViewById(R.id.buttonBackward);
        buttonLeftLight = (Button)findViewById(R.id.buttonLeftLight);
        buttonRightLight = (Button)findViewById(R.id.buttonRightLight);

        //reference to textViews.
        final TextView carBatteryTextView = (TextView) findViewById(R.id.textViewBattery);
        final TextView carTemperatureTextView = (TextView) findViewById(R.id.textViewTemp);
        final TextView carChargingTextView = (TextView) findViewById(R.id.textViewCharging);
        final TextView carSpeedTextView = (TextView) findViewById(R.id.textViewCarSpeed);
        final TextView carDistanceTextView = (TextView) findViewById(R.id.textViewDistance);
        final TextView carVoltageTextView = (TextView) findViewById(R.id.textViewVoltage);
        final TextView carCurrentTextView = (TextView) findViewById(R.id.textViewCurrent);
        final TextView carMotorPwmDutyTextView = (TextView) findViewById(R.id.textViewStepperMotorTxt);
        final TextView carTimeElapsedTextView = (TextView) findViewById(R.id.textViewTimeElapsed);

        //reference to seekbars
        seekBarServoDirection = (SeekBar) findViewById(R.id.seekBarDirection);
        seekBarDcMotor = (SeekBar) findViewById(R.id.seekBarDcMotor);

        final Handler handler = new Handler();
        class MyRunnable implements Runnable {

            private final Handler handler;

            public MyRunnable(Handler handler) {
                this.handler = handler;
            }
            @Override
            public void run() {
                this.handler.postDelayed(this, 10);

                carBatteryTextView.setText(String.valueOf(carModel.getBatteryLevel()));
                carTemperatureTextView.setText(String.valueOf(carModel.getTemperature()));
                carChargingTextView.setText(String.valueOf(carModel.getChargingState()));
                carSpeedTextView.setText(String.valueOf(carModel.getCarSpeed()));
                carDistanceTextView.setText(String.valueOf(carModel.getDistanceTraveled()));
                carVoltageTextView.setText(String.valueOf(carModel.getChargingStationVoltage()));
                carCurrentTextView.setText(String.valueOf(carModel.getCurrentDraw()));
                carMotorPwmDutyTextView.setText(progressDCSeekBar+"%");
                carTimeElapsedTextView.setText(String.valueOf(carModel.getTimeElapsed()));

                // Buttons
                buttonStop.setOnClickListener(view -> {
                    carModel.setCarCommand(Car.CarCommands.STOP);
                    instance.sendMessage("00");
                });
                buttonForward.setOnClickListener(view -> {
                    carModel.setCarCommand(Car.CarCommands.FORWARD);
                    instance.sendMessage("01");
                });
                buttonBackward.setOnClickListener(view -> {
                    carModel.setCarCommand(Car.CarCommands.BACKWARD);
                    instance.sendMessage("02");
                });
                buttonLeftLight.setOnClickListener(view -> {
                    instance.sendMessage("03");
                });
                buttonRightLight.setOnClickListener(view -> {
                    instance.sendMessage("04");
                });
                seekBarDcMotor.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {

                    @Override
                    public void onProgressChanged(SeekBar seekBar, int progressValue, boolean fromUser) {
                        progressDCSeekBar = progressValue;
                        instance.sendMessage("05"+progressValue);
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
                        instance.sendMessage("06"+progressValue);
                    }

                    @Override
                    public void onStartTrackingTouch(SeekBar seekBar) {

                    }

                    @Override
                    public void onStopTrackingTouch(SeekBar seekBar) {
                        //seekBar.setProgress(50);
                    }
                });
            }
        }
        handler.post(new MyRunnable(handler));
    }
}
