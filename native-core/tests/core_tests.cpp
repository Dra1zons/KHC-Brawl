#include "modcore.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace modcore;
namespace {
int count=0;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);++count;}
template<class F> void rejects(F f,const char* message){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,message);}
std::vector<Theme> themes(){return {{"Default","MenuMusic"},{"Winter","WinterMusic"},{"Summer","SummerMusic"}};}
// TEST DOUBLE ONLY. This is NOT a Brawl Stars bridge.
struct FakeBridge:GameBridge {
 bool supported=true,accepted=true;
 std::vector<Command> commands;
 bool supports(Capability)const override{return supported;}
 bool execute(const Command& c)override{commands.push_back(c);return accepted;}
};
}
int main(){
 try{
 Settings s;s.validate();
 rejects([&]{auto v=s;v.fps=14;v.validate();},"FPS lower bound");
 rejects([&]{auto v=s;v.fps=241;v.validate();},"FPS upper bound");
 rejects([&]{auto v=s;v.musicVolume=1.1;v.validate();},"Volume range");
 rejects([&]{auto v=s;v.sfxVolume=std::numeric_limits<double>::quiet_NaN();v.validate();},"NaN rejected");
 rejects([&]{auto v=s;v.background="bad\nkey";v.validate();},"No injected config lines");
 rejects([&]{auto v=s;v.particleCount=201;v.validate();},"Particle count bound");
 rejects([&]{auto v=s;v.particleSpeed=0;v.validate();},"Particle speed bound");
 auto dir=std::filesystem::temp_directory_path()/("modcore-tests-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(dir);auto file=dir/"settings.cfg";
 check(SettingsStore::load(file)==Settings{},"Missing file gives defaults");
 s.fps=90;s.disableShake=true;s.musicVolume=0.37;s.sfxVolume=0.29;s.background="Winter";s.music="WinterMusic";s.particleScale=1.3;s.autoPlayAgain=true;
 SettingsStore::save(file,s);check(SettingsStore::load(file)==s,"Settings exact roundtrip");
 s.fps=120;SettingsStore::save(file,s);check(SettingsStore::load(file)==s,"Atomic replace existing file");
 auto put=[&](const char* text){std::ofstream o(file);o<<text;};
 put("schema=1\nschema=1\n");rejects([&]{SettingsStore::load(file);},"Duplicate key rejected");
 put("schema=2\n");rejects([&]{SettingsStore::load(file);},"Unknown schema rejected");
 put("fps=60\n");rejects([&]{SettingsStore::load(file);},"Missing schema rejected");
 put("schema=1\nunknown=3\n");rejects([&]{SettingsStore::load(file);},"Unknown key rejected");
 put("schema=1\nfps=60abc\n");rejects([&]{SettingsStore::load(file);},"Integer suffix rejected");
 put("schema=1\nautoPlayAgain=true\n");rejects([&]{SettingsStore::load(file);},"Invalid boolean rejected");
 put("schema=1\nmusicVolume=nan\n");rejects([&]{SettingsStore::load(file);},"NaN in file rejected");
 std::filesystem::remove_all(dir);
 ThemeSelector selector(themes(),123);Settings t;t.background="Winter";t.music="SummerMusic";
 auto choice=selector.choose(t,false);check(choice.background=="Winter"&&choice.music=="SummerMusic","Explicit background and music");
 t.randomTheme=true;t.independentMusic=false;
 for(int i=0;i<40;++i){auto c=selector.choose(t,false);bool match=false;for(const auto& theme:themes())if(c.background==theme.id&&c.music==theme.music)match=true;check(match,"Random theme keeps paired music");}
 t.independentMusic=true;
 for(int i=0;i<40;++i){auto c=selector.choose(t,false);bool a=false,b=false;for(const auto& th:themes()){a=a||th.id==c.background;b=b||th.music==c.music;}check(a&&b,"Random independent IDs stay in catalog");}
 t.randomTheme=false;t.randomAfterBattle=false;check(selector.choose(t,true).background=="Winter","No random after battle when disabled");
 t.background="missing";rejects([&]{selector.choose(t,false);},"Unknown theme rejected");
 rejects([&]{ThemeSelector({},1);},"Empty catalog rejected");
 rejects([&]{ThemeSelector({{"A","X"},{"A","Y"}},1);},"Duplicate theme rejected");
 FramePacer pacer;check(pacer.delayNs(0)==0,"Default no FPS pacing");pacer.setLimit(60);
 check(pacer.delayNs(1000000000)==0,"First frame no delay");
 check(pacer.delayNs(1001000000)==15666666,"Second frame pacing");
 check(pacer.delayNs(1017666666)==15666666,"Next deadline pacing");
 check(pacer.delayNs(2000000000)==0,"Long stall resets deadline");
 rejects([&]{pacer.delayNs(1);},"Backward monotonic clock rejected");
 pacer.setLimit(0);check(pacer.delayNs(0)==0,"FPS reset");
 rejects([&]{pacer.setLimit(300);},"Invalid FPS cap rejected");
 pacer.setLimit(60);rejects([&]{pacer.delayNs(std::numeric_limits<std::int64_t>::max());},"Clock overflow guarded");
 Runtime runtime(themes(),123);
 for(const auto& r:runtime.applySettings())check(r.status==Status::NoBridge,"Never reports working without game bridge");
 FakeBridge bridge;runtime.attach(&bridge);bridge.supported=false;
 for(const auto& r:runtime.applySettings())check(r.status==Status::Unsupported,"Unsupported capability gated");
 check(bridge.commands.empty(),"No execution without support");
 bridge.supported=true;bridge.accepted=false;
 for(const auto& r:runtime.applySettings())check(r.status==Status::Rejected,"Bridge rejection propagated");
 bridge.accepted=true;bridge.commands.clear();
 for(const auto& r:runtime.applySettings())check(r.status==Status::Applied,"Supported command delivered to TEST bridge");
 check(bridge.commands.size()==6,"All six settings commands sent to TEST bridge");
 check(runtime.onResultsShown(4,true).status==Status::Disabled,"Autoplay opt-in");
 Settings conf;conf.autoPlayAgain=true;conf.skipHighlight=true;conf.instantReward=true;conf.randomAfterBattle=true;
 runtime.configure(conf);
 check(runtime.onResultsShown(4,false).status==Status::InvalidEvent,"Cannot queue guard");
 check(runtime.onResultsShown(0,true).status==Status::InvalidEvent,"Battle id guard");
 check(runtime.onResultsShown(4,true).status==Status::Applied,"Autoplay dispatch to TEST bridge");
 check(runtime.onResultsShown(4,true).status==Status::AlreadyRequested,"Duplicate results cannot queue twice");
 check(runtime.onResultsShown(5,true).status==Status::Applied,"Next battle allowed");
 check(runtime.onResultsShown(4,true).status==Status::AlreadyRequested,"Older duplicate still blocked");
 bridge.accepted=false;check(runtime.onResultsShown(6,true).status==Status::Rejected,"Failed queue not committed");
 bridge.accepted=true;check(runtime.onResultsShown(6,true).status==Status::Applied,"Retry after rejected request");
 check(runtime.onHighlightRequested(0).status==Status::InvalidEvent,"Invalid highlight guard");
 check(runtime.onHighlightRequested(7).status==Status::Applied,"Skip highlight event dispatch to TEST bridge");
 check(runtime.onRewardAnimationRequested().status==Status::Applied,"Reward animation dispatch to TEST bridge");
 check(runtime.onReturnToLobby().status==Status::Applied,"Post-battle theme dispatch to TEST bridge");
 check(runtime.requestExitLobby().status==Status::Applied,"Exit request dispatch to TEST bridge");
 runtime.attach(nullptr);check(runtime.requestExitLobby().status==Status::NoBridge,"Detached bridge stays unavailable");
 auto bad=conf;bad.fps=-1;rejects([&]{runtime.configure(bad);},"Reject invalid settings transaction");check(runtime.settings()==conf,"Keep old settings on failed validate");
 std::cout<<"PASS: "<<count<<" checks. Test bridge only; no game integration or Android execution.\n";return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
