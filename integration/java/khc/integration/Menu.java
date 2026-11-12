package khc.integration;

import android.app.Activity;
import android.graphics.Color;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;

/** In-process Android view. No external app, overlay permission, game API or network. */
public final class Menu {
    private static final String TAG = "khc.integrated.menu.v1";
    private Menu() {}
    public static void attach(final Activity activity) {
        if (activity == null || activity.isFinishing() || activity.isDestroyed()) return;
        try {
            activity.getWindow().getDecorView().post(new Runnable() {
                @Override public void run() {
                    if (activity.isFinishing() || activity.isDestroyed()) return;
                    try { addViews(activity); }
                    catch (RuntimeException error) { Log.e("KHCMenu", "Cannot attach in-process menu", error); }
                }
            });
        } catch (RuntimeException error) { Log.e("KHCMenu", "Cannot schedule menu", error); }
    }
    private static int dp(Activity a, int x) {
        return Math.round(x * a.getResources().getDisplayMetrics().density);
    }
    private static TextView text(Activity a, String value, int size) {
        TextView t = new TextView(a); t.setText(value); t.setTextColor(Color.WHITE);
        t.setTextSize(size); t.setPadding(0, dp(a, 6), 0, dp(a, 6)); return t;
    }
    private static Button button(Activity a, String value) {
        Button b = new Button(a); b.setText(value); b.setAllCaps(false);
        b.setTextColor(Color.WHITE); b.setBackgroundTintList(android.content.res.ColorStateList.valueOf(0xff29466c));
        b.setMinHeight(dp(a, 48)); return b;
    }
    private static void addViews(final Activity a) {
        final View decor = a.getWindow().getDecorView();
        if (decor.findViewWithTag(TAG) != null) return;
        final Window w = a.getWindow();
        final float originalBrightness = w.getAttributes().screenBrightness;
        final boolean originalKeepScreenOn = (w.getAttributes().flags & WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON) != 0;
        final FrameLayout root = new FrameLayout(a); root.setTag(TAG);
        root.setClickable(false); root.setFocusable(false);
        final Button toggle = button(a, "KHC"); toggle.setContentDescription("Открыть меню KHC внутри игры");
        FrameLayout.LayoutParams toggleLayout = new FrameLayout.LayoutParams(dp(a, 72), dp(a, 48), Gravity.TOP | Gravity.END);
        toggleLayout.setMargins(dp(a, 12), dp(a, 12), dp(a, 12), 0);
        final ScrollView scroll = new ScrollView(a); scroll.setVisibility(View.GONE);
        scroll.setBackgroundColor(0xf51a2331); scroll.setFillViewport(false);
        int availableWidth = a.getResources().getDisplayMetrics().widthPixels - dp(a, 24);
        int availableHeight = a.getResources().getDisplayMetrics().heightPixels - dp(a, 88);
        FrameLayout.LayoutParams panelLayout = new FrameLayout.LayoutParams(Math.min(dp(a, 340), Math.max(dp(a, 180), availableWidth)), Math.min(dp(a, 420), Math.max(dp(a, 100), availableHeight)), Gravity.TOP | Gravity.END);
        panelLayout.setMargins(dp(a, 12), dp(a, 72), dp(a, 12), dp(a, 12));
        LinearLayout content = new LinearLayout(a); content.setOrientation(LinearLayout.VERTICAL);
        content.setPadding(dp(a, 16), dp(a, 12), dp(a, 16), dp(a, 16));
        content.addView(text(a, "KHC · в процессе клиента", 21));
        content.addView(text(a, "Интеграционный тест. Это не полный аналог BSD. Здесь нет скин-чейнджера или функций боя.", 14));
        final Switch keepOn = new Switch(a); keepOn.setText("Не гасить экран"); keepOn.setTextColor(Color.WHITE);
        keepOn.setMinHeight(dp(a, 48)); keepOn.setChecked(originalKeepScreenOn);
        keepOn.setOnCheckedChangeListener((v, enabled) -> {
            if (enabled) w.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            else w.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        });
        content.addView(keepOn);
        final TextView brightnessLabel = text(a, "Яркость окна: системная", 16); content.addView(brightnessLabel);
        final SeekBar brightness = new SeekBar(a); brightness.setMax(99);
        brightness.setProgress(originalBrightness < 0 ? 69 : Math.max(0, Math.min(99, Math.round(originalBrightness * 100) - 1)));
        brightness.setContentDescription("Яркость только окна игры");
        brightness.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onStartTrackingTouch(SeekBar s) {}
            public void onStopTrackingTouch(SeekBar s) {}
            public void onProgressChanged(SeekBar s, int value, boolean fromUser) {
                if (!fromUser) return;
                WindowManager.LayoutParams params = w.getAttributes(); params.screenBrightness = (value + 1) / 100f;
                w.setAttributes(params); brightnessLabel.setText("Яркость окна: " + (value + 1) + "%");
            }
        });
        content.addView(brightness, new LinearLayout.LayoutParams(-1, dp(a, 48)));
        Button reset = button(a, "Вернуть исходные настройки окна");
        reset.setOnClickListener(v -> {
            WindowManager.LayoutParams p = w.getAttributes(); p.screenBrightness = originalBrightness; w.setAttributes(p);
            keepOn.setChecked(originalKeepScreenOn);
            brightness.setProgress(originalBrightness < 0 ? 69 : Math.max(0, Math.min(99, Math.round(originalBrightness * 100) - 1)));
            brightnessLabel.setText("Яркость окна: исходная");
        });
        content.addView(reset);
        Button close = button(a, "Закрыть меню"); close.setOnClickListener(v -> scroll.setVisibility(View.GONE)); content.addView(close);
        content.addView(text(a, "Настройки меняют только это окно. Системная яркость и игровые аккаунты не затрагиваются.", 14));
        scroll.addView(content); root.addView(scroll, panelLayout); root.addView(toggle, toggleLayout);
        toggle.setOnClickListener(v -> scroll.setVisibility(scroll.getVisibility() == View.VISIBLE ? View.GONE : View.VISIBLE));
        a.addContentView(root, new android.view.ViewGroup.LayoutParams(-1, -1));
    }
}
