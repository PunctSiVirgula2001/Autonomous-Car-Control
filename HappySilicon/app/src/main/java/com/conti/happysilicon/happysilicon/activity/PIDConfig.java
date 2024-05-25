package com.conti.happysilicon.happysilicon.activity;

import android.annotation.SuppressLint;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.os.Build;
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

import com.androidplot.xy.BoundaryMode;
import com.androidplot.xy.LineAndPointFormatter;
import com.androidplot.xy.SimpleXYSeries;
import com.androidplot.xy.XYPlot;
import com.androidplot.xy.XYSeries;
import com.androidplot.xy.XYGraphWidget;

import android.graphics.Color;
import android.graphics.Paint;

import com.github.mikephil.charting.data.Entry;


import java.text.DecimalFormat;
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
    private List<Entry> entries = new ArrayList<Entry>();
    private XYPlot plot;



    private ArrayList<Entry> buffer = new ArrayList<>();
    private static final int BUFFER_SIZE = 2; // Update the chart after every 5 entries

    @SuppressLint("MissingInflatedId")
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.pid_settings);

        plot = findViewById(R.id.linearChart);
        configurePlot();

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
            udpClient.sendMessage("00");
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

            if (Car.allowed_to_plot) {
                float time = Car.getTimeOfSamplingFromEsp();
                float data = Car.getValueOfSamplingFromEsp();
                buffer.add(new Entry(time, data));
                if (buffer.size() >= BUFFER_SIZE) {
                    for (Entry entry : buffer) {
                        add_data_to_graph(entry.getX(), entry.getY());
                    }
                    buffer.clear(); // Clear the buffer after updating the chart
                }
                Car.allowed_to_plot = false;
            }

            if (Car.isResetGraph()) {
                Car.setResetGraph(false);
                Car.resetTimer();
                Car.startTimer();
                entries.clear(); // Clear entries when resetting the graph
                plot.clear();    // Clear the plot
                configurePlot(); // Reconfigure the plot
                plot.redraw();   // Redraw the plot
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

    private void configurePlot() {
        plot.setRangeBoundaries(0, 100, BoundaryMode.FIXED);
        plot.setDomainBoundaries(0, 60, BoundaryMode.FIXED);

        // Set the format for the labels
        plot.getGraph().getLineLabelStyle(XYGraphWidget.Edge.LEFT).setFormat(new DecimalFormat("0"));
        plot.getGraph().getLineLabelStyle(XYGraphWidget.Edge.BOTTOM).setFormat(new DecimalFormat("0"));

        // Set the background color of the plot to white
        plot.getGraph().getGridBackgroundPaint().setColor(Color.WHITE);

        // Set the grid lines to another color if needed
        Paint gridLinePaint = new Paint();
        gridLinePaint.setColor(Color.LTGRAY);
        gridLinePaint.setStrokeWidth(6); // Set the grid line thickness
        plot.getGraph().setDomainGridLinePaint(gridLinePaint);
        plot.getGraph().setRangeGridLinePaint(gridLinePaint);

        // Set the border for the plot
        plot.getBorderPaint().setColor(Color.LTGRAY);
        plot.getBorderPaint().setStrokeWidth(10); // Set the border thickness

        // Adjust padding to ensure labels are not cut off
        plot.getGraph().setPaddingLeft(0);
        plot.getGraph().setPaddingRight(30);
        plot.getGraph().setPaddingTop(0);
        plot.getGraph().setPaddingBottom(0);

        // Move the label position by modifying the Paint object
        Paint leftLabelPaint = plot.getGraph().getLineLabelStyle(XYGraphWidget.Edge.LEFT).getPaint();
        leftLabelPaint.setTextAlign(Paint.Align.RIGHT);
        leftLabelPaint.setTextSize(20); // Increase text size for visibility
        plot.getGraph().getLineLabelStyle(XYGraphWidget.Edge.LEFT).setPaint(leftLabelPaint);

        Paint bottomLabelPaint = plot.getGraph().getLineLabelStyle(XYGraphWidget.Edge.BOTTOM).getPaint();
        bottomLabelPaint.setTextAlign(Paint.Align.CENTER);
        bottomLabelPaint.setTextSize(20); // Increase text size for visibility
        plot.getGraph().getLineLabelStyle(XYGraphWidget.Edge.BOTTOM).setPaint(bottomLabelPaint);

        // Set thicker line for the series
        LineAndPointFormatter seriesFormat = new LineAndPointFormatter(Color.RED, null, null, null);
        seriesFormat.getLinePaint().setStrokeWidth(10); // Set the line thickness


        plot.addSeries(new SimpleXYSeries(new ArrayList<>(), SimpleXYSeries.ArrayFormat.Y_VALS_ONLY, "Series"), seriesFormat);
    }



    private void updatePlot() {
        List<Number> xVals = new ArrayList<>();
        List<Number> yVals = new ArrayList<>();

        for (Entry entry : entries) {
            xVals.add(entry.getX());
            yVals.add(entry.getY());
        }

        XYSeries series = new SimpleXYSeries(xVals, yVals, "null");
        LineAndPointFormatter seriesFormat = new LineAndPointFormatter(Color.RED, null, null, null);
        seriesFormat.getLinePaint().setStrokeWidth(10); // Set the line thickness

        plot.clear();
        plot.addSeries(series, seriesFormat);
        plot.redraw();
    }


    @Override
    protected void onResume() {
        super.onResume();
        Car.setResetGraph(true);
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
        updatePlot();
    }


}
