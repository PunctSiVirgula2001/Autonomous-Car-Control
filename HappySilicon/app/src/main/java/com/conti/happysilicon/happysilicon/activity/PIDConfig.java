package com.conti.happysilicon.happysilicon.activity;

import android.annotation.SuppressLint;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

import com.conti.happysilicon.happysilicon.MyApp;
import com.conti.happysilicon.happysilicon.R;
import com.conti.happysilicon.happysilicon.model.Car;
import com.conti.happysilicon.happysilicon.network.UdpSocketClient;

import com.github.mikephil.charting.charts.LineChart;
import com.github.mikephil.charting.components.XAxis;
import com.github.mikephil.charting.components.YAxis;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.data.LineData;
import com.github.mikephil.charting.data.LineDataSet;

import java.util.ArrayList;
import java.util.List;

public class PIDConfig extends AppCompatActivity {
    int progressTestDCSeekBar = 0;
    private Car carModel;
    private UdpSocketClient udpClient;
    private Button sendPIDConfig;
    private Button sendTestFWD;
    private Button sendTestBWD;
    private Button sendTestSTOP;
    private SeekBar sendseekBarTestDcMotor;
    private TextView actualKP;
    private TextView actualKI;
    private TextView actualKD;
    private TextView integralValue;
    private TextView testSeekBarTextView;
    private TextView measuredValue;
    private TextView plainTextKP;
    private TextView plainTextKI;
    private TextView plainTextKD;
    private final Handler handler = new Handler();
    private boolean pid_data_recovered = false;
    List<Entry> entries = new ArrayList<Entry>();
    XAxis xAxis;
    YAxis leftAxis;
    YAxis rightAxis;
    LineChart chart;
    LineDataSet dataSet;
    LineData lineData;
    @SuppressLint("MissingInflatedId")
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.pid_settings);

        MyApp app = (MyApp) getApplicationContext();
        udpClient = app.getUdpSocketClient();
        carModel = Car.getInstance();
        //need setters and getters because they are received from esp32
        actualKP = (TextView)findViewById((R.id.textActualKP));
        actualKI = (TextView)findViewById((R.id.textActualKI));
        actualKD = (TextView)findViewById((R.id.textActualKD));
        integralValue = (TextView)findViewById((R.id.textIntegralValue));
        measuredValue = (TextView)findViewById((R.id.textMeasuredValue));
        //save and send to esp32
        plainTextKP = (TextView)findViewById((R.id.editTextKP));
        plainTextKI = (TextView)findViewById((R.id.editTextKI));
        plainTextKD = (TextView)findViewById((R.id.editTextKD));
        testSeekBarTextView = (TextView)findViewById((R.id.textViewTestSeekBarDCMotor));
        sendPIDConfig = (Button)findViewById(R.id.buttonSendPidConfig);
        sendTestFWD = (Button)findViewById(R.id.buttonFWDTest);
        sendTestBWD = (Button)findViewById(R.id.buttonBWDTest);
        sendTestSTOP = (Button) findViewById(R.id.buttonSTOPTest);
        sendseekBarTestDcMotor = (SeekBar) findViewById(R.id.seekBarSendTestDCMotor);

        sendseekBarTestDcMotor.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {   // SEEKBAR MOTOR SPEED
            @Override
            public void onProgressChanged(SeekBar seekBar, int progressValue, boolean fromUser) {
                udpClient.tx++;
                progressTestDCSeekBar = progressValue;
                udpClient.sendMessage("05"+progressValue);
            }
            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }
            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                //Toast.makeText(getApplicationContext(), "Stopped tracking seekbar", Toast.LENGTH_SHORT).show();
            }
        });
        sendTestSTOP.setOnClickListener(view -> {   // TEST STOP BUTTON
            carModel.setCarCommand(Car.CarCommands.STOP);
            udpClient.sendMessage("00");
            udpClient.tx++;
        });
        sendTestFWD.setOnClickListener(view -> {  //TEST FORWARD BUTTON
            carModel.setCarCommand(Car.CarCommands.FORWARD);
            udpClient.sendMessage("01");
            udpClient.tx++;
        });
        sendTestBWD.setOnClickListener(view -> { //TEST BACKWARD BUTTON
            carModel.setCarCommand(Car.CarCommands.BACKWARD);
            udpClient.sendMessage("02");
            udpClient.tx++;
        });

        // Start the Runnable for updating UI
        MyRunnable myRunnable = new MyRunnable(handler);
        handler.post(myRunnable);

        // Create the chart for printing current value for speed.
        chart = findViewById(R.id.lineChart);;
        xAxis = chart.getXAxis();
        leftAxis = chart.getAxisLeft();
        rightAxis = chart.getAxisRight();
        xAxis.setAxisMaximum(60f); // max = 60 seconds
        xAxis.setAxisMinimum(0f);   // min = 0 seconds
        leftAxis.setAxisMaximum(100f); // max = 100
        leftAxis.setAxisMinimum(0f);   // min = 0
        rightAxis.setEnabled(false);

        plainTextKP.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {}
            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {}
            @Override
            public void afterTextChanged(Editable editable) {
                SharedPreferences sharedPreferences = getSharedPreferences("MySharedPref", MODE_PRIVATE);
                SharedPreferences.Editor myEdit = sharedPreferences.edit();
                // Save the current text from the EditText
                myEdit.putString("plainTextKP", editable.toString());
                // Commit the changes
                myEdit.apply();
            }
        });

        plainTextKI.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {}
            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {}
            @Override
            public void afterTextChanged(Editable editable) {
                SharedPreferences sharedPreferences = getSharedPreferences("MySharedPref", MODE_PRIVATE);
                SharedPreferences.Editor myEdit = sharedPreferences.edit();
                // Save the current text from the EditText
                myEdit.putString("plainTextKI", editable.toString());
                // Commit the changes
                myEdit.apply();
            }
        });
        plainTextKD.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {}
            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {}
            @Override
            public void afterTextChanged(Editable editable) {
                SharedPreferences sharedPreferences = getSharedPreferences("MySharedPref", MODE_PRIVATE);
                SharedPreferences.Editor myEdit = sharedPreferences.edit();
                // Save the current text from the EditText
                myEdit.putString("plainTextKD", editable.toString());
                // Commit the changes
                myEdit.apply();
            }
        });

        sendPIDConfig.setOnClickListener(view -> { // SEND PID CONFIG BUTTON
            String PIDString = getConcatenatedPIDValues();
            Log.d("PID", PIDString + '\n');
            udpClient.sendMessage(PIDString);
            udpClient.tx++;
            Car.setResetGraph(true);
            Car.resetTimer();
            Car.startTimer();
        });

    }
    private class MyRunnable implements Runnable 
    {
        private final Handler handler;
        public MyRunnable(Handler handler) 
        {
            this.handler = handler;
        }
        @Override
        public void run() 
        {
            // Schedule next update with a delay
            this.handler.postDelayed(this, 5);
            if(!pid_data_recovered)
            {
                restoreKPKIKD();
                pid_data_recovered = true;
            }
            // Update UI elements with data from carModel
            //carBatteryTextView.setText(String.valueOf(carModel.getBatteryLevel()));
            actualKP.setText(String.valueOf(carModel.getActualKP()));
            actualKI.setText(String.valueOf(carModel.getActualKI()));
            actualKD.setText(String.valueOf(carModel.getActualKD()));
            integralValue.setText(String.valueOf(carModel.getIntegralValue()));
            measuredValue.setText(String.valueOf(carModel.getCarSpeed()));
            testSeekBarTextView.setText(progressTestDCSeekBar+" Hz");
            if(carModel.isNewDataFromEsp()==true)
            {
                //Log.d("Data from ESP", "Time: " + carModel.getTimeOfSamplingFromEsp() + " Value: " + carModel.getValueOfSamplingFromEsp() + '\n');
                add_data_to_graph(carModel.getTimeOfSamplingFromEsp(), carModel.getValueOfSamplingFromEsp());
                carModel.setNewDataFromEsp(false);
            }
            if(carModel.isResetGraph()==true)
            {
                entries.clear();
                chart.invalidate();
                carModel.setResetGraph(false);
            }

        }
        public void restoreKPKIKD()
        {
            SharedPreferences sharedPreferences = getSharedPreferences("MySharedPref", MODE_PRIVATE);
            String plainTextKP1 = sharedPreferences.getString("plainTextKP", "DefaultKP");
            String plainTextKI1 = sharedPreferences.getString("plainTextKI", "DefaultKI");
            String plainTextKD1 = sharedPreferences.getString("plainTextKD", "DefaultKD");
            plainTextKP.setText(plainTextKP1);
            plainTextKI.setText(plainTextKI1);
            plainTextKD.setText(plainTextKD1);

        }
    }

    public String getConcatenatedPIDValues() 
    {
        SharedPreferences sharedPreferences = getSharedPreferences("MySharedPref", MODE_PRIVATE);
        String plainTextKP1 = sharedPreferences.getString("plainTextKP", "DefaultKP");
        String plainTextKI1 = sharedPreferences.getString("plainTextKI", "DefaultKI");
        String plainTextKD1 = sharedPreferences.getString("plainTextKD", "DefaultKD");

        return "08" + plainTextKP1 + " " + plainTextKI1 + " " + plainTextKD1;
    }
    

    public void add_data_to_graph(float time, float data) {
        entries.add(new Entry(time, data));
        if (dataSet == null) {
            dataSet = new LineDataSet(entries, "Measured Value");
            dataSet.setColor(Color.BLUE);
            dataSet.setLineWidth(3f);
            dataSet.setDrawCircles(false); // Disable drawing circles
            dataSet.setMode(LineDataSet.Mode.CUBIC_BEZIER); // Set line mode to cubic bezier
            lineData = new LineData(dataSet);
            chart.setData(lineData);
        } else {
            lineData.notifyDataChanged();
            chart.notifyDataSetChanged();
        }
        
        // Set zoom to horizontal
        chart.zoom(1f, 1f, 0f, 0f);
        
        chart.invalidate();
    }

}
