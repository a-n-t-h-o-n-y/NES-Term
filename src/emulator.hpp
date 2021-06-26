#ifndef NES_TERM_EMULATOR_HPP
#define NES_TERM_EMULATOR_HPP
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>

#include <CPU.h>
#include <Cartridge.h>
#include <MainBus.h>
#include <Mapper.h>
#include <PPU.h>
#include <PictureBus.h>
#include <framebuffer.hpp>

#include "controller.hpp"

namespace oxnes {

class Emulator {
   public:
    Emulator()
        : main_bus_{}, cpu_{main_bus_}, picture_bus_{}, ppu_{picture_bus_}
    {
        if (!main_bus_.setReadCallback(sn::PPUSTATUS,
                                       [&] { return ppu_.getStatus(); }) ||
            !main_bus_.setReadCallback(sn::PPUDATA,
                                       [&](void) { return ppu_.getData(); }) ||
            !main_bus_.setReadCallback(sn::OAMDATA,
                                       [&] { return ppu_.getOAMData(); })) {
            throw std::runtime_error{
                "Critical error: Failed to set read I/O callbacks"};
        }

        if (!main_bus_.setWriteCallback(sn::PPUCTRL,
                                        [&](sn::Byte b) { ppu_.control(b); }) ||
            !main_bus_.setWriteCallback(sn::PPUMASK,
                                        [&](sn::Byte b) { ppu_.setMask(b); }) ||
            !main_bus_.setWriteCallback(
                sn::OAMADDR, [&](sn::Byte b) { ppu_.setOAMAddress(b); }) ||
            !main_bus_.setWriteCallback(
                sn::PPUADDR, [&](sn::Byte b) { ppu_.setDataAddress(b); }) ||
            !main_bus_.setWriteCallback(
                sn::PPUSCROL, [&](sn::Byte b) { ppu_.setScroll(b); }) ||
            !main_bus_.setWriteCallback(sn::PPUDATA,
                                        [&](sn::Byte b) { ppu_.setData(b); }) ||
            !main_bus_.setWriteCallback(sn::OAMDMA,
                                        [&](sn::Byte b) { DMA(b); }) ||
            !main_bus_.setWriteCallback(
                sn::OAMDATA, [&](sn::Byte b) { ppu_.setOAMData(b); })) {
            throw std::runtime_error{
                "Critical error: Failed to set write I/O callbacks"};
        }

        ppu_.setInterruptCallback([&]() { cpu_.interrupt<sn::CPU::NMI>(); });
    }

   public:
    void set_controller1_read_callback(std::function<sn::Byte()> fn)
    {
        if (!main_bus_.setReadCallback(sn::JOY1, std::move(fn)))
            throw std::runtime_error("Could not set controller1 read callback");
    }

    void set_controller2_read_callback(std::function<sn::Byte()> fn)
    {
        if (!main_bus_.setReadCallback(sn::JOY2, std::move(fn)))
            throw std::runtime_error("Could not set controller2 read callback");
    }

    void set_controller_write_callback(std::function<void(sn::Byte)> fn)
    {
        if (!main_bus_.setWriteCallback(sn::JOY1, std::move(fn)))
            throw std::runtime_error("Could not set controller write callback");
    }

    void load_cartridge(std::string const& rom_path)
    {
        if (!cartridge_.loadFromFile(rom_path))
            throw std::runtime_error{"Failed to load ROM: " + rom_path};

        mapper_ptr_ = sn::Mapper::createMapper(
            static_cast<sn::Mapper::Type>(cartridge_.getMapper()), cartridge_,
            [&] { picture_bus_.updateMirroring(); });

        if (mapper_ptr_ == nullptr)
            throw std::runtime_error{"Mapper failed to be created."};

        if (!main_bus_.setMapper(mapper_ptr_.get()) ||
            !picture_bus_.setMapper(mapper_ptr_.get())) {
            throw std::runtime_error{"Error setting mappers."};
        }

        cpu_.reset();
        ppu_.reset();
    }

    void step()
    {
        ppu_.step();
        ppu_.step();
        ppu_.step();

        cpu_.step();
    }

    void register_draw_callback(std::function<void(sn::Framebuffer const&)> fn)
    {
        ppu_.register_draw_callback(std::move(fn));
    }

   private:
    sn::MainBus main_bus_;
    sn::CPU cpu_;
    sn::PictureBus picture_bus_;
    sn::PPU ppu_;
    sn::Cartridge cartridge_;
    std::unique_ptr<sn::Mapper> mapper_ptr_;

   private:
    void DMA(sn::Byte page)
    {
        cpu_.skipDMACycles();
        ppu_.doDMA(main_bus_.getPagePtr(page));
    }
};

}  // namespace oxnes
#endif  // NES_TERM_EMULATOR_HPP
