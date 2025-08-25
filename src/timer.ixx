export module sandbox.timer;

import std;

namespace sandbox::timer {
    constexpr std::uint32_t MAX_TICKS_PER_FRAME = 100;

    export using getter_t = double (*)();
    export using OnFpsUpdate = void (*)(std::uint32_t);

    export class FixedUpdateTimer {
        getter_t getter_;
        double ticksPerSecond_;
        double lastTime_ = 0;
        double passedTime_ = 0;
        double deltaTime_ = 0;
        double partialTick_ = 0;
        double timescale_ = 1;
        std::uint32_t tickCount_ = 0;
        double accumulatedTime_ = 0;
        std::uint32_t accumulatedFrames_ = 0;
        std::uint32_t framesPerSecond_ = 0;
        OnFpsUpdate onFpsUpdate_ = nullptr;
    public:
        explicit FixedUpdateTimer(const getter_t& getter, const double ticksPerSecond) :
            getter_(getter),
            ticksPerSecond_(ticksPerSecond) {}

        [[nodiscard]] double currentTime() const { return getter_(); }
        [[nodiscard]] double ticksPerSecond() const { return ticksPerSecond_; }
        [[nodiscard]] double deltaTime() const { return deltaTime_; }
        [[nodiscard]] std::uint32_t framesPerSecond() const { return framesPerSecond_; }
        [[nodiscard]] double partialTick() const { return partialTick_; }
        [[nodiscard]] double timescale() const { return timescale_; }
        void setTimescale(const double timescale) { timescale_ = timescale; }
        [[nodiscard]] std::uint32_t tickCount() const { return tickCount_; }

        void setFpsCallback(const OnFpsUpdate& onFpsUpdate) { onFpsUpdate_ = onFpsUpdate; }

        void advanceTime() {
            const auto currTime = currentTime();
            const auto delta = currTime - lastTime_;
            lastTime_ = currTime;

            deltaTime_ = std::clamp(delta, 0.0, 1.0);
            passedTime_ += delta * timescale_ * ticksPerSecond_;
            tickCount_ = std::clamp(static_cast<std::uint32_t>(passedTime_), 0U, MAX_TICKS_PER_FRAME);
            passedTime_ -= tickCount_;
            partialTick_ = passedTime_;
        }

        void calculateFps() {
            ++accumulatedFrames_;
            while (currentTime() - 1.0 >= accumulatedTime_) {
                framesPerSecond_ = accumulatedFrames_;
                if (onFpsUpdate_ != nullptr) {
                    onFpsUpdate_(framesPerSecond_);
                }
                accumulatedTime_ += 1.0;
                accumulatedFrames_ = 0;
            }
        }
    };
}
