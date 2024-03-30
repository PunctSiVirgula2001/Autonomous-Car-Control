package com.conti.happysilicon.happysilicon.Utilities;
public class SimpleTimer {
    private long startTime = 0;
    private boolean running = false;

    // Starts the timer
    public void start() {
        this.startTime = System.nanoTime(); // Use nanoTime for better precision
        this.running = true;
    }

    // Returns the elapsed time in seconds since the timer was started
    public float getElapsedTimeSecs() {
        long elapsed;
        if (running) {
            elapsed = (System.nanoTime() - startTime);
        } else {
            elapsed = 0;
        }
        return elapsed / 1_000_000_000.0f; // Convert from nanoseconds to seconds
    }

    // Checks if the timer is running
    public boolean isRunning() {
        return running;
    }

    // Stops the timer
    public void stop() {
        this.running = false;
    }

    // Resets the timer
    public void reset() {
        this.startTime = 0;
        this.running = false;
    }
}
