#!/bin/bash
set -euo pipefail
BASE="${1:?base APK required}"
ACTIVITY="${2:?fully qualified game Activity required}"
OUT="${3:-integration-build/game}"
SDK="${ANDROID_HOME:?}"; BT="$SDK/build-tools/35.0.0"
mkdir -p "$OUT"
# Only decode into a fresh directory. apktool handles code; resources remain binary.
java -Xmx4g -jar integration-build/apktool.jar d -r "$BASE" -o "$OUT/decoded"
python3 integration/patch_lifecycle.py "$OUT/decoded" --activity "$ACTIVITY" --report "$OUT/integration-report.json"
java -Xmx4g -jar integration-build/apktool.jar b "$OUT/decoded" -o "$OUT/unsigned.apk"
python3 integration/merge_dex.py "$OUT/unsigned.apk" integration-build/helper/dex/classes.dex
"$BT/zipalign" -P 16 -f 4 "$OUT/unsigned.apk" "$OUT/aligned.apk"
if [ ! -f integration-build/test-signing.p12 ]; then
  keytool -genkeypair -keystore integration-build/test-signing.p12 -storetype PKCS12 -storepass android -keypass android -alias khc-debug -keyalg RSA -keysize 2048 -validity 3650 -dname 'CN=KHC Integration Test'
fi
"$BT/apksigner" sign --ks integration-build/test-signing.p12 --ks-key-alias khc-debug --ks-pass pass:android --key-pass pass:android --out "$OUT/KHC-integrated-test.apk" "$OUT/aligned.apk"
"$BT/apksigner" verify --verbose "$OUT/KHC-integrated-test.apk"
"$BT/zipalign" -c -P 16 4 "$OUT/KHC-integrated-test.apk"
sha256sum "$OUT/KHC-integrated-test.apk" > "$OUT/SHA256SUMS.txt"
