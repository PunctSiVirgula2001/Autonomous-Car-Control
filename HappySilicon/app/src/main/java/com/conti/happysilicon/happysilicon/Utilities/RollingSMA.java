package com.conti.happysilicon.happysilicon.Utilities;
import java.util.LinkedList;
import java.util.Queue;
public class RollingSMA {
    private final Queue<Double> window = new LinkedList<>();
    private final int period;
    private double sum = 0.0;

    public RollingSMA(int period) {
        if (period <= 0)
            throw new IllegalArgumentException("Period must be greater than 0");
        this.period = period;
    }

    public float next(double value) {
        // Add new value to the sum
        sum += value;
        window.add(value);

        // Remove oldest value if window is full, and adjust sum
        if (window.size() > period) {
            sum -= window.remove();
        }

        // Calculate SMA based on the current sum and window size
        return (float) (sum / window.size());
    }
}
