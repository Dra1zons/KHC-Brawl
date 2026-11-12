#!/usr/bin/env python3
import argparse,re,zipfile
from pathlib import Path

def merge(apk,dex):
    d=Path(dex).read_bytes()
    if not d.startswith(b'dex\n'):raise ValueError('Not a DEX')
    with zipfile.ZipFile(apk,'a',compression=zipfile.ZIP_DEFLATED) as z:
        names=z.namelist()
        if len(names)!=len(set(names)):raise ValueError('Duplicate ZIP entries')
        indexes=[]
        for n in names:
            m=re.fullmatch(r'classes(\d*)\.dex',n)
            if m:indexes.append(int(m.group(1) or '1'))
        if not indexes:raise ValueError('Original classes.dex missing')
        n=max(indexes)+1;name='classes'+str(n)+'.dex'
        z.writestr(name,d)
    return name

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('apk');p.add_argument('dex');a=p.parse_args();print('Added helper as',merge(a.apk,a.dex))
