#ifndef NES_TERM_CONTROLLER_HPP
#define NES_TERM_CONTROLLER_HPP
#include <cstdint>
#include <mutex>
#include <type_traits>

namespace oxnes {

using Byte = std::uint8_t;

// thread safe.
class Controller {
   public:
    enum class Button : Byte {
        A      = 0b00000001,
        B      = 0b00000010,
        Select = 0b00000100,
        Start  = 0b00001000,
        Up     = 0b00010000,
        Down   = 0b00100000,
        Left   = 0b01000000,
        Right  = 0b10000000
    };

   public:
    void press(Button b)
    {
        auto const lock = std::lock_guard{mtx_};
        state_ |= static_cast<std::underlying_type_t<Button>>(b);
    }

    void release(Button b)
    {
        auto const lock = std::lock_guard{mtx_};
        state_ &= ~(static_cast<std::underlying_type_t<Button>>(b));
    }

   public:
    auto read() -> Byte
    {
        auto const lock = std::lock_guard{mtx_};

        if (strobe_) {
            return state_ |
                   static_cast<std::underlying_type_t<Button>>(Button::A);
        }
        else {
            auto const response =
                (state_ & (1 << button_index_)) >> button_index_;
            if (button_index_ == 7)
                button_index_ = 0;
            else
                ++button_index_;
            return response;
        }
    }

    void strobe(Byte b)
    {
        auto const lock = std::lock_guard{mtx_};

        strobe_ = ((b & 1) == 1);
        if (strobe_)
            button_index_ = 0;
    }

   private:
    bool strobe_       = false;
    Byte state_        = 0;
    Byte button_index_ = 0;
    std::mutex mtx_;
};

}  // namespace oxnes
#endif  // NES_TERM_CONTROLLER_HPP
