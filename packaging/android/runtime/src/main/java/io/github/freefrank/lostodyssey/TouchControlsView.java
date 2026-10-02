package io.github.freefrank.lostodyssey;

import android.app.AlertDialog;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.View;
import android.widget.CheckBox;
import android.util.SparseArray;

/** Local touch controls layered over SDL's surface; physical pads stay in SDL. */
final class TouchControlsView extends View {
    private static final int UP = 0x0001, DOWN = 0x0002, LEFT = 0x0004, RIGHT = 0x0008;
    private static final int START = 0x0010, BACK = 0x0020;
    private static final int L3 = 0x0040, R3 = 0x0080;
    private static final int LB = 0x0100, RB = 0x0200;
    private static final int A = 0x1000, B = 0x2000, X = 0x4000, Y = 0x8000;
    private static final int LT = 0x10000, RT = 0x20000;
    private static final int STICK_LEFT = 1, STICK_RIGHT = 2, DPAD = 3, BUTTON = 4;

    private static final class Pointer {
        final int kind;
        final int button;
        float x, y;

        Pointer(int kind, int button, float x, float y) {
            this.kind = kind;
            this.button = button;
            this.x = x;
            this.y = y;
        }
    }

    private final RuntimeActivity activity;
    private final SparseArray<Pointer> pointers = new SparseArray<>();
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private boolean enabled;
    private int settingsPointer = -1;
    private float unit, dpadX, dpadY, leftX, leftY, rightX, rightY, faceX, faceY;
    private float settingX, settingY, topY;
    private int shownButtons, shownLt, shownRt, shownLx, shownLy, shownRx, shownRy;

    TouchControlsView(RuntimeActivity activity) {
        super(activity);
        this.activity = activity;
        enabled = activity.getSharedPreferences("touch_controls", Context.MODE_PRIVATE)
            .getBoolean("enabled", true);
        setClickable(true);
        setContentDescription("Touch controller settings");
    }

    @Override
    protected void onSizeChanged(int width, int height, int oldWidth, int oldHeight) {
        super.onSizeChanged(width, height, oldWidth, oldHeight);
        unit = Math.min(48f * getResources().getDisplayMetrics().density,
                        Math.min(height * 0.12f, width * 0.055f));
        dpadX = width * 0.12f; dpadY = height * 0.75f;
        leftX = width * 0.31f; leftY = height * 0.75f;
        rightX = width * 0.69f; rightY = height * 0.75f;
        faceX = width * 0.88f; faceY = height * 0.75f;
        topY = height * 0.22f;
        // The SDL surface can extend behind the status bar on Android 16.
        settingX = width - unit * 1.25f;
        settingY = unit * 1.65f;
        clearTouches();
    }

    private boolean near(float x, float y, float cx, float cy, float radius) {
        return Math.hypot(x - cx, y - cy) <= radius;
    }

    private boolean pill(float x, float y, float cx, float cy) {
        return Math.abs(x - cx) <= unit * 0.72f && Math.abs(y - cy) <= unit * 0.39f;
    }

    private int buttonAt(float x, float y) {
        final float offset = unit * 0.70f;
        if (near(x, y, faceX, faceY + offset, unit * 0.49f)) return A;
        if (near(x, y, faceX + offset, faceY, unit * 0.49f)) return B;
        if (near(x, y, faceX - offset, faceY, unit * 0.49f)) return X;
        if (near(x, y, faceX, faceY - offset, unit * 0.49f)) return Y;
        if (near(x, y, leftX, leftY - unit * 1.75f, unit * 0.43f)) return L3;
        if (near(x, y, rightX, rightY - unit * 1.75f, unit * 0.43f)) return R3;
        if (pill(x, y, getWidth() * 0.46f, topY)) return BACK;
        if (pill(x, y, getWidth() * 0.54f, topY)) return START;
        if (pill(x, y, getWidth() * 0.12f, topY)) return LB;
        if (pill(x, y, getWidth() * 0.26f, topY)) return LT;
        if (pill(x, y, getWidth() * 0.74f, topY)) return RT;
        if (pill(x, y, getWidth() * 0.88f, topY)) return RB;
        return 0;
    }

    private Pointer startPointer(float x, float y) {
        if (near(x, y, leftX, leftY, unit * 1.16f)) return new Pointer(STICK_LEFT, 0, x, y);
        if (near(x, y, rightX, rightY, unit * 1.16f)) return new Pointer(STICK_RIGHT, 0, x, y);
        if (near(x, y, dpadX, dpadY, unit * 1.40f)) return new Pointer(DPAD, 0, x, y);
        int button = buttonAt(x, y);
        return button == 0 ? null : new Pointer(BUTTON, button, x, y);
    }

    private static int stick(float value) {
        if (Math.abs(value) < 0.12f) return 0;
        return Math.round(Math.max(-1f, Math.min(1f, value)) * 32767f);
    }

    private void publish() {
        int buttons = 0, lt = 0, rt = 0, lx = 0, ly = 0, rx = 0, ry = 0;
        boolean leftAssigned = false, rightAssigned = false;
        if (enabled && unit > 0) {
            for (int i = 0; i < pointers.size(); ++i) {
                Pointer pointer = pointers.valueAt(i);
                if (pointer.kind == STICK_LEFT && !leftAssigned) {
                    float dx = (pointer.x - leftX) / unit;
                    float dy = (leftY - pointer.y) / unit;
                    float length = (float) Math.hypot(dx, dy);
                    if (length > 1f) { dx /= length; dy /= length; }
                    lx = stick(dx); ly = stick(dy); leftAssigned = true;
                } else if (pointer.kind == STICK_RIGHT && !rightAssigned) {
                    float dx = (pointer.x - rightX) / unit;
                    float dy = (rightY - pointer.y) / unit;
                    float length = (float) Math.hypot(dx, dy);
                    if (length > 1f) { dx /= length; dy /= length; }
                    rx = stick(dx); ry = stick(dy); rightAssigned = true;
                } else if (pointer.kind == DPAD) {
                    float dx = (pointer.x - dpadX) / unit;
                    float dy = (pointer.y - dpadY) / unit;
                    if (dx < -0.28f) buttons |= LEFT;
                    if (dx > 0.28f) buttons |= RIGHT;
                    if (dy < -0.28f) buttons |= UP;
                    if (dy > 0.28f) buttons |= DOWN;
                } else if (pointer.kind == BUTTON && buttonAt(pointer.x, pointer.y) == pointer.button) {
                    if (pointer.button == LT) lt = 255;
                    else if (pointer.button == RT) rt = 255;
                    else buttons |= pointer.button;
                }
            }
        }
        shownButtons = buttons; shownLt = lt; shownRt = rt;
        shownLx = lx; shownLy = ly; shownRx = rx; shownRy = ry;
        RuntimeActivity.nativeSetTouchInput(buttons, lt, rt, lx, ly, rx, ry);
        invalidate();
    }

    void clearTouches() {
        pointers.clear();
        settingsPointer = -1;
        publish();
    }

    private void setControlsEnabled(boolean value) {
        clearTouches();
        enabled = value;
        activity.getSharedPreferences("touch_controls", Context.MODE_PRIVATE)
            .edit().putBoolean("enabled", value).apply();
        invalidate();
    }

    private void showSettings() {
        clearTouches();
        CheckBox toggle = new CheckBox(activity);
        toggle.setText("Show touch controls");
        toggle.setChecked(enabled);
        toggle.setPadding(Math.round(unit * 0.3f), 0, 0, 0);
        new AlertDialog.Builder(activity)
            .setTitle("Controller settings")
            .setView(toggle)
            .setPositiveButton("Apply", (dialog, which) -> setControlsEnabled(toggle.isChecked()))
            .setNegativeButton("Cancel", null)
            .show();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        // A mouse click belongs to SDL's pointer path, not to the virtual pad.
        if (!event.isFromSource(InputDevice.SOURCE_TOUCHSCREEN)
                && !event.isFromSource(InputDevice.SOURCE_STYLUS)) return false;
        if (event.getToolType(event.getActionIndex()) == MotionEvent.TOOL_TYPE_MOUSE) return false;
        int action = event.getActionMasked();
        int index = event.getActionIndex();
        int id = event.getPointerId(index);
        if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
            float x = event.getX(index), y = event.getY(index);
            if (near(x, y, settingX, settingY, unit * 0.82f)) {
                settingsPointer = id;
                return true;
            }
            if (!enabled) return action == MotionEvent.ACTION_POINTER_DOWN;
            Pointer pointer = startPointer(x, y);
            if (pointer == null) return action == MotionEvent.ACTION_POINTER_DOWN;
            pointers.put(id, pointer);
            publish();
            return true;
        }
        if (action == MotionEvent.ACTION_MOVE) {
            for (int i = 0; i < event.getPointerCount(); ++i) {
                Pointer pointer = pointers.get(event.getPointerId(i));
                if (pointer != null) { pointer.x = event.getX(i); pointer.y = event.getY(i); }
            }
            publish();
            return true;
        }
        if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP) {
            pointers.remove(id);
            if (id == settingsPointer) {
                settingsPointer = -1;
                if (near(event.getX(index), event.getY(index), settingX, settingY, unit * 0.82f)) {
                    performClick();
                    showSettings();
                    return true;
                }
            }
            publish();
            return true;
        }
        if (action == MotionEvent.ACTION_CANCEL) {
            clearTouches();
            return true;
        }
        return super.onTouchEvent(event);
    }

    @Override
    public boolean performClick() {
        super.performClick();
        return true;
    }

    @Override
    protected void onDetachedFromWindow() {
        clearTouches();
        super.onDetachedFromWindow();
    }

    private void drawButton(Canvas canvas, float x, float y, float radius, String label, boolean active) {
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(active ? Color.argb(210, 70, 159, 214) : Color.argb(108, 18, 27, 38));
        canvas.drawCircle(x, y, radius, paint);
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(Math.max(2f, unit * 0.045f));
        paint.setColor(Color.argb(220, 222, 239, 249));
        canvas.drawCircle(x, y, radius, paint);
        paint.setStyle(Paint.Style.FILL);
        paint.setTextAlign(Paint.Align.CENTER);
        paint.setTextSize(unit * 0.48f);
        paint.setFakeBoldText(true);
        paint.setColor(Color.WHITE);
        canvas.drawText(label, x, y - (paint.ascent() + paint.descent()) * 0.5f, paint);
    }

    private void drawPill(Canvas canvas, float x, String label, boolean active) {
        float halfWidth = unit * 0.68f, halfHeight = unit * 0.35f;
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(active ? Color.argb(210, 70, 159, 214) : Color.argb(108, 18, 27, 38));
        canvas.drawRoundRect(x - halfWidth, topY - halfHeight, x + halfWidth,
                             topY + halfHeight, halfHeight, halfHeight, paint);
        paint.setTextAlign(Paint.Align.CENTER);
        paint.setTextSize(unit * 0.29f);
        paint.setFakeBoldText(true);
        paint.setColor(Color.WHITE);
        canvas.drawText(label, x, topY - (paint.ascent() + paint.descent()) * 0.5f, paint);
    }

    private void drawStick(Canvas canvas, float x, float y, int axisX, int axisY) {
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(Color.argb(96, 18, 27, 38));
        canvas.drawCircle(x, y, unit, paint);
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(Math.max(2f, unit * 0.045f));
        paint.setColor(Color.argb(215, 222, 239, 249));
        canvas.drawCircle(x, y, unit, paint);
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(axisX != 0 || axisY != 0
            ? Color.argb(220, 70, 159, 214) : Color.argb(165, 125, 146, 161));
        canvas.drawCircle(x + axisX / 32767f * unit * 0.58f,
                          y - axisY / 32767f * unit * 0.58f, unit * 0.36f, paint);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (unit <= 0) return;
        if (enabled) {
            drawStick(canvas, leftX, leftY, shownLx, shownLy);
            drawStick(canvas, rightX, rightY, shownRx, shownRy);
            drawButton(canvas, leftX, leftY - unit * 1.75f, unit * 0.38f,
                       "L3", (shownButtons & L3) != 0);
            drawButton(canvas, rightX, rightY - unit * 1.75f, unit * 0.38f,
                       "R3", (shownButtons & R3) != 0);
            float d = unit * 0.72f;
            drawButton(canvas, dpadX, dpadY - d, unit * 0.40f, "↑", (shownButtons & UP) != 0);
            drawButton(canvas, dpadX, dpadY + d, unit * 0.40f, "↓", (shownButtons & DOWN) != 0);
            drawButton(canvas, dpadX - d, dpadY, unit * 0.40f, "←", (shownButtons & LEFT) != 0);
            drawButton(canvas, dpadX + d, dpadY, unit * 0.40f, "→", (shownButtons & RIGHT) != 0);
            drawButton(canvas, faceX, faceY + d, unit * 0.47f, "A", (shownButtons & A) != 0);
            drawButton(canvas, faceX + d, faceY, unit * 0.47f, "B", (shownButtons & B) != 0);
            drawButton(canvas, faceX - d, faceY, unit * 0.47f, "X", (shownButtons & X) != 0);
            drawButton(canvas, faceX, faceY - d, unit * 0.47f, "Y", (shownButtons & Y) != 0);
            drawPill(canvas, getWidth() * 0.12f, "LB", (shownButtons & LB) != 0);
            drawPill(canvas, getWidth() * 0.26f, "LT", shownLt != 0);
            drawPill(canvas, getWidth() * 0.46f, "BACK", (shownButtons & BACK) != 0);
            drawPill(canvas, getWidth() * 0.54f, "START", (shownButtons & START) != 0);
            drawPill(canvas, getWidth() * 0.74f, "RT", shownRt != 0);
            drawPill(canvas, getWidth() * 0.88f, "RB", (shownButtons & RB) != 0);
        }
        drawButton(canvas, settingX, settingY, unit * 0.70f,
                   enabled ? "CTRL" : "OFF", false);
    }
}
