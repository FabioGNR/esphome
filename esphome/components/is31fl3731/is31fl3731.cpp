#include "is31fl3731.h"
#include "esphome/core/log.h"

namespace esphome {
namespace is31fl3731 {

static const uint8_t ISSI_REG_CONFIG_PICTUREMODE = 0x00;

static const uint8_t ISSI_COMMANDREGISTER = 0xFD;
static const uint8_t ISSI_REG_PICTUREFRAME = 0x01;
static const uint8_t ISSI_REG_SHUTDOWN = 0x0A;
static const uint8_t ISSI_BANK_FUNCTIONREG = 0x0B;

static const char *const TAG = "is31fl3731.display";

void IS31FL3731Component::set_writer(is31fl3731_writer_t &&writer) {
  this->writer_ = writer;
}

void IS31FL3731Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up IS31FL3731...");
  
  select_bank(ISSI_BANK_FUNCTIONREG);
  write_byte(ISSI_REG_SHUTDOWN, 0x01);

  select_bank(ISSI_BANK_FUNCTIONREG);
  write_byte(ISSI_REG_SHUTDOWN, ISSI_REG_CONFIG_PICTUREMODE);
}

void IS31FL3731Component::dump_config() {
  ESP_LOGCONFIG(TAG, "IS31FL3731:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with IS31FL3731 failed!");
  }
}

float IS31FL3731Component::get_setup_priority() const { return setup_priority::DATA; }

void IS31FL3731Component::loop() {
  display_frame(0);
}

void IS31FL3731Component::update() {
  //
  if(writer_.has_value()) {
    (*writer_)(*this);
  }
}

display::DisplayType IS31FL3731Component::get_display_type() {
  return display::DisplayType::DISPLAY_TYPE_GRAYSCALE;
}

void IS31FL3731Component::fill(Color color) {
  select_bank(0);
  auto fill_buffer = std::array<uint8_t, 24>();
  fill_buffer.fill(color.w);

  for (uint8_t i = 0; i < 6; i++) {
    write_bytes(0x24 + i * 24, fill_buffer);
  }
}

void IS31FL3731Component::draw_pixel_at(int x, int y, Color color) {
  const uint8_t lednum = x + y * get_width_internal();

  this->set_led_pwm(lednum, color.w, 0);
}

int IS31FL3731Component::get_height_internal() { return 9; }

int IS31FL3731Component::get_width_internal() {
  return 16;
}

void IS31FL3731Component::display_frame(uint8_t bank) {
  select_bank(ISSI_BANK_FUNCTIONREG); // TODO: check result

  const bool result = write_byte(ISSI_REG_PICTUREFRAME, bank);
  if (!result) {
    ESP_LOGE(TAG, "Failed to display frame %d", bank);
  }
}

void IS31FL3731Component::select_bank(uint8_t bank) {
  const bool result = write_byte(ISSI_COMMANDREGISTER, bank);
  if (!result) {
    ESP_LOGE(TAG, "Failed to select bank %d", bank);
  }
}

void IS31FL3731Component::set_led_pwm(uint8_t lednum, uint8_t pwm, uint8_t bank) {
  if (lednum >= 144)
    return;

  this->select_bank(bank); // TODO: check result

  const bool result = this->write_byte(0x24 + lednum, pwm);
  if (!result) {
    ESP_LOGE(TAG, "Failed to set LED PWM %d", lednum);
  }
}

}
}