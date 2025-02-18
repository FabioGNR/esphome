#include "rotary_encoder.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace rotary_encoder_special {

static const char *const TAG = "rotary_encoder_special";

// Only apply if DRAM_ATTR exists on this platform (exists on ESP32, not on ESP8266)
#ifndef DRAM_ATTR
#define DRAM_ATTR
#endif


void IRAM_ATTR HOT RotaryEncoderSensorStore::gpio_intr(RotaryEncoderSensorStore *arg) {
  bool pin_a = arg->pin_a.digital_read();
  bool pin_b = arg->pin_b.digital_read();
  if (arg->last_a_state && arg->last_b_state) {
    if (!pin_a) {
      if (arg->counter > arg->min_value) {
        arg->counter--;
      }
    } else if (!pin_b) {
      if (arg->counter < arg->max_value) {
        arg->counter++;
      }
    }
  }
  arg->last_a_state = pin_a;
  arg->last_b_state = pin_b;
}

void RotaryEncoderSensor::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Rotary Encoder '%s'...", this->name_.c_str());

  int32_t initial_value = 0;
  switch (this->restore_mode_) {
    case ROTARY_ENCODER_RESTORE_DEFAULT_ZERO:
      this->rtc_ = global_preferences->make_preference<int32_t>(this->get_object_id_hash());
      if (!this->rtc_.load(&initial_value)) {
        initial_value = 0;
      }
      break;
    case ROTARY_ENCODER_ALWAYS_ZERO:
      initial_value = 0;
      break;
  }
  initial_value = clamp(initial_value, this->store_.min_value, this->store_.max_value);

  this->store_.counter = initial_value;

  this->pin_a_->setup();
  this->store_.pin_a = this->pin_a_->to_isr();
  this->pin_b_->setup();
  this->store_.pin_b = this->pin_b_->to_isr();

  this->pin_a_->attach_interrupt(RotaryEncoderSensorStore::gpio_intr, &this->store_, gpio::INTERRUPT_ANY_EDGE);
  this->pin_b_->attach_interrupt(RotaryEncoderSensorStore::gpio_intr, &this->store_, gpio::INTERRUPT_ANY_EDGE);
}
void RotaryEncoderSensor::dump_config() {
  LOG_SENSOR("", "Rotary Encoder", this);
  LOG_PIN("  Pin A: ", this->pin_a_);
  LOG_PIN("  Pin B: ", this->pin_b_);

  const LogString *restore_mode;
  switch (this->restore_mode_) {
    case ROTARY_ENCODER_RESTORE_DEFAULT_ZERO:
      restore_mode = LOG_STR("Restore (Defaults to zero)");
      break;
    case ROTARY_ENCODER_ALWAYS_ZERO:
      restore_mode = LOG_STR("Always zero");
      break;
    default:
      restore_mode = LOG_STR("");
  }
  ESP_LOGCONFIG(TAG, "  Restore Mode: %s", LOG_STR_ARG(restore_mode));
}

void RotaryEncoderSensor::loop() {
  int counter = this->store_.counter;
  if (this->store_.last_read != counter || this->publish_initial_value_) {
    if (this->restore_mode_ == ROTARY_ENCODER_RESTORE_DEFAULT_ZERO) {
      this->rtc_.save(&counter);
    }
    this->store_.last_read = counter;
    this->publish_state(counter);
    this->listeners_.call(counter);
    this->publish_initial_value_ = false;
  }
}

float RotaryEncoderSensor::get_setup_priority() const { return setup_priority::DATA; }
void RotaryEncoderSensor::set_restore_mode(RotaryEncoderRestoreMode restore_mode) {
  this->restore_mode_ = restore_mode;
}
void RotaryEncoderSensor::set_min_value(int32_t min_value) { this->store_.min_value = min_value; }
void RotaryEncoderSensor::set_max_value(int32_t max_value) { this->store_.max_value = max_value; }

}  // namespace rotary_encoder
}  // namespace esphome
