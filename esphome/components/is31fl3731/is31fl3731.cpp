#include "is31fl3731.h"
#include "esphome/core/log.h"

namespace esphome {
namespace is31fl3731 {

static const uint8_t COMMANDREGISTER = 0xFD;
static const uint8_t REG_CONFIG = 0x00;
static const uint8_t REG_PICTUREFRAME = 0x01;
static const uint8_t REG_SHUTDOWN = 0x0A;
static const uint8_t BANK_FUNCTIONREG = 0x0B;

static const uint8_t CFG_CONFIG_PICTUREMODE = 0x00;
static const uint8_t CFG_SHUTDOWN_NORMAL = 0x01;

static const uint8_t REG_LED_CONTROL_START = 0x00;
static const uint8_t REG_LED_CONTROL_END = 0x11;
static const uint8_t REG_LED_PWD_START = 0x24;
static const uint8_t REG_LED_PWD_END = 0xB3;

static const uint8_t NUM_FRAMES = 8;
static const uint8_t NUM_LEDS = 144;

static const char *const TAG = "is31fl3731.display";

void IS31FL3731Component::set_writer(is31fl3731_writer_t &&writer) {
  this->writer_ = writer;
}

void IS31FL3731Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up IS31FL3731...");
  
  select_bank(BANK_FUNCTIONREG);

  write_byte(REG_SHUTDOWN, CFG_SHUTDOWN_NORMAL);

  select_bank(BANK_FUNCTIONREG);
  write_byte(REG_CONFIG, CFG_CONFIG_PICTUREMODE);

  for (uint8_t frame = 0; frame < NUM_FRAMES; frame++) {
    select_bank(frame);
    power_leds(true);
  }
}

void IS31FL3731Component::dump_config() {
  ESP_LOGCONFIG(TAG, "IS31FL3731:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with IS31FL3731 failed!");
  }
}

float IS31FL3731Component::get_setup_priority() const { return setup_priority::PROCESSOR; }

void IS31FL3731Component::loop() {
}

void IS31FL3731Component::update() {
  if(writer_.has_value()) {
    ESP_LOGV(TAG, "Call lambda");
    if (current_frame_ == (NUM_FRAMES - 1)) {
      current_frame_ = 0;
    } else {
      current_frame_++;
    }
    select_bank(current_frame_);

    fill(display::COLOR_OFF);
    (*writer_)(*this);
  }
  display_frame(current_frame_);
}

display::DisplayType IS31FL3731Component::get_display_type() {
  return display::DisplayType::DISPLAY_TYPE_GRAYSCALE;
}

void IS31FL3731Component::fill(Color color) {
  ESP_LOGV(TAG, "Fill with color %d", color.w);

  select_bank(current_frame_);

  // TODO: why is this split into 6 iterations?
  const uint8_t iterations = 6;
  const uint8_t leds_per_iteration = NUM_LEDS / iterations;

  auto fill_buffer = std::array<uint8_t, leds_per_iteration>();
  fill_buffer.fill(color.w);

  for (uint8_t i = 0; i < iterations; i++) {
    const auto pwm_reg = REG_LED_PWD_START + i * leds_per_iteration;
    const bool written = write_bytes(pwm_reg, fill_buffer);
    if (!written) {
      ESP_LOGE(TAG, "Failed to fill line %d", i);
    }
  }
}

void IS31FL3731Component::draw_pixel_at(int x, int y, Color color) {
  switch (this->rotation_) {
    case display::DISPLAY_ROTATION_0_DEGREES:
      break;
    case display::DISPLAY_ROTATION_90_DEGREES:
      std::swap(x, y);
      x = get_width_internal() - x - 1;
      break;
    case display::DISPLAY_ROTATION_180_DEGREES:
      x = get_width_internal() - x - 1;
      y = get_height_internal() - y - 1;
      break;
    case display::DISPLAY_ROTATION_270_DEGREES:
      std::swap(x, y);
      y = get_height_internal() - y - 1;
      break;
  }

  const uint8_t lednum = x + y * get_width_internal();

  this->set_led_pwm(lednum, color.w, current_frame_);
}

int IS31FL3731Component::get_height_internal() { return height_; }

int IS31FL3731Component::get_width_internal() {
  return width_;
}

void IS31FL3731Component::display_frame(uint8_t bank) {
  ESP_LOGV(TAG, "Display frame %d", bank);
  select_bank(BANK_FUNCTIONREG);

  const bool result = write_byte(REG_PICTUREFRAME, bank);
  if (!result) {
    ESP_LOGE(TAG, "Failed to display frame %d", bank);
  }
}

void IS31FL3731Component::select_bank(uint8_t bank) {
  ESP_LOGV(TAG, "Select bank %d", bank);
  write_byte(COMMANDREGISTER, bank);
}

void IS31FL3731Component::set_led_pwm(uint8_t lednum, uint8_t pwm, uint8_t bank) {
  ESP_LOGV(TAG, "Set LED PWM %d %d %d", lednum, pwm, bank);
  if (lednum >= NUM_LEDS) {
    ESP_LOGE(TAG, "Failed to set LED PWM %d, lednum out of range", lednum);
    return;
  }

  select_bank(bank);

  const bool result = this->write_byte(REG_LED_PWD_START + lednum, pwm);
  if (!result) {
    ESP_LOGE(TAG, "Failed to set LED PWM %d", lednum);
  }
}

void IS31FL3731Component::power_leds(bool on) {
  ESP_LOGV(TAG, "Power LEDs %d", on);
  for (uint8_t i = REG_LED_CONTROL_START; i <= REG_LED_CONTROL_END; i++) {
    write_byte(i, on ? 0xff : 0x00); // each 8 LEDs on/off
  }
}

}
}
