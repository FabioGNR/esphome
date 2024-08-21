#pragma once

#include "esphome/core/component.h"
#include "esphome/components/display/display_buffer.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace is31fl3731 {

class IS31FL3731Component;

using is31fl3731_writer_t = std::function<void(IS31FL3731Component &)>;


class IS31FL3731Component : public display::Display,
                            public i2c::I2CDevice {
 public:
  void set_writer(is31fl3731_writer_t &&writer);

  // ========== INTERNAL METHODS ==========
  // (In most use cases you won't need these)
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override;
  void loop() override;
  void update() override;

  display::DisplayType get_display_type() override;
  void fill(Color color) override;
  void draw_pixel_at(int x, int y, Color color) override;
  int get_height_internal() override;
  int get_width_internal() override;
 protected:
  void display_frame(uint8_t bank);
  void select_bank(uint8_t bank);
  void set_led_pwm(uint8_t lednum, uint8_t pwm, uint8_t bank);
  void power_leds(bool on);

  optional<is31fl3731_writer_t> writer_{};
  bool forced_setup_ {};
};

}  // namespace is31fl3731
}  // namespace esphome
