package io.github.freefrank.lostodyssey;

import android.app.AlertDialog;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.SparseArray;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.View;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;

/** Local touch controls layered over SDL's surface; physical pads stay in SDL. */
final class TouchControlsView extends View {
    private static final int UP = 0x0001, DOWN = 0x0002, LEFT = 0x0004, RIGHT = 0x0008;
    private static final int START = 0x0010, BACK = 0x0020;
    private static final int L3 = 0x0040, R3 = 0x0080;
    private static final int LB = 0x0100, RB = 0x0200;
    private static final int A = 0x1000, B = 0x2000, X = 0x4000, Y = 0x8000;
    private static final String[] NAMES = {
        "LEFT STICK", "RIGHT STICK", "D-PAD", "A", "B", "X", "Y",
        "L3", "R3", "LB", "LT", "BACK", "START", "RT", "RB"
    };

    private static final class Pointer {
        final int element;
        float x, y;

        Pointer(int element, float x, float y) {
            this.element = element;
            this.x = x;
            this.y = y;
        }
    }

    private final RuntimeActivity activity;
    private final SharedPreferences preferences;
    private final SparseArray<Pointer> pointers = new SparseArray<>();
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private TouchControlLayout layout, draft;
    private boolean enabled, editing, customLayout;
    private int settingsPointer = -1, dragPointer = -1, toolbarPointer = -1;
    private int selected = -1, toolbarPressed = -1;
    private float dragOffsetX, dragOffsetY, baseUnit;
    private int shownButtons, shownLt, shownRt, shownLx, shownLy, shownRx, shownRy;

    TouchControlsView(RuntimeActivity activity) {
        super(activity);
        this.activity = activity;
        preferences = activity.getSharedPreferences("touch_controls", Context.MODE_PRIVATE);
        enabled = preferences.getBoolean("enabled", true);
        customLayout = preferences.getInt("layout_version", 0) == 1;
        setClickable(true);
        setContentDescription("Touch controller settings");
    }

    private static float bounded(float value, float minimum, float maximum,
                                 float fallback) {
        return Float.isFinite(value) ? Math.max(minimum, Math.min(maximum, value))
            : fallback;
    }

    @Override
    protected void onSizeChanged(int width, int height, int oldWidth, int oldHeight) {
        super.onSizeChanged(width, height, oldWidth, oldHeight);
        if (width <= 0 || height <= 0) return;
        baseUnit = Math.min(width * .0715f, height * .1625f);
        float size = bounded(preferences.getFloat("size", 1f),
                             TouchControlLayout.MIN_SIZE,
                             TouchControlLayout.MAX_SIZE, 1f);
        float opacity = bounded(preferences.getFloat("opacity", 1f),
                                TouchControlLayout.MIN_OPACITY,
                                TouchControlLayout.MAX_OPACITY, 1f);
        TouchControlLayout fresh = TouchControlLayout.defaults(
            width, height, baseUnit * size);
        fresh.size = size;
        fresh.opacity = opacity;
        if (customLayout) {
            for (int id = 0; id < TouchControlLayout.COUNT; ++id) {
                float storedX = preferences.getFloat("x_" + id, fresh.x[id]);
                float storedY = preferences.getFloat("y_" + id, fresh.y[id]);
                if (Float.isFinite(storedX)) fresh.x[id] = storedX;
                if (Float.isFinite(storedY)) fresh.y[id] = storedY;
                fresh.visible[id] = preferences.getBoolean("visible_" + id, true);
            }
        }
        layout = fresh;
        layout.clampAll(width, height, baseUnit * layout.size, 0f);
        if (draft != null) draft.clampAll(width, height, baseUnit * draft.size,
                                          editorReservedBottom());
        clearTouches();
    }

    private float editorReservedBottom() { return getHeight() * .15f; }
    private TouchControlLayout activeLayout() { return editing ? draft : layout; }
    private float unit() { return baseUnit * activeLayout().size; }
    private float px(int id) { return activeLayout().pixelX(id, getWidth()); }
    private float py(int id) { return activeLayout().pixelY(id, getHeight()); }
    private float settingX() { return getWidth() * .5f; }
    private float settingY() { return getHeight() * .111f; }

    private static boolean near(float x, float y, float cx, float cy, float radius) {
        float dx = x - cx, dy = y - cy;
        return dx * dx + dy * dy <= radius * radius;
    }

    private boolean hit(int id, float x, float y) {
        float radius = TouchControlLayout.hitRadius(id) * unit();
        return near(x, y, px(id), py(id), radius);
    }

    private int elementAt(float x, float y, boolean includeHidden) {
        int best = -1;
        float bestDistance = Float.POSITIVE_INFINITY;
        for (int id = 0; id < TouchControlLayout.COUNT; ++id) {
            if (!includeHidden && !activeLayout().visible[id]) continue;
            float radius = TouchControlLayout.hitRadius(id) * unit();
            float dx = x - px(id), dy = y - py(id);
            float distance = (dx * dx + dy * dy) / (radius * radius);
            if (distance <= 1f && distance < bestDistance) {
                best = id;
                bestDistance = distance;
            }
        }
        return best;
    }

    private static int stick(float value) {
        if (Math.abs(value) < .12f) return 0;
        return Math.round(Math.max(-1f, Math.min(1f, value)) * 32767f);
    }

    private static int buttonMask(int element) {
        switch (element) {
            case TouchControlLayout.A: return A;
            case TouchControlLayout.B: return B;
            case TouchControlLayout.X: return X;
            case TouchControlLayout.Y: return Y;
            case TouchControlLayout.L3: return L3;
            case TouchControlLayout.R3: return R3;
            case TouchControlLayout.LB: return LB;
            case TouchControlLayout.RB: return RB;
            case TouchControlLayout.BACK: return BACK;
            case TouchControlLayout.START: return START;
            default: return 0;
        }
    }

    private void publish() {
        int buttons = 0, lt = 0, rt = 0, lx = 0, ly = 0, rx = 0, ry = 0;
        boolean leftAssigned = false, rightAssigned = false;
        if (enabled && !editing && layout != null && baseUnit > 0f) {
            float unit = unit();
            for (int i = 0; i < pointers.size(); ++i) {
                Pointer pointer = pointers.valueAt(i);
                int id = pointer.element;
                if (!layout.visible[id]) continue;
                if (id == TouchControlLayout.LEFT_STICK && !leftAssigned) {
                    float dx = (pointer.x - px(id)) / unit;
                    float dy = (py(id) - pointer.y) / unit;
                    float length = (float) Math.hypot(dx, dy);
                    if (length > 1f) { dx /= length; dy /= length; }
                    lx = stick(dx); ly = stick(dy); leftAssigned = true;
                } else if (id == TouchControlLayout.RIGHT_STICK && !rightAssigned) {
                    float dx = (pointer.x - px(id)) / unit;
                    float dy = (py(id) - pointer.y) / unit;
                    float length = (float) Math.hypot(dx, dy);
                    if (length > 1f) { dx /= length; dy /= length; }
                    rx = stick(dx); ry = stick(dy); rightAssigned = true;
                } else if (id == TouchControlLayout.DPAD) {
                    float dx = (pointer.x - px(id)) / unit;
                    float dy = (pointer.y - py(id)) / unit;
                    if (dx < -.28f) buttons |= LEFT;
                    if (dx > .28f) buttons |= RIGHT;
                    if (dy < -.28f) buttons |= UP;
                    if (dy > .28f) buttons |= DOWN;
                } else if (hit(id, pointer.x, pointer.y)) {
                    if (id == TouchControlLayout.LT) lt = 255;
                    else if (id == TouchControlLayout.RT) rt = 255;
                    else buttons |= buttonMask(id);
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
        settingsPointer = dragPointer = toolbarPointer = -1;
        toolbarPressed = -1;
        publish();
    }

    private void setControlsEnabled(boolean value) {
        clearTouches();
        enabled = value;
        preferences.edit().putBoolean("enabled", value).apply();
        invalidate();
    }

    private void saveLayout() {
        SharedPreferences.Editor editor = preferences.edit().putInt("layout_version", 1);
        for (int id = 0; id < TouchControlLayout.COUNT; ++id) {
            editor.putFloat("x_" + id, layout.x[id]);
            editor.putFloat("y_" + id, layout.y[id]);
            editor.putBoolean("visible_" + id, layout.visible[id]);
        }
        editor.apply();
        customLayout = true;
    }

    private void applySettings(boolean show, int sizePercent, int opacityPercent) {
        clearTouches();
        float size = sizePercent / 100f;
        if (!customLayout) layout = TouchControlLayout.defaults(
            getWidth(), getHeight(), baseUnit * size);
        layout.size = size;
        layout.opacity = opacityPercent / 100f;
        layout.clampAll(getWidth(), getHeight(), baseUnit * layout.size, 0f);
        preferences.edit().putFloat("size", layout.size)
            .putFloat("opacity", layout.opacity).apply();
        setControlsEnabled(show);
    }

    private void showSettings() {
        clearTouches();
        LinearLayout content = new LinearLayout(activity);
        content.setOrientation(LinearLayout.VERTICAL);
        int padding = Math.round(20f * getResources().getDisplayMetrics().density);
        content.setPadding(padding, 0, padding, 0);

        CheckBox toggle = new CheckBox(activity);
        toggle.setText("Show touch controls");
        toggle.setChecked(enabled);
        content.addView(toggle);

        TextView sizeLabel = new TextView(activity);
        SeekBar size = new SeekBar(activity);
        size.setMax(80);
        size.setProgress(Math.round(layout.size * 100f) - 60);
        sizeLabel.setText("Control size: " + (size.getProgress() + 60) + "%");
        size.setOnSeekBarChangeListener(seekListener(value ->
            sizeLabel.setText("Control size: " + (value + 60) + "%")));
        content.addView(sizeLabel);
        content.addView(size);

        TextView opacityLabel = new TextView(activity);
        SeekBar opacity = new SeekBar(activity);
        opacity.setMax(75);
        opacity.setProgress(Math.round(layout.opacity * 100f) - 25);
        opacityLabel.setText("Opacity: " + (opacity.getProgress() + 25) + "%");
        opacity.setOnSeekBarChangeListener(seekListener(value ->
            opacityLabel.setText("Opacity: " + (value + 25) + "%")));
        content.addView(opacityLabel);
        content.addView(opacity);

        new AlertDialog.Builder(activity)
            .setTitle("Controller settings")
            .setView(content)
            .setPositiveButton("Apply", (dialog, which) ->
                applySettings(toggle.isChecked(), size.getProgress() + 60,
                              opacity.getProgress() + 25))
            .setNeutralButton("Edit layout", (dialog, which) -> {
                applySettings(toggle.isChecked(), size.getProgress() + 60,
                              opacity.getProgress() + 25);
                enterEditor();
            })
            .setNegativeButton("Cancel", null)
            .show();
    }

    private interface ProgressChanged { void update(int value); }

    private static SeekBar.OnSeekBarChangeListener seekListener(ProgressChanged listener) {
        return new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar seekBar, int progress,
                                                     boolean fromUser) {
                listener.update(progress);
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        };
    }

    private void enterEditor() {
        clearTouches();
        draft = layout.copy();
        draft.clampAll(getWidth(), getHeight(), baseUnit * draft.size,
                       editorReservedBottom());
        editing = true;
        selected = -1;
        publish();
    }

    private void leaveEditor(boolean save) {
        clearTouches();
        if (save) {
            layout = draft.copy();
            saveLayout();
        }
        editing = false;
        draft = null;
        selected = -1;
        publish();
    }

    private void resetDraft() {
        dragPointer = -1;
        float size = draft.size, opacity = draft.opacity;
        draft = TouchControlLayout.defaults(getWidth(), getHeight(), baseUnit * size);
        draft.size = size;
        draft.opacity = opacity;
        draft.clampAll(getWidth(), getHeight(), baseUnit * size, editorReservedBottom());
        selected = -1;
        invalidate();
    }

    private int toolbarAt(float x, float y) {
        if (Math.abs(y - getHeight() * .91f) > getHeight() * .052f) return -1;
        for (int i = 0; i < 4; ++i) {
            if (Math.abs(x - getWidth() * (.37f + i * .09f)) <= getWidth() * .043f)
                return i;
        }
        return -1;
    }

    private boolean onEditorTouch(MotionEvent event, int action, int index, int id) {
        if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
            float x = event.getX(index), y = event.getY(index);
            int toolbar = toolbarAt(x, y);
            if (toolbar >= 0) {
                if (toolbarPointer == -1) {
                    toolbarPointer = id;
                    toolbarPressed = toolbar;
                }
            } else if (dragPointer == -1) {
                int element = elementAt(x, y, true);
                if (element >= 0) {
                    selected = element;
                    dragPointer = id;
                    dragOffsetX = px(element) - x;
                    dragOffsetY = py(element) - y;
                } else {
                    selected = -1;
                }
            }
            invalidate();
            return true;
        }
        if (action == MotionEvent.ACTION_MOVE) {
            if (dragPointer != -1) {
                for (int i = 0; i < event.getPointerCount(); ++i) {
                    if (event.getPointerId(i) == dragPointer) {
                        draft.move(selected, event.getX(i) + dragOffsetX,
                                   event.getY(i) + dragOffsetY, getWidth(), getHeight(),
                                   unit(), editorReservedBottom());
                        invalidate();
                        break;
                    }
                }
            }
            return true;
        }
        if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP) {
            if (id == dragPointer) dragPointer = -1;
            if (id == toolbarPointer) {
                int pressed = toolbarPressed;
                toolbarPointer = toolbarPressed = -1;
                if (pressed == toolbarAt(event.getX(index), event.getY(index))) {
                    if (pressed == 0) leaveEditor(true);
                    else if (pressed == 1) leaveEditor(false);
                    else if (pressed == 2) resetDraft();
                    else if (pressed == 3 && selected >= 0) {
                        dragPointer = -1;
                        draft.visible[selected] = !draft.visible[selected];
                        invalidate();
                    }
                }
            }
            return true;
        }
        if (action == MotionEvent.ACTION_CANCEL) {
            dragPointer = toolbarPointer = toolbarPressed = -1;
            return true;
        }
        return true;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        // A mouse click belongs to SDL's pointer path, not to the virtual pad.
        if (!event.isFromSource(InputDevice.SOURCE_TOUCHSCREEN)
                && !event.isFromSource(InputDevice.SOURCE_STYLUS)) return false;
        if (event.getToolType(event.getActionIndex()) == MotionEvent.TOOL_TYPE_MOUSE) return false;
        if (layout == null) return false;
        int action = event.getActionMasked();
        int index = event.getActionIndex();
        int id = event.getPointerId(index);
        if (editing) return onEditorTouch(event, action, index, id);
        if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
            float x = event.getX(index), y = event.getY(index);
            if (near(x, y, settingX(), settingY(), unit() * .50f)) {
                settingsPointer = id;
                return true;
            }
            if (!enabled) return action == MotionEvent.ACTION_POINTER_DOWN;
            int element = elementAt(x, y, false);
            if (element < 0) return action == MotionEvent.ACTION_POINTER_DOWN;
            pointers.put(id, new Pointer(element, x, y));
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
                if (near(event.getX(index), event.getY(index), settingX(), settingY(),
                         unit() * .50f)) {
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

    private void color(int value, float opacity) {
        paint.setColor(Color.argb(Math.round(Color.alpha(value) * opacity),
                                  Color.red(value), Color.green(value), Color.blue(value)));
    }

    private void drawButton(Canvas canvas, float x, float y, float radius,
                            String label, boolean active, float opacity) {
        paint.setStyle(Paint.Style.FILL);
        color(active ? Color.argb(190, 70, 159, 214) : Color.argb(90, 0, 0, 0), opacity);
        canvas.drawCircle(x, y, radius, paint);
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(Math.max(2f, unit() * .015f));
        color(Color.argb(210, 222, 239, 249), opacity);
        canvas.drawCircle(x, y, radius, paint);
        paint.setStyle(Paint.Style.FILL);
        paint.setTextAlign(Paint.Align.CENTER);
        paint.setTextSize(unit() * .26f);
        paint.setFakeBoldText(false);
        color(Color.WHITE, opacity);
        canvas.drawText(label, x, y - (paint.ascent() + paint.descent()) * .5f, paint);
    }

    private void drawPill(Canvas canvas, float x, float y, String label,
                          boolean active, float opacity) {
        float halfWidth = unit() * .42f, halfHeight = unit() * .22f;
        paint.setStyle(Paint.Style.FILL);
        color(active ? Color.argb(190, 70, 159, 214) : Color.argb(90, 0, 0, 0), opacity);
        canvas.drawRoundRect(x - halfWidth, y - halfHeight, x + halfWidth,
                             y + halfHeight, halfHeight, halfHeight, paint);
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(Math.max(2f, unit() * .015f));
        color(Color.argb(210, 222, 239, 249), opacity);
        canvas.drawRoundRect(x - halfWidth, y - halfHeight, x + halfWidth,
                             y + halfHeight, halfHeight, halfHeight, paint);
        paint.setStyle(Paint.Style.FILL);
        paint.setTextAlign(Paint.Align.CENTER);
        paint.setTextSize(unit() * .20f);
        paint.setFakeBoldText(true);
        color(Color.WHITE, opacity);
        canvas.drawText(label, x, y - (paint.ascent() + paint.descent()) * .5f, paint);
    }

    private void drawStick(Canvas canvas, float x, float y, int axisX, int axisY,
                           float opacity) {
        float unit = unit();
        paint.setStyle(Paint.Style.FILL);
        color(Color.argb(72, 0, 0, 0), opacity);
        canvas.drawCircle(x, y, unit, paint);
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(Math.max(2f, unit * .015f));
        color(Color.argb(210, 222, 239, 249), opacity);
        canvas.drawCircle(x, y, unit, paint);
        color(Color.argb(92, 222, 239, 249), opacity);
        canvas.drawCircle(x, y, unit * .68f, paint);
        paint.setStyle(Paint.Style.FILL);
        color(axisX != 0 || axisY != 0
            ? Color.argb(220, 70, 159, 214) : Color.argb(180, 175, 185, 191), opacity);
        canvas.drawCircle(x + axisX / 32767f * unit * .55f,
                          y - axisY / 32767f * unit * .55f, unit * .40f, paint);
    }

    private void drawElement(Canvas canvas, int id) {
        TouchControlLayout controls = activeLayout();
        if (!editing && !controls.visible[id]) return;
        float opacity = editing ? (controls.visible[id] ? Math.max(.70f,
            controls.opacity) : .40f) : controls.opacity;
        float x = px(id), y = py(id), unit = unit();
        switch (id) {
            case TouchControlLayout.LEFT_STICK:
                drawStick(canvas, x, y, shownLx, shownLy, opacity); break;
            case TouchControlLayout.RIGHT_STICK:
                drawStick(canvas, x, y, shownRx, shownRy, opacity); break;
            case TouchControlLayout.DPAD:
                drawButton(canvas, x, y - unit * .855f, unit * .45f, "↑",
                           (shownButtons & UP) != 0, opacity);
                drawButton(canvas, x, y + unit * .855f, unit * .45f, "↓",
                           (shownButtons & DOWN) != 0, opacity);
                drawButton(canvas, x - unit * .855f, y, unit * .45f, "←",
                           (shownButtons & LEFT) != 0, opacity);
                drawButton(canvas, x + unit * .855f, y, unit * .45f, "→",
                           (shownButtons & RIGHT) != 0, opacity);
                break;
            case TouchControlLayout.BACK:
            case TouchControlLayout.START:
                drawPill(canvas, x, y, NAMES[id], (shownButtons & buttonMask(id)) != 0,
                         opacity);
                break;
            default:
                boolean active = id == TouchControlLayout.LT ? shownLt != 0
                    : id == TouchControlLayout.RT ? shownRt != 0
                    : (shownButtons & buttonMask(id)) != 0;
                drawButton(canvas, x, y, unit * .45f, NAMES[id], active, opacity);
                break;
        }
        if (editing && id == selected) {
            paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeWidth(Math.max(3f, unit * .025f));
            color(Color.argb(230, 255, 195, 67), 1f);
            canvas.drawCircle(x, y, TouchControlLayout.hitRadius(id) * unit * 1.08f, paint);
        }
    }

    private void drawEditorToolbar(Canvas canvas) {
        String description = selected < 0 ? "Drag a control · tap to select"
            : NAMES[selected] + (draft.visible[selected] ? " · visible" : " · hidden");
        paint.setStyle(Paint.Style.FILL);
        paint.setTextAlign(Paint.Align.CENTER);
        paint.setTextSize(Math.max(18f, unit() * .16f));
        paint.setFakeBoldText(true);
        color(Color.argb(240, 255, 255, 255), 1f);
        canvas.drawText(description, getWidth() * .5f, getHeight() * .82f, paint);
        for (int i = 0; i < 4; ++i) {
            float x = getWidth() * (.37f + i * .09f);
            float y = getHeight() * .91f;
            float halfWidth = getWidth() * .043f;
            float halfHeight = getHeight() * .045f;
            paint.setStyle(Paint.Style.FILL);
            color(Color.argb(i == toolbarPressed ? 220 : 185, 22, 33, 43), 1f);
            canvas.drawRoundRect(x - halfWidth, y - halfHeight, x + halfWidth,
                                 y + halfHeight, halfHeight * .45f,
                                 halfHeight * .45f, paint);
            paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeWidth(Math.max(2f, unit() * .012f));
            color(Color.argb(220, 222, 239, 249), 1f);
            canvas.drawRoundRect(x - halfWidth, y - halfHeight, x + halfWidth,
                                 y + halfHeight, halfHeight * .45f,
                                 halfHeight * .45f, paint);
            paint.setStyle(Paint.Style.FILL);
            paint.setTextAlign(Paint.Align.CENTER);
            paint.setTextSize(Math.max(11f * getResources().getDisplayMetrics().density,
                Math.min(unit() * .18f, getWidth() * .019f)));
            paint.setFakeBoldText(true);
            color(Color.WHITE, i == 3 && selected < 0 ? .45f : 1f);
            String label = i == 0 ? "SAVE" : i == 1 ? "CANCEL" : i == 2 ? "RESET"
                : selected >= 0 && !draft.visible[selected] ? "SHOW" : "HIDE";
            canvas.drawText(label, x, y - (paint.ascent() + paint.descent()) * .5f, paint);
        }
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (baseUnit <= 0f || activeLayout() == null) return;
        if (enabled || editing) {
            for (int id = 0; id < TouchControlLayout.COUNT; ++id) drawElement(canvas, id);
        }
        if (editing) drawEditorToolbar(canvas);
        else drawButton(canvas, settingX(), settingY(), unit() * .45f,
                        enabled ? "CTRL" : "OFF", false, 1f);
    }
}
