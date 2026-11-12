#include "modcore.hpp"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <tuple>

namespace modcore {
namespace {
bool validId(const std::string& s) {
    if(s.empty() || s.size()>160) return false;
    for(unsigned char c:s) if(c<32 || c==127) return false;
    return true;
}
void checkFps(int fps) { if(fps!=0 && (fps<15 || fps>240)) throw std::invalid_argument("fps: expected 0 or 15..240"); }
bool parseBool(const std::string& s) { if(s=="1")return true; if(s=="0")return false; throw std::invalid_argument("expected 0 or 1"); }
int parseInt(const std::string& s) { std::size_t n=0; int x=std::stoi(s,&n); if(n!=s.size())throw std::invalid_argument("invalid integer");return x; }
double parseDouble(const std::string& s) { std::size_t n=0;double x=std::stod(s,&n);if(n!=s.size() || !std::isfinite(x))throw std::invalid_argument("invalid number");return x; }
Capability capabilityFor(Operation op) {
    switch(op) {
    case Operation::SetAppearance:return Capability::Appearance;
    case Operation::SetAudio:return Capability::Audio;
    case Operation::SetFrameLimit:return Capability::FrameLimit;
    case Operation::SetCamera:return Capability::Camera;
    case Operation::SetQuality:return Capability::Quality;
    case Operation::SetParticles:return Capability::Particles;
    case Operation::SkipRewardAnimation:return Capability::RewardAnimation;
    default:return Capability::BattleFlow;
    }
}
}
void Settings::validate() const {
    checkFps(fps);
    if(!validId(background)||!validId(music))throw std::invalid_argument("invalid resource identifier");
    for(double v:{musicVolume,sfxVolume})if(!std::isfinite(v)||v<0||v>1)throw std::invalid_argument("volume: expected 0..1");
    if(particleCount<0||particleCount>200)throw std::invalid_argument("particleCount: expected 0..200");
    for(double v:{particleScale,particleSpeed})if(!std::isfinite(v)||v<0.1||v>5)throw std::invalid_argument("particle parameter: expected 0.1..5");
}
bool Settings::operator==(const Settings& b) const {
    return std::tie(fps,lowResolution,disableShake,randomTheme,randomAfterBattle,independentMusic,skipHighlight,instantReward,autoPlayAgain,background,music,musicVolume,sfxVolume,particleCount,particleScale,particleSpeed)==std::tie(b.fps,b.lowResolution,b.disableShake,b.randomTheme,b.randomAfterBattle,b.independentMusic,b.skipHighlight,b.instantReward,b.autoPlayAgain,b.background,b.music,b.musicVolume,b.sfxVolume,b.particleCount,b.particleScale,b.particleSpeed);
}
Settings SettingsStore::load(const std::filesystem::path& file) {
    std::error_code ec;
    bool exists=std::filesystem::exists(file,ec);
    if(ec)throw std::runtime_error("cannot inspect settings: "+ec.message());
    if(!exists)return Settings{};
    if(std::filesystem::file_size(file)>65536)throw std::invalid_argument("settings too large");
    std::ifstream in(file);if(!in)throw std::runtime_error("cannot open settings");
    Settings s;std::string line;std::set<std::string> seen;bool version=false;
    while(std::getline(in,line)) {
        if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line.empty())continue;
        auto pos=line.find('=');if(pos==std::string::npos)throw std::invalid_argument("expected key=value");
        auto k=line.substr(0,pos),v=line.substr(pos+1);
        if(!seen.insert(k).second)throw std::invalid_argument("duplicate setting: "+k);
        if(k=="schema") {if(v!="1")throw std::invalid_argument("unsupported schema");version=true;}
        else if(k=="fps")s.fps=parseInt(v);
        else if(k=="lowResolution")s.lowResolution=parseBool(v);
        else if(k=="disableShake")s.disableShake=parseBool(v);
        else if(k=="randomTheme")s.randomTheme=parseBool(v);
        else if(k=="randomAfterBattle")s.randomAfterBattle=parseBool(v);
        else if(k=="independentMusic")s.independentMusic=parseBool(v);
        else if(k=="skipHighlight")s.skipHighlight=parseBool(v);
        else if(k=="instantReward")s.instantReward=parseBool(v);
        else if(k=="autoPlayAgain")s.autoPlayAgain=parseBool(v);
        else if(k=="background")s.background=v;
        else if(k=="music")s.music=v;
        else if(k=="musicVolume")s.musicVolume=parseDouble(v);
        else if(k=="sfxVolume")s.sfxVolume=parseDouble(v);
        else if(k=="particleCount")s.particleCount=parseInt(v);
        else if(k=="particleScale")s.particleScale=parseDouble(v);
        else if(k=="particleSpeed")s.particleSpeed=parseDouble(v);
        else throw std::invalid_argument("unknown setting: "+k);
    }
    if(in.bad())throw std::runtime_error("settings read error");
    if(!version)throw std::invalid_argument("missing schema");
    s.validate();return s;
}
void SettingsStore::save(const std::filesystem::path& file,const Settings& s) {
    s.validate();if(!file.parent_path().empty())std::filesystem::create_directories(file.parent_path());
    auto tmp=file;tmp+=".tmp";
    { std::ofstream out(tmp,std::ios::trunc);if(!out)throw std::runtime_error("cannot write settings");
      out<<std::setprecision(17)<<"schema=1\n"<<"fps="<<s.fps<<"\nlowResolution="<<s.lowResolution<<"\ndisableShake="<<s.disableShake<<"\nrandomTheme="<<s.randomTheme<<"\nrandomAfterBattle="<<s.randomAfterBattle<<"\nindependentMusic="<<s.independentMusic<<"\nskipHighlight="<<s.skipHighlight<<"\ninstantReward="<<s.instantReward<<"\nautoPlayAgain="<<s.autoPlayAgain<<"\nbackground="<<s.background<<"\nmusic="<<s.music<<"\nmusicVolume="<<s.musicVolume<<"\nsfxVolume="<<s.sfxVolume<<"\nparticleCount="<<s.particleCount<<"\nparticleScale="<<s.particleScale<<"\nparticleSpeed="<<s.particleSpeed<<'\n';
      out.flush();if(!out)throw std::runtime_error("settings write error");
    }
    // Atomic replacement on Android/Linux. Single writer; no durability/fsync guarantee.
    std::filesystem::rename(tmp,file);
}
ThemeSelector::ThemeSelector(std::vector<Theme> themes,std::uint32_t seed):themes_(std::move(themes)),rng_(seed) {
    if(themes_.empty())throw std::invalid_argument("empty theme catalog");
    std::set<std::string> ids;
    for(const auto& t:themes_)if(!validId(t.id)||!validId(t.music)||!ids.insert(t.id).second)throw std::invalid_argument("invalid/duplicate theme");
}
ThemeChoice ThemeSelector::choose(const Settings& s,bool afterBattle) {
    s.validate();const Theme* background=nullptr;
    for(const auto& t:themes_)if(t.id==s.background)background=&t;
    if(!background)throw std::invalid_argument("unknown background");
    if((afterBattle && s.randomAfterBattle)||(!afterBattle && s.randomTheme))background=&themes_[std::uniform_int_distribution<std::size_t>(0,themes_.size()-1)(rng_)];
    std::string music=s.music;
    if((afterBattle && s.randomAfterBattle)||(!afterBattle && s.randomTheme)) {
        music=s.independentMusic?themes_[std::uniform_int_distribution<std::size_t>(0,themes_.size()-1)(rng_)].music:background->music;
    }
    bool known=false;for(const auto& t:themes_)if(t.music==music)known=true;
    if(!known)throw std::invalid_argument("unknown music identifier");
    return {background->id,music};
}
void FramePacer::setLimit(int fps) {checkFps(fps);fps_=fps;next_.reset();last_.reset();}
std::int64_t FramePacer::delayNs(std::int64_t now) {
    if(now<0)throw std::invalid_argument("negative clock");
    if(last_ && now<*last_)throw std::invalid_argument("clock moved backwards");
    last_=now;if(!fps_)return 0;
    std::int64_t period=1000000000LL/fps_;
    if(now>std::numeric_limits<std::int64_t>::max()-period*8)throw std::overflow_error("clock overflow");
    if(!next_ || now-*next_>period*4) {next_=now+period;return 0;}
    auto wait=*next_>now?*next_-now:0;
    next_=(*next_>now?*next_:now)+period;
    return wait;
}
Runtime::Runtime(std::vector<Theme> themes,std::uint32_t seed):selector_(std::move(themes),seed){}
void Runtime::attach(GameBridge* bridge) {bridge_=bridge;requestedBattles_.clear();}
void Runtime::configure(Settings s) {s.validate();settings_=std::move(s);}
Result Runtime::dispatch(Command c) {
    if(!bridge_)return {c.operation,Status::NoBridge};
    if(!bridge_->supports(capabilityFor(c.operation)))return {c.operation,Status::Unsupported};
    return {c.operation,bridge_->execute(c)?Status::Applied:Status::Rejected};
}
std::vector<Result> Runtime::applySettings() {
    auto theme=selector_.choose(settings_,false);std::vector<Result> result;
    for(auto op:{Operation::SetAppearance,Operation::SetAudio,Operation::SetFrameLimit,Operation::SetCamera,Operation::SetQuality,Operation::SetParticles})result.push_back(dispatch({op,settings_,theme,0}));
    return result;
}
Result Runtime::onReturnToLobby() {
    if(!settings_.randomAfterBattle)return {Operation::SetAppearance,Status::Disabled};
    return dispatch({Operation::SetAppearance,settings_,selector_.choose(settings_,true),0});
}
Result Runtime::onHighlightRequested(std::uint64_t id) {
    if(!id)return {Operation::SkipHighlight,Status::InvalidEvent};
    if(!settings_.skipHighlight)return {Operation::SkipHighlight,Status::Disabled};
    return dispatch({Operation::SkipHighlight,settings_,{},id});
}
Result Runtime::onRewardAnimationRequested() {
    if(!settings_.instantReward)return {Operation::SkipRewardAnimation,Status::Disabled};
    return dispatch({Operation::SkipRewardAnimation,settings_,{},0});
}
Result Runtime::onResultsShown(std::uint64_t id,bool canQueue) {
    if(!settings_.autoPlayAgain)return {Operation::PlayAgain,Status::Disabled};
    return requestPlayAgain(id,canQueue);
}
Result Runtime::requestPlayAgain(std::uint64_t id,bool canQueue) {
    if(!id || !canQueue)return {Operation::PlayAgain,Status::InvalidEvent};
    if(requestedBattles_.count(id))return {Operation::PlayAgain,Status::AlreadyRequested};
    auto result=dispatch({Operation::PlayAgain,settings_,{},id});
    if(result.status==Status::Applied)requestedBattles_.insert(id);
    return result;
}
Result Runtime::requestExitLobby() {return dispatch({Operation::ExitLobby,settings_,{},0});}
const char* statusName(Status s) {
    switch(s) {
    case Status::Applied:return "APPLIED_BY_BRIDGE";
    case Status::NoBridge:return "NO_GAME_BRIDGE";
    case Status::Unsupported:return "UNSUPPORTED";
    case Status::Rejected:return "REJECTED_BY_BRIDGE";
    case Status::Disabled:return "DISABLED";
    case Status::AlreadyRequested:return "ALREADY_REQUESTED";
    default:return "INVALID_EVENT";
    }
}
} // namespace modcore
