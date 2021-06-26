#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <termox/termox.hpp>

#include "emulator.hpp"
#include "termox/painter/color.hpp"

namespace {

constexpr auto nes_width  = 256;
constexpr auto nes_height = 240;

[[nodiscard]] auto custom_nes_palette() -> ox::Palette
{
    using ox::RGB;
    return ox::make_palette(
        RGB{0x666666}, RGB{0x002a88}, RGB{0x1412a7}, RGB{0x3b00a4},
        RGB{0x5c007e}, RGB{0x6e0040}, RGB{0x6c0600}, RGB{0x561d00},
        RGB{0x333500}, RGB{0x0b4800}, RGB{0x005200}, RGB{0x004f08},
        RGB{0x00404d}, RGB{0x000000}, RGB{0x000000}, RGB{0x000000},
        RGB{0xadadad}, RGB{0x155fd9}, RGB{0x4240ff}, RGB{0x7527fe},
        RGB{0xa01acc}, RGB{0xb71e7b}, RGB{0xb53120}, RGB{0x994e00},
        RGB{0x6b6d00}, RGB{0x388700}, RGB{0x0c9300}, RGB{0x008f32},
        RGB{0x007c8d}, RGB{0x000000}, RGB{0x000000}, RGB{0x000000},
        RGB{0xfffeff}, RGB{0x64b0ff}, RGB{0x9290ff}, RGB{0xc676ff},
        RGB{0xf36aff}, RGB{0xfe6ecc}, RGB{0xfe8170}, RGB{0xea9e22},
        RGB{0xbcbe00}, RGB{0x88d800}, RGB{0x5ce430}, RGB{0x45e082},
        RGB{0x48cdde}, RGB{0x4f4f4f}, RGB{0x000000}, RGB{0x000000},
        RGB{0xfffeff}, RGB{0xc0dfff}, RGB{0xd3d2ff}, RGB{0xe8c8ff},
        RGB{0xfbc2ff}, RGB{0xfec4ea}, RGB{0xfeccc5}, RGB{0xf7d8a5},
        RGB{0xe4e594}, RGB{0xcfef96}, RGB{0xbdf4ab}, RGB{0xb3f3cc},
        RGB{0xb5ebf2}, RGB{0xb8b8b8}, RGB{0x000000}, RGB{0x000000});
}

}  // namespace

namespace nesterm {

class NES_widget
    : public ox::
          Color_graph_static_bounds<int, 0, nes_width - 1, nes_height - 1, 0> {
   private:
    static constexpr auto display_width  = nes_width - 1;
    static constexpr auto display_height = (nes_height - 1) / 2;

    using Base_t =
        Color_graph_static_bounds<int, 0, nes_width - 1, nes_height - 1, 0>;

    using Clock_t = std::chrono::high_resolution_clock;

   public:
    NES_widget(std::string const& rom_path)
    {
        using namespace ox::pipe;

        ox::Terminal::set_palette(custom_nes_palette());

        *this | maximum_width(display_width) | maximum_height(display_height) |
            strong_focus();

        emulator_.register_draw_callback(
            [this, previous_time =
                       Clock_t::now()](sn::Framebuffer const& buf) mutable {
                constexpr auto zero = Clock_t::duration{0};
                constexpr auto period =
                    std::chrono::microseconds{16'639};  // 60.0988139 fps
                auto const to_wait = period - (Clock_t::now() - previous_time);
                if (to_wait > zero)
                    std::this_thread::sleep_for(to_wait);
                previous_time = Clock_t::now();
                next_buffer_  = buf;
            });

        emulator_.set_controller1_read_callback(
            [this] { return controller1_.read(); });
        emulator_.set_controller2_read_callback(
            [this] { return controller2_.read(); });
        emulator_.set_controller_write_callback([this](sn::Byte b) {
            controller1_.strobe(b);
            controller2_.strobe(b);
        });

        emulator_.load_cartridge(rom_path);

        loop_.run_async([this](auto& queue) {
            while (!next_buffer_.has_value())
                emulator_.step();  // Might set next_buffer_

            queue.append(ox::Custom_event{
                [this, buf = *next_buffer_] { this->handle_next_frame(buf); }});
            next_buffer_ = std::nullopt;
        });
    }

   protected:
    auto key_press_event(ox::Key k) -> bool override
    {
        // TODO key to button function for each player
        switch (k) {
            using ox::Key;
            case Key::z: controller1_.press(Controller::Button::A); break;
            case Key::x: controller1_.press(Controller::Button::B); break;
            case Key::Backspace:
                controller1_.press(Controller::Button::Select);
                break;
            case Key::Enter:
                controller1_.press(Controller::Button::Start);
                break;
            case Key::Arrow_up:
                controller1_.press(Controller::Button::Up);
                break;
            case Key::Arrow_down:
                controller1_.press(Controller::Button::Down);
                break;
            case Key::Arrow_left:
                controller1_.press(Controller::Button::Left);
                break;
            case Key::Arrow_right:
                controller1_.press(Controller::Button::Right);
                break;
            default: break;
        }
        return Base_t::key_press_event(k);
    }

    auto key_release_event(ox::Key k) -> bool override
    {
        // TODO key to button function for each player
        switch (k) {
            using ox::Key;
            case Key::z: controller1_.release(Controller::Button::A); break;
            case Key::x: controller1_.release(Controller::Button::B); break;
            case Key::Backspace:
                controller1_.release(Controller::Button::Select);
                break;
            case Key::Enter:
                controller1_.release(Controller::Button::Start);
                break;
            case Key::Arrow_up:
                controller1_.release(Controller::Button::Up);
                break;
            case Key::Arrow_down:
                controller1_.release(Controller::Button::Down);
                break;
            case Key::Arrow_left:
                controller1_.release(Controller::Button::Left);
                break;
            case Key::Arrow_right:
                controller1_.release(Controller::Button::Right);
                break;
            default: break;
        }
        return Base_t::key_release_event(k);
    }

   private:
    Emulator emulator_;
    Controller controller1_;
    Controller controller2_;

    ox::Event_loop loop_;
    std::optional<sn::Framebuffer> next_buffer_ = std::nullopt;

   private:
    void handle_next_frame(sn::Framebuffer const& buf)
    {
        this->Base_t::reset(translate_to_pairs(buf));
    }

    [[nodiscard]] static auto translate_to_pairs(sn::Framebuffer const& buf)
        -> std::vector<std::pair<Base_t::Coordinate, ox::Color>>
    {
        auto result = std::vector<std::pair<Base_t::Coordinate, ox::Color>>{};
        result.reserve(nes_width * nes_height);
        for (auto x = 0; x < nes_width; ++x) {
            for (auto y = 0; y < nes_height; ++y) {
                result.push_back(
                    {{x, Base_t::boundary().north - y}, ox::Color{buf[x][y]}});
            }
        }
        return result;
    }

    /// Translate ox::Key to gameboy button, if there is a representation.
    // [[nodiscard]] static auto key_to_button(ox::Key k)
    //     -> std::optional<::GbButton>
    // {
    //     using ox::Key;
    //     switch (k) {
    //         case Key::Arrow_up: return ::GbButton::Up;
    //         case Key::Arrow_down: return ::GbButton::Down;
    //         case Key::Arrow_left: return ::GbButton::Left;
    //         case Key::Arrow_right: return ::GbButton::Right;
    //         case Key::z: return ::GbButton::A;
    //         case Key::x: return ::GbButton::B;
    //         case Key::Enter: return ::GbButton::Start;
    //         case Key::Backspace: return ::GbButton::Select;
    //         default: return std::nullopt;
    //     }
    // }
};

struct App : ox::Float_2d<NES_widget> {
    App(std::string const& rom_path) : ox::Float_2d<NES_widget>{rom_path}
    {
        using namespace ox::pipe;
        constexpr auto background = ox::Color{14};
        *this | direct_focus() | forward_focus(this->widget.widget);
        this->buffer_1 | bg(background);
        this->buffer_2 | bg(background);
        this->widget.buffer_1 | bg(background);
        this->widget.buffer_2 | bg(background);
    }
};

}  // namespace nesterm

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Please pass ROM path as first parameter.\n";
        return 1;
    }

    auto const rom_path = argv[1];

    return ox::System{ox::Mouse_mode::Basic, ox::Key_mode::Raw}
        .run<nesterm::App>(rom_path);
}
