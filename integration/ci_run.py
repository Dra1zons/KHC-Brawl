#!/usr/bin/env python3
"""Publish a bounded build error as a check annotation for remote diagnosis."""
import subprocess,sys,tempfile
from pathlib import Path
with tempfile.TemporaryFile() as log:
    result=subprocess.run(sys.argv[1:],stdout=log,stderr=subprocess.STDOUT)
    log.seek(0);text=log.read().decode('utf-8','replace')
print(text)
if result.returncode:
    message=text[-7000:].replace('%','%25').replace('\r','%0D').replace('\n','%0A')
    print('::error title=Integration build::'+message)
raise SystemExit(result.returncode)
