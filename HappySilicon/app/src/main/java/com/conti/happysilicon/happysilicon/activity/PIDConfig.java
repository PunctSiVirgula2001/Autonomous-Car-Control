package com.conti.happysilicon.happysilicon.activity;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

import com.conti.happysilicon.happysilicon.MyApp;
import com.conti.happysilicon.happysilicon.R;
import com.conti.happysilicon.happysilicon.model.Car;
import com.conti.happysilicon.happysilicon.network.UdpSocketClient;

import java.net.DatagramPacket;

public class PIDConfig extends AppCompatActivity {
    private Car carModel;
    private UdpSocketClient udpClient;
    private Button sendPIDConfig;
    private Button savePIDConfig;
    private TextView actualKP;
    private TextView actualKI;
    private TextView actualKD;
    private TextView setPoint;
    private TextView measuredValue;
    private TextView plainTextKP;
    private TextView plainTextKI;
    private TextView plainTextKD;
    private final Handler handler = new Handler();
    private boolean pid_data_recovered = false;
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
        setPoint = (TextView)findViewById((R.id.textSetpoint));
        measuredValue = (TextView)findViewById((R.id.textMeasuredValue));
        //save and send to esp32
        plainTextKP = (TextView)findViewById((R.id.editTextKP));
        plainTextKI = (TextView)findViewById((R.id.editTextKI));
        plainTextKD = (TextView)findViewById((R.id.editTextKD));
        sendPIDConfig = (Button)findViewById(R.id.buttonSendPidConfig);
        savePIDConfig = (Button)findViewById(R.id.buttonSaveConfig);

        // Start the Runnable for updating UI
        MyRunnable myRunnable = new MyRunnable(handler);
        handler.post(myRunnable);


//        plainTextKP.setOnFocusChangeListener(new View.OnFocusChangeListener() {
//            @Override
//            public void onFocusChange(View v, boolean hasFocus) {
//                if (!hasFocus) {
//                    // The user is done typing.
//                    String text = editText.getText().toString();
//                    // Save the text here using Shared Preferences or any other persistence method
//                }
//            }
//        });
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
//            carModel.setCarCommand(Car.CarCommands.BACKWARD);
            udpClient.sendMessage(PIDString);
            udpClient.tx++;

//            udpClient.receiveMessage(message -> {
//                // Update UI with the received message
//                runOnUiThread(() -> {
//                    udpClient.rx++;
//                });
        });

        sendPIDConfig.setOnClickListener(view -> { // SEND PID CONFIG BUTTON
            String PIDString = getConcatenatedPIDValues();
            Log.d("PID", PIDString + '\n');
//            carModel.setCarCommand(Car.CarCommands.BACKWARD);
            udpClient.sendMessage(PIDString);
            udpClient.tx++;
            // need to make some logic to for the send to finish
            udpClient.receiveMessage(message -> {
                // Update UI with the received message
                runOnUiThread(() -> {
                    Log.i("",message);
                    while(!message.equals("OKPID!"))
                    {
                        Log.i("KP","" + getSaveKP());
                        carModel.setActualKP(getSaveKP()); // it doesn't set the actual kP
                        carModel.setActualKI(getSaveKI());
                        carModel.setActualKD(getSaveKD());
                    }
                });
            });
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
            setPoint.setText(String.valueOf(carModel.getSetPoint()));
            measuredValue.setText(String.valueOf(carModel.getMeasuredValue()));

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
    public String getConcatenatedPIDValues() {
        SharedPreferences sharedPreferences = getSharedPreferences("MySharedPref", MODE_PRIVATE);
        String plainTextKP1 = sharedPreferences.getString("plainTextKP", "DefaultKP");
        String plainTextKI1 = sharedPreferences.getString("plainTextKI", "DefaultKI");
        String plainTextKD1 = sharedPreferences.getString("plainTextKD", "DefaultKD");

        return "08" + plainTextKP1 + " "  + plainTextKI1 + " " + plainTextKD1;
    }
    public float getSaveKP() {
        SharedPreferences sharedPreferences = getSharedPreferences("MySharedPref", MODE_PRIVATE);
        return Float.parseFloat(sharedPreferences.getString("plainTextKP", "DefaultKP"));
    }

    public float getSaveKI() {
        SharedPreferences sharedPreferences = getSharedPreferences("MySharedPref", MODE_PRIVATE);
        return Float.parseFloat(sharedPreferences.getString("plainTextKI", "DefaultKP"));
    }

    public float getSaveKD() {
        SharedPreferences sharedPreferences = getSharedPreferences("MySharedPref", MODE_PRIVATE);
        return Float.parseFloat(sharedPreferences.getString("plainTextKD", "DefaultKP"));
    }
}
