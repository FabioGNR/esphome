#include "is31fl3731.h"
#include "esphome/core/log.h"

namespace esphome {
namespace is31fl3731 {

static const uint8_t ISSI_REG_CONFIG_PICTUREMODE = 0x00;

static const uint8_t ISSI_COMMANDREGISTER = 0xFD;
static const uint8_t ISSI_REG_CONFIG = 0x00;
static const uint8_t ISSI_REG_PICTUREFRAME = 0x01;
static const uint8_t ISSI_REG_SHUTDOWN = 0x0A;
static const uint8_t ISSI_BANK_FUNCTIONREG = 0x0B;

static const uint8_t NUM_FRAMES = 8;

static const char *const TAG = "is31fl3731.display";

void IS31FL3731Component::set_writer(is31fl3731_writer_t &&writer) {
  this->writer_ = writer;
}

void IS31FL3731Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up IS31FL3731...");
  
  select_bank(ISSI_BANK_FUNCTIONREG);
  write_byte(ISSI_REG_SHUTDOWN, 0x01);

  select_bank(ISSI_BANK_FUNCTIONREG);
  write_byte(ISSI_REG_CONFIG, ISSI_REG_CONFIG_PICTUREMODE);

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
  auto fill_buffer = std::array<uint8_t, 24>();
  fill_buffer.fill(color.w);

  for (uint8_t i = 0; i < 6; i++) {
    const bool written = write_bytes(0x24 + i * 24, fill_buffer);
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

int IS31FL3731Component::get_height_internal() { return 9; }

int IS31FL3731Component::get_width_internal() {
  return 16;
}

void IS31FL3731Component::display_frame(uint8_t bank) {
  ESP_LOGV(TAG, "Display frame %d", bank);
  select_bank(ISSI_BANK_FUNCTIONREG);

  const bool result = write_byte(ISSI_REG_PICTUREFRAME, bank);
  if (!result) {
    ESP_LOGE(TAG, "Failed to display frame %d", bank);
  }
}

void IS31FL3731Component::select_bank(uint8_t bank) {
  ESP_LOGV(TAG, "Select bank %d", bank);
  write_byte(ISSI_COMMANDREGISTER, bank);
}

void IS31FL3731Component::set_led_pwm(uint8_t lednum, uint8_t pwm, uint8_t bank) {
  ESP_LOGV(TAG, "Set LED PWM %d %d %d", lednum, pwm, bank);
  if (lednum >= 144) {
    ESP_LOGE(TAG, "Failed to set LED PWM %d, lednum out of range", lednum);
    return;
  }

  select_bank(bank);

  const bool result = this->write_byte(0x24 + lednum, pwm);
  if (!result) {
    ESP_LOGE(TAG, "Failed to set LED PWM %d", lednum);
  }
}

void IS31FL3731Component::power_leds(bool on) {
  ESP_LOGV(TAG, "Power LEDs %d", on);
  for (uint8_t i = 0; i <= 0x11; i++) {
    write_byte(i, on ? 0xff : 0x00); // each 8 LEDs on
  }
}

}
}
