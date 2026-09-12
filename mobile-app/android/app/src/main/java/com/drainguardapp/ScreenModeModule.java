package com.drainguardapp;

import android.app.Activity;

import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContextBaseJavaModule;
import com.facebook.react.bridge.ReactMethod;

public class ScreenModeModule extends ReactContextBaseJavaModule {
  ScreenModeModule(ReactApplicationContext reactContext) {
    super(reactContext);
  }

  @Override
  public String getName() {
    return "ScreenMode";
  }

  @ReactMethod
  public void setCameraMode(boolean enabled) {
    Activity activity = getCurrentActivity();
    if (!(activity instanceof MainActivity)) {
      return;
    }

    activity.runOnUiThread(() -> ((MainActivity) activity).setCameraMode(enabled));
  }
}
