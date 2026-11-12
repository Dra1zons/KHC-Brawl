#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

namespace modcore {
// Single-threaded core. The game adapter MUST call from its designated thread.
struct Settings {
    int fps = 0; // 0 = no limit requested, not 'unlimited hardware FPS'
    bool lowResolution = false, disableShake = false;
    bool randomTheme = false, randomAfterBattle = false, independentMusic = false;
    bool skipHighlight = false, instantReward = false, autoPlayAgain = false;
    std::string background = "Default", music = "MenuMusic";
    double musicVolume = 1.0, sfxVolume = 1.0;
    int particleCount = 20;
    double particleScale = 1.0, particleSpeed = 1.0;
    void validate() const;
    bool operator==(const Settings& other) const;
};
class SettingsStore {
public:
    static Settings load(const std::filesystem::path& file); // missing file = defaults
    static void save(const std::filesystem::path& file, const Settings& settings);
};
struct Theme { std::string id, music; };
struct ThemeChoice { std::string background, music; };
class ThemeSelector {
    std::vector<Theme> themes_;
    std::mt19937 rng_;
public:
    explicit ThemeSelector(std::vector<Theme> themes, std::uint32_t seed);
    ThemeChoice choose(const Settings& settings, bool afterBattle);
};
// Call once per frame, then wait for the returned time BEFORE rendering.
// This computes a deadline only. The adapter decides where sleeping is safe.
class FramePacer {
    int fps_ = 0;
    std::optional<std::int64_t> next_, last_;
public:
    void setLimit(int fps);
    std::int64_t delayNs(std::int64_t monotonicNowNs);
};
enum class Capability { Appearance, Audio, FrameLimit, Camera, Quality, Particles, BattleFlow, RewardAnimation };
enum class Operation { SetAppearance, SetAudio, SetFrameLimit, SetCamera, SetQuality, SetParticles, SkipHighlight, SkipRewardAnimation, PlayAgain, ExitLobby };
struct Command {
    Operation operation;
    Settings settings;
    ThemeChoice theme;
    std::uint64_t battleId = 0;
};
// No Brawl Stars implementation is supplied. No offsets or signatures are guessed.
class GameBridge {
public:
    virtual ~GameBridge() = default;
    virtual bool supports(Capability capability) const = 0;
    virtual bool execute(const Command& command) = 0;
};
enum class Status { Applied, NoBridge, Unsupported, Rejected, Disabled, AlreadyRequested, InvalidEvent };
struct Result { Operation operation; Status status; };
class Runtime {
    Settings settings_;
    GameBridge* bridge_ = nullptr; // non-owning, caller keeps it alive
    ThemeSelector selector_;
    std::unordered_set<std::uint64_t> requestedBattles_;
    Result dispatch(Command command);
public:
    explicit Runtime(std::vector<Theme> themes, std::uint32_t seed = std::random_device{}());
    void attach(GameBridge* bridge); // attaching a new bridge clears per-session event history
    void configure(Settings settings);
    const Settings& settings() const { return settings_; }
    std::vector<Result> applySettings();
    Result onReturnToLobby();
    Result onHighlightRequested(std::uint64_t battleId);
    Result onRewardAnimationRequested();
    Result onResultsShown(std::uint64_t battleId, bool canQueue);
    Result requestPlayAgain(std::uint64_t battleId, bool canQueue);
    Result requestExitLobby();
};
const char* statusName(Status value);
} // namespace modcore
