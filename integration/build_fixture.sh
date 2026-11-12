#!/bin/bash
set -euo pipefail
SDK="${ANDROID_HOME:?}"; BT="$SDK/build-tools/35.0.0"; OUT=integration-build/fixture
mkdir -p "$OUT/src/khc/fixture" "$OUT/classes" "$OUT/dex"
cat > "$OUT/AndroidManifest.xml" <<'XML'
<manifest xmlns:android="http://schemas.android.com/apk/res/android" package="khc.fixture" android:versionCode="1" android:versionName="1"><uses-sdk android:minSdkVersion="26" android:targetSdkVersion="35"/><application android:label="KHC Integration Fixture" android:theme="@android:style/Theme.Material.NoActionBar"><activity android:name=".GameActivity" android:exported="true"><intent-filter><action android:name="android.intent.action.MAIN"/><category android:name="android.intent.category.LAUNCHER"/></intent-filter></activity></application></manifest>
XML
cat > "$OUT/src/khc/fixture/GameActivity.java" <<'JAVA'
package khc.fixture;
public final class GameActivity extends android.app.Activity {
 @Override public void onCreate(android.os.Bundle state) {
  super.onCreate(state);
  android.widget.TextView text=new android.widget.TextView(this);
  text.setText("Integration test fixture, NOT Brawl Stars");setContentView(text);
 }
}
JAVA
javac -source 8 -target 8 -classpath "$SDK/platforms/android-35/android.jar" -d "$OUT/classes" "$OUT/src/khc/fixture/GameActivity.java"
"$BT/d8" --lib "$SDK/platforms/android-35/android.jar" --min-api 26 --output "$OUT/dex" "$OUT/classes/khc/fixture/GameActivity.class"
"$BT/aapt2" link -I "$SDK/platforms/android-35/android.jar" --manifest "$OUT/AndroidManifest.xml" -o "$OUT/fixture.apk" --min-sdk-version 26 --target-sdk-version 35
python3 -c 'import zipfile; z=zipfile.ZipFile("integration-build/fixture/fixture.apk","a");z.write("integration-build/fixture/dex/classes.dex","classes.dex");z.close()'
bash integration/integrate.sh "$OUT/fixture.apk" khc.fixture.GameActivity integration-build/smoke
java -jar integration-build/apktool.jar d -r integration-build/smoke/KHC-integrated-test.apk -o integration-build/roundtrip
python3 - <<'PY'
from pathlib import Path
import zipfile
root=Path('integration-build/roundtrip')
game=next(root.glob('smali*/khc/fixture/GameActivity.smali')).read_text()
assert 'Lkhc/integration/Menu;->attach(Landroid/app/Activity;)V' in game
assert len(list(root.glob('smali*/khc/integration/Menu.smali')))==1
print('PASS: signed fixture roundtrip contains Activity hook and helper in separate DEX. Not a gameplay/runtime test.')
PY
