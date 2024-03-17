package com.conti.happysilicon.happysilicon.model;

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
    private float distanceTraveled;
    private CarDirectionStatus movingDirection;

    // TODO traffic signs and lights to be implemented

    // Exclusive to Diagnosis Mode
    // Thermals
    private float temperature;
    private float currentDraw;
    private float chargingStationVoltage;
    private int chargingTime; // in seconds
    private boolean rightLightToggle;
    private boolean leftLightToggle;
    private float actualKP;
    private float actualKI;
    private float actualKD;
    private float setPoint;
    private float measuredValue;


    // TODO other sensors and camera status handling
    // TODO any data from motor controllers and battery charger

    private Car(){

        timeElapsed = 0;
        batteryLevel = 0;
        chargingState = false;
        carSpeed = 0;
        distanceTraveled = 0;
        movingDirection = CarDirectionStatus.STOP;
        temperature = 0;
        chargingStationVoltage = 0;
        chargingTime = 0;
        currentDraw = 0;
        actualKP = 0;
        actualKI = 0;
        actualKD = 0;
        setPoint = 0;
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
    public void setCarSpeed(float carSpeed) {
        this.carSpeed = carSpeed;
    }
    public void setBatteryLevel(float batteryLevel){
        this.batteryLevel = batteryLevel;
    }
    public void setChargingState(boolean state){
        this.chargingState = state;
    }
    public void setTimeElapsed(int timeElapsed){
        this.timeElapsed = timeElapsed;
    }
    public void setDistanceTraveled(float distanceTraveled){ this.distanceTraveled = distanceTraveled;}
    public void setMovingDirection(CarDirectionStatus movingDirection) { this.movingDirection = movingDirection;}
    public void setTemperature(float temperature) {
        this.temperature = temperature;
    }
    public void setChargingStationVoltage(float chargingStationVoltage) {this.chargingStationVoltage = chargingStationVoltage;}
    public void setChargingTime(int chargingTime) {
        this.chargingTime = chargingTime;
    }
    public void setCarCommand(CarCommands carCommand){
        this.carCommand = carCommand;
    }
    public void setCarLights(boolean leftLight, boolean rightLight){
        this.leftLightToggle = leftLight;
        this.rightLightToggle = rightLight;
    }
    public void setCurrentDraw(float current){
        currentDraw = current;
    }
    public void setActualKP(float KP) {actualKP = KP;}
    public void setActualKI(float KI) {actualKP = KI;}
    public void setActualKD(float KD) {actualKP = KD;}
    public void setSetPoint(float setPoint) {this.setPoint = setPoint;}
    public void setMeasuredValue(float measuredValue) {this.measuredValue = measuredValue;}


    // Getters
    public float getCarSpeed() {
        return this.carSpeed;
    }
    public float getBatteryLevel(){return this.batteryLevel;}
    public boolean getChargingState(){
        return this.chargingState;
    }
    public float getTimeElapsed(){
        return this.timeElapsed;
    }
    public float getDistanceTraveled(){
        return this.distanceTraveled;
    }
    public CarDirectionStatus getMovingDirection() {
        return this.movingDirection;
    }
    public float getTemperature() {
        return this.temperature;
    }
    public float getChargingStationVoltage() {
        return this.chargingStationVoltage;
    }
    public float getChargingTime() {
        return this.chargingTime;
    }
    public float getCurrentDraw(){
        return this.currentDraw;
    }
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
    public float getSetPoint(){return this.setPoint;}
    public float getMeasuredValue(){return this.measuredValue;}
}
