#pragma once
#include <array>
#include <condition_variable>
#include <mutex>

#include <linux/input-event-codes.h>

namespace gawl {
struct Area {
    std::array<double, 4> data;
    void magnify(const double scale) {
        for(auto& p : data) {
            p *= scale;
        }
    }
    double operator[](size_t i) const {
        return data[i];
    }
    double& operator[](size_t i) {
        return data[i];
    }
};
using Color = std::array<double, 4>;
enum class ButtonState {
    press,
    release,
    repeat,
    leave,
};
enum class WheelAxis {
    vertical,
    horizontal,
};
enum class Align {
    left,
    center,
    right,
};
template <typename T>
struct SafeVar {
    std::mutex mutex;
    T          data;
    void       store(T src) {
        std::lock_guard<std::mutex> lock(mutex);
        data = src;
    }
    T load() {
        std::lock_guard<std::mutex> lock(mutex);
        return data;
    }
    SafeVar(T src) : data(src) {}
    SafeVar() {}
};
class ConditionalVariable {
  private:
    std::condition_variable condv;
    SafeVar<bool>           waked;

  public:
    void wait() {
        waked.store(false);
        std::unique_lock<std::mutex> lock(waked.mutex);
        condv.wait(lock, [this]() { return waked.data; });
    }
    template <typename T>
    bool wait_for(T duration) {
        waked.store(false);
        std::unique_lock<std::mutex> lock(waked.mutex);
        return condv.wait_for(lock, duration, [this]() { return waked.data; });
    }
    void wakeup() {
        waked.store(true);
        condv.notify_all();
    }
};
} // namespace gawl
