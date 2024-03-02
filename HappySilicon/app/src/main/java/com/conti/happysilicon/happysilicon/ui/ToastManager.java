package com.conti.happysilicon.happysilicon.ui;

import android.content.Context;
import android.os.Looper;
import android.widget.Toast;

import android.os.Handler;

public class ToastManager {
    private static ToastManager instance;
    private Toast mCurrentToast;
    private Handler handler = new Handler(Looper.getMainLooper());  // Handler tied to the main thread

    private ToastManager() { }

    public static synchronized ToastManager getInstance() {
        if (instance == null) {
            instance = new ToastManager();
        }
        return instance;
    }

    public void showToast(Context context, String message) {
        handler.post(() -> {
            if (mCurrentToast != null) {
                mCurrentToast.cancel();
            }
            mCurrentToast = Toast.makeText(context, message, Toast.LENGTH_SHORT);
            mCurrentToast.show();
        });
    }
}


