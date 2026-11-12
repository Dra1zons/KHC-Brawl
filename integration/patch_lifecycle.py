#!/usr/bin/env python3
"""Patch one decoded Activity lifecycle. No guessed offsets; fail closed on ambiguity."""
import argparse, json, re
from pathlib import Path
HOOK='    invoke-static/range {p0 .. p0}, Lkhc/integration/Menu;->attach(Landroid/app/Activity;)V'
MARKER='Lkhc/integration/Menu;->attach(Landroid/app/Activity;)V'
KNOWN_ACTIVITY_BASES={'Landroid/app/Activity;','Landroid/app/NativeActivity;','Landroid/app/ListActivity;'}

def descriptor(name):
    if not re.fullmatch(r'[A-Za-z_$][\w$]*(\.[A-Za-z_$][\w$]*)+',name):raise ValueError('Invalid Activity class name')
    return 'L'+name.replace('.','/')+';'

def classes(root, desc):
    rel=desc[1:-1]+'.smali'
    return [p/rel for p in Path(root).glob('smali*') if p.is_dir() and (p/rel).is_file()]

def find_class(root,desc):
    found=classes(root,desc)
    if len(found)!=1:raise ValueError('Expected exactly one '+desc+'; found '+str(len(found))+'. Duplicate DEX classes require manual analysis.')
    return found[0]

def superclass(text):
    m=re.search(r'^\.super\s+(L[^\s]+;)\s*$',text,re.M)
    if not m:raise ValueError('No unambiguous superclass')
    return m.group(1)

def prove_activity(root,desc):
    seen=set();chain=[]
    while desc not in KNOWN_ACTIVITY_BASES:
        if desc in seen:raise ValueError('Inheritance cycle')
        seen.add(desc);path=find_class(root,desc);text=path.read_text();chain.append(desc);desc=superclass(text)
    return chain+[desc]

def patch(root,name):
    desc=descriptor(name);chain=prove_activity(root,desc)
    path=find_class(root,desc);text=path.read_text();sup=superclass(text)
    matches=list(re.finditer(r'^\.method\b[^\n]*\bonResume\(\)V[^\n]*\n.*?^\.end method\s*$',text,re.M|re.S))
    if len(matches)>1:raise ValueError('Duplicate lifecycle definitions')
    if matches:
        m=matches[0];block=m.group();header=block.splitlines()[0]
        if re.search(r'\b(native|abstract|static|private)\b',header):raise ValueError('Unsupported lifecycle access/body: '+header)
        if MARKER in block:return {'class':name,'path':str(path),'changed':False,'reason':'already patched','inheritance':chain}
        returns=list(re.finditer(r'^(\s*)return-void\s*(?:#.*)?$',block,re.M))
        if not returns:raise ValueError('No safe return-void point')
        # p0 is this regardless of .locals/.registers. /range supports high register indexes.
        new=re.sub(r'^([ \t]*)return-void([ \t]*(?:#.*)?)$',lambda m:HOOK+'\n'+m.group(0),block,flags=re.M)
        if new.count(MARKER)!=len(returns):raise ValueError('Not all lifecycle returns can be safely patched')
        updated=text[:m.start()]+new+text[m.end():]
        mode='patch existing onResume';points=len(returns)
    else:
        # Adding an override must not override a final/private/static ancestor method.
        for ancestor in chain[1:-1]:
            ancestor_text=find_class(root,ancestor).read_text()
            for header in re.findall(r'^\.method[^\n]*\bonResume\(\)V[^\n]*$',ancestor_text,re.M):
                if re.search(r'\b(final|private|static)\b',header):raise ValueError('Cannot safely override ancestor lifecycle: '+ancestor)
        updated=text.rstrip()+'\n\n.method public onResume()V\n    .locals 0\n\n    invoke-super/range {p0 .. p0}, '+sup+'->onResume()V\n'+HOOK+'\n    return-void\n.end method\n'
        mode='add onResume override';points=1
    path.write_text(updated)
    return {'class':name,'path':str(path),'changed':True,'mode':mode,'hook_points':points,'inheritance':chain}

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('decoded');p.add_argument('--activity',required=True);p.add_argument('--report',default='integration-report.json');a=p.parse_args()
    r=patch(a.decoded,a.activity);Path(a.report).write_text(json.dumps(r,indent=2));print(json.dumps(r,indent=2))
