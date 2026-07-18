#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/light/light_output.h"
#include "rbdimmer_hub.h"

extern "C" {
#include <rbdimmerESP32.h>
}

namespace esphome {
namespace rbdimmer {

static const char *const TAG_LIGHT = "rbdimmer.light";

class RBDimmerLight : public light::LightOutput, public Component {
 public:
  void set_hub(RBDimmerHub *hub) { this->hub_ = hub; }
  void set_pin(uint8_t pin) { this->pin_ = pin; }
  void set_phase(uint8_t phase) { this->phase_ = phase; }

  void setup() override {
    if (this->hub_ == nullptr || !this->hub_->is_initialized()) {
      ESP_LOGE(TAG_LIGHT, "Hub not initialized");
      this->mark_failed();
      return;
    }

    rbdimmer_config_t config = {
        .gpio_pin = this->pin_,
        .phase = this->phase_,
        .initial_level = 0,
    };

    rbdimmer_err_t err = rbdimmer_create_channel(&config, &this->channel_);
    if (err != RBDIMMER_OK) {
      ESP_LOGE(TAG_LIGHT, "Failed to create channel on pin %d: %d", this->pin_, err);
      this->mark_failed();
      return;
    }

    ESP_LOGI(TAG_LIGHT, "Channel created: pin=%d, phase=%d", this->pin_, this->phase_);
  }

  light::LightTraits get_traits() override {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
    return traits;
  }

  void write_state(light::LightState *state) override {
    if (this->channel_ == nullptr)
      return;

    float brightness;
    state->current_values_as_brightness(&brightness);

    // Library range is 0-10000 (conduction time in microseconds at 50 Hz,
    // half-cycle = ~10000us); use round() to avoid float truncation.
    uint16_t level = static_cast<uint16_t>(roundf(brightness * 10000.0f));
    if (level > 10000) level = 10000;
    rbdimmer_set_level(this->channel_, level);
  }

  void dump_config() override {
    ESP_LOGCONFIG(TAG_LIGHT, "RBDimmer Light:");
    ESP_LOGCONFIG(TAG_LIGHT, "  Pin: %d", this->pin_);
    ESP_LOGCONFIG(TAG_LIGHT, "  Phase: %d", this->phase_);
  }

  float get_setup_priority() const override { return setup_priority::HARDWARE - 1.0f; }

  rbdimmer_channel_t *get_channel() { return this->channel_; }

 protected:
  RBDimmerHub *hub_{nullptr};
  uint8_t pin_{0};
  uint8_t phase_{0};
  rbdimmer_channel_t *channel_{nullptr};
};

}  // namespace rbdimmer
}  // namespace esphome
