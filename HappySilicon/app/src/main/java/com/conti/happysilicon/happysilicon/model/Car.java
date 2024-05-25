package com.conti.happysilicon.happysilicon.model;

import com.conti.happysilicon.happysilicon.Utilities.SimpleTimer;

public class Car {
    private static Car instance;


    public enum CarDirectionStatus {
        STOP,
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT
    }
    public enum CarCommands{
        START,
        STOP,
        FORWARD,
        BACKWARD
    }
    // Here the start stop command is stored
    private CarCommands carCommand;
    private int timeElapsed;
    private float batteryLevel;
    private boolean chargingState;
    private float carSpeed;     // cm/s
    private float distSensFw;
    private float distSensBw;
    private float roll;
    private float pitch;
    private CarDirectionStatus movingDirection;

    // TODO traffic signs and lights to be implemented

    // Exclusive to Diagnosis Mode
    // Thermals
    private float temperature;
    private float currentDraw;
    private boolean rightLightToggle;
    private boolean leftLightToggle;
    
    // Exclusive to PID Settings
    private float actualKP;
    private float actualKI;
    private float actualKD;
    private float integralValue;
    private float measuredValue;
    private static float time_of_sampling_from_esp = 0;
    private static float value_of_sampling_from_esp = 0;
    private static boolean new_data_from_esp = false;
    private static boolean reset_graph = false;
    public static boolean allowed_to_plot = false;
    private static SimpleTimer timer; // Create a new timer instance


    // TODO other sensors and camera status handling
    // TODO any data from motor controllers and battery charger

    private Car(){

        timeElapsed = 0;
        batteryLevel = 0;
        chargingState = false;
        carSpeed = 0;
        distSensFw = 0;
        distSensBw = 0;
        movingDirection = CarDirectionStatus.STOP;
        temperature = 0;
        currentDraw = 0;
        roll = 0;
        pitch = 0;
        actualKP = 0;
        actualKI = 0;
        actualKD = 0;
        integralValue = 0;
        measuredValue = 0;
        //TODO update car lights state
        rightLightToggle = false;
        leftLightToggle = false;
    }

    public static synchronized Car getInstance() {
        if (instance == null) {
            instance = new Car();
        }
        return instance;
    }
    //Setters
    public void setCarSpeed(float carSpeed) { this.carSpeed = carSpeed; }
    public void setBatteryLevel(float batteryLevel){ this.batteryLevel = batteryLevel; }
    public void setChargingState(boolean state){ this.chargingState = state; }
    public void setTimeElapsed(int timeElapsed){ this.timeElapsed = timeElapsed; }
    public void setDistSensFw(float distSensFw){ this.distSensFw = distSensFw; }
    public void setDistSensBw(float distSensBw){ this.distSensBw = distSensBw; }
    public void setRoll(float roll) {this.roll = roll;}
    public void setPitch(float pitch) {this.pitch = pitch;}
    public void setMovingDirection(CarDirectionStatus movingDirection) { this.movingDirection = movingDirection; }
    public void setTemperature(float temperature) { this.temperature = temperature; }
    public void setCarCommand(CarCommands carCommand){ this.carCommand = carCommand; }
    public void setCarLights(boolean leftLight, boolean rightLight){ this.leftLightToggle = leftLight; this.rightLightToggle = rightLight; }
    public void setActualKP(float KP) {actualKP = KP;}
    public void setActualKI(float KI) {actualKI = KI;}
    public void setActualKD(float KD) {actualKD = KD;}
    public void setIntegralValue(float setPoint) {this.integralValue = setPoint;}
    public void setMeasuredValue(float measuredValue) {this.measuredValue = measuredValue;}
    public static void setTimeOfSamplingFromEsp(float time) {time_of_sampling_from_esp = time;}
    public static void setValueOfSamplingFromEsp(float value) {value_of_sampling_from_esp = value;}
    public static void setNewDataFromEsp(boolean newData) {new_data_from_esp = newData;}
    public static void setResetGraph(boolean reset) {reset_graph = reset;}
    public static void setTimer(SimpleTimer timer1) {timer = timer1;}
    public static void resetTimer() {timer.reset();}
    public static void startTimer() {timer.start();}


    // Getters
    public float getCarSpeed() { return this.carSpeed; }
    public float getBatteryLevel() { return this.batteryLevel; }
    public float getDistSensFw() { return this.distSensFw; }
    public float getDistSensBw() { return this.distSensBw; }
    public float getRoll() {return this.roll;}
    public float getPitch() {return this.pitch;}
    public float getTimeElapsed() { return this.timeElapsed; }
    public float getTemperature() { return this.temperature; }
    public float getCurrentDraw() { return this.currentDraw; }
    public int getCarLightsState(){
        if(this.rightLightToggle&&this.leftLightToggle){
            return 0;
        }else if(this.rightLightToggle&&!this.leftLightToggle){
            return 1;
        }else if(!this.rightLightToggle&&this.leftLightToggle){
            return 2;
        }else{
            return 3;
        }
    }
    public float getActualKP(){return this.actualKP;}
    public float getActualKI(){return this.actualKI;}
    public float getActualKD(){return this.actualKD;}
    public float getIntegralValue(){return this.integralValue;}
    public float getMeasuredValue(){return this.measuredValue;}
    public static float getTimeOfSamplingFromEsp() {return time_of_sampling_from_esp;}
    public static float getValueOfSamplingFromEsp() {return value_of_sampling_from_esp;}
    public static boolean isNewDataFromEsp() {return new_data_from_esp;}
    public static boolean isResetGraph() {return reset_graph;}
    //get elapsed
    public static float getElapsedTimeSecs() {return timer.getElapsedTimeSecs();}
    public static boolean isTimerRunning() {return timer.isRunning();}
    

}
