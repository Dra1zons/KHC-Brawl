#!/usr/bin/env python3
"""Download the selected APK on the CI runner. Does not execute it."""
import argparse,hashlib,json,re,urllib.parse,urllib.request,zipfile
from pathlib import Path
MAX=4*1024**3

def secure_url(value):
    p=urllib.parse.urlparse(value)
    if p.scheme!='https' or not p.hostname or p.username or p.password:raise ValueError('Expected a public HTTPS URL without credentials')
    return value

def download(source,out):
    url=secure_url(source['url']);expected=source.get('sha256','').lower().strip()
    if expected and not re.fullmatch(r'[a-f0-9]{64}',expected):raise ValueError('Invalid SHA256')
    host=urllib.parse.urlparse(url).hostname.lower()
    if host in {'disk.yandex.ru','disk.yandex.com','yadi.sk'}:
        api='https://cloud-api.yandex.net/v1/disk/public/resources/download?'+urllib.parse.urlencode({'public_key':url})
        with urllib.request.urlopen(api,timeout=60) as response:
            meta=json.load(response)
        url=secure_url(meta['href'])
    out=Path(out);out.parent.mkdir(parents=True,exist_ok=True);tmp=out.with_suffix(out.suffix+'.part');h=hashlib.sha256();size=0
    try:
        with urllib.request.urlopen(urllib.request.Request(url,headers={'User-Agent':'KHC-integration-build/1'}),timeout=120) as response,tmp.open('wb') as dest:
            secure_url(response.geturl())
            length=response.headers.get('Content-Length')
            if length and int(length)>MAX:raise ValueError('File too large')
            while True:
                chunk=response.read(1024*1024)
                if not chunk:break
                size+=len(chunk)
                if size>MAX:raise ValueError('File too large')
                h.update(chunk);dest.write(chunk)
        actual=h.hexdigest()
        if expected and actual!=expected:raise ValueError('APK SHA256 mismatch')
        with zipfile.ZipFile(tmp) as z:
            if 'AndroidManifest.xml' not in z.namelist() or 'classes.dex' not in z.namelist():raise ValueError('Expected a standalone APK, not an XAPK, APKM or folder')
        tmp.replace(out)
        report={'bytes':size,'sha256':actual,'hash_matched_user_value':bool(expected)}
        out.with_suffix('.source-info.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))
    except Exception:
        tmp.unlink(missing_ok=True);raise

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('config');p.add_argument('output');a=p.parse_args()
    try:download(json.loads(Path(a.config).read_text()),a.output)
    except Exception as e:raise SystemExit('APK download failed: '+type(e).__name__+'. Verify the public source URL and checksum.')
