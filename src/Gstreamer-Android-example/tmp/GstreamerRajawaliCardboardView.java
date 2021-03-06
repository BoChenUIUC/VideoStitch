package com.gst_sdk_tutorials.tutorial_5;

import android.content.Context;
import android.util.Log;
import android.view.View;

import org.rajawali3d.cardboard.RajawaliCardboardView;

/**
 * Created by wlee on 2016/4/25.
 */
public class GstreamerRajawaliCardboardView extends RajawaliCardboardView {
    public int media_width = 320;
    public int media_height = 240;

    public GstreamerRajawaliCardboardView(Context context) {
        super(context);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int width = 0, height = 0;
        int wmode = View.MeasureSpec.getMode(widthMeasureSpec);
        int hmode = View.MeasureSpec.getMode(heightMeasureSpec);
        int wsize = View.MeasureSpec.getSize(widthMeasureSpec);
        int hsize = View.MeasureSpec.getSize(heightMeasureSpec);

        Log.i("GStreamer", "onMeasure called with " + media_width + "x" + media_height);
        // Obey width rules
        switch (wmode) {
            case View.MeasureSpec.AT_MOST:
                if (hmode == View.MeasureSpec.EXACTLY) {
                    width = Math.min(hsize * media_width / media_height, wsize);
                    break;
                }
            case View.MeasureSpec.EXACTLY:
                width = wsize;
                break;
            case View.MeasureSpec.UNSPECIFIED:
                width = media_width;
        }

        // Obey height rules
        switch (hmode) {
            case View.MeasureSpec.AT_MOST:
                if (wmode == View.MeasureSpec.EXACTLY) {
                    height = Math.min(wsize * media_height / media_width, hsize);
                    break;
                }
            case View.MeasureSpec.EXACTLY:
                height = hsize;
                break;
            case View.MeasureSpec.UNSPECIFIED:
                height = media_height;
        }

        // Finally, calculate best size when both axis are free
        if (hmode == View.MeasureSpec.AT_MOST && wmode == View.MeasureSpec.AT_MOST) {
            int correct_height = width * media_height / media_width;
            int correct_width = height * media_width / media_height;

            if (correct_height < height)
                height = correct_height;
            else
                width = correct_width;
        }

        // Obey minimum size
        width = Math.max (getSuggestedMinimumWidth(), width);
        height = Math.max (getSuggestedMinimumHeight(), height);
        setMeasuredDimension(width, height);
    }
}
