package io.ebensos.brawlmenu;

import android.app.Activity;
import android.os.Bundle;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.webkit.WebResourceRequest;

/** Offline menu prototype. Does not modify or connect to any game. */
public final class MainActivity extends Activity {
    private WebView web;
    static { System.loadLibrary("khc_core"); }
    private static native String nativeBridgeStatus();
    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        web = new WebView(this);
        web.getSettings().setJavaScriptEnabled(true);
        web.getSettings().setDomStorageEnabled(true);
        web.getSettings().setAllowFileAccess(false);
        web.getSettings().setAllowContentAccess(false);
        web.setWebViewClient(new WebViewClient() {
            @Override public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest request) { return true; }
        });
        setContentView(web);
        web.loadDataWithBaseURL("https://menu.invalid/", readAsset("index.html").replace("__NATIVE_STATUS__", nativeBridgeStatus()), "text/html", "UTF-8", null);
    }
    private String readAsset(String name) {
        try (java.io.InputStream stream = getAssets().open(name);
             java.io.ByteArrayOutputStream out = new java.io.ByteArrayOutputStream()) {
            byte[] buffer = new byte[8192];
            int count;
            while ((count = stream.read(buffer)) != -1) out.write(buffer, 0, count);
            return out.toString("UTF-8");
        } catch (java.io.IOException error) { throw new IllegalStateException("Cannot load local menu", error); }
    }
    @Override protected void onDestroy() { if (web != null) { web.destroy(); web = null; } super.onDestroy(); }
}
