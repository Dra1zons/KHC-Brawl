import json,sys,tempfile,unittest,zipfile
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from patch_lifecycle import patch,MARKER
from merge_dex import merge
from download_base import secure_url

class PatcherTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.root=Path(self.tmp.name)
        self.file=self.root/'smali/test/game/Activity.smali';self.file.parent.mkdir(parents=True)
    def tearDown(self):self.tmp.cleanup()
    def put(self,body='',superclass='Landroid/app/Activity;'):
        self.file.write_text('.class public Ltest/game/Activity;\n.super '+superclass+'\n'+body)
    def test_add_override(self):
        self.put();r=patch(self.root,'test.game.Activity');s=self.file.read_text()
        self.assertTrue(r['changed']);self.assertIn('invoke-super/range {p0 .. p0}',s);self.assertEqual(s.count(MARKER),1)
    def test_idempotent(self):
        self.put();patch(self.root,'test.game.Activity');before=self.file.read_text();r=patch(self.root,'test.game.Activity')
        self.assertFalse(r['changed']);self.assertEqual(before,self.file.read_text())
    def test_existing_high_registers_multiple_returns(self):
        self.put('.method public onResume()V\n.locals 32\nif-eqz p0, :other\nreturn-void\n:other\nreturn-void\n.end method\n')
        r=patch(self.root,'test.game.Activity');self.assertEqual(r['hook_points'],2);self.assertEqual(self.file.read_text().count(MARKER),2)
    def test_reject_duplicate_class(self):
        self.put();p=self.root/'smali_classes2/test/game/Activity.smali';p.parent.mkdir(parents=True);p.write_text(self.file.read_text())
        with self.assertRaises(ValueError):patch(self.root,'test.game.Activity')
    def test_reject_non_activity(self):
        self.put(superclass='Ljava/lang/Object;')
        with self.assertRaises(ValueError):patch(self.root,'test.game.Activity')
    def test_reject_native_method(self):
        self.put('.method public native onResume()V\n.end method\n')
        with self.assertRaises(ValueError):patch(self.root,'test.game.Activity')
    def test_reject_final_ancestor(self):
        self.put(superclass='Ltest/game/Base;');p=self.file.parent/'Base.smali'
        p.write_text('.class public Ltest/game/Base;\n.super Landroid/app/Activity;\n.method public final onResume()V\n.locals 0\nreturn-void\n.end method\n')
        with self.assertRaises(ValueError):patch(self.root,'test.game.Activity')
    def test_reject_invalid_name(self):
        self.put()
        with self.assertRaises(ValueError):patch(self.root,'../../etc/passwd')
    def test_reject_no_return(self):
        self.put('.method public onResume()V\n.locals 0\nthrow p0\n.end method\n')
        with self.assertRaises(ValueError):patch(self.root,'test.game.Activity')
    def test_merge_next_dex(self):
        apk=self.root/'x.apk';dex=self.root/'helper.dex';dex.write_bytes(b'dex\n035\0test fixture, not real dex')
        with zipfile.ZipFile(apk,'w') as z:z.writestr('classes.dex',b'base');z.writestr('classes3.dex',b'base3')
        self.assertEqual(merge(apk,dex),'classes4.dex')
        with zipfile.ZipFile(apk) as z:self.assertEqual(z.read('classes4.dex'),dex.read_bytes())
    def test_merge_reject_invalid(self):
        apk=self.root/'x.apk';dex=self.root/'bad.dex';dex.write_bytes(b'not a dex')
        with zipfile.ZipFile(apk,'w') as z:z.writestr('classes.dex',b'base')
        with self.assertRaises(ValueError):merge(apk,dex)
    def test_urls(self):
        self.assertEqual(secure_url('https://disk.yandex.ru/d/example'),'https://disk.yandex.ru/d/example')
        for s in ['http://example.com/a','file:///etc/passwd','https://user:pass@example.com/apk']:
            with self.assertRaises(ValueError):secure_url(s)

if __name__=='__main__':unittest.main(verbosity=2)
