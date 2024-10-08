#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace pulsar_rs485 {

class PulsarRS485Component : public PollingComponent, public uart::UARTDevice {
 public:
  PulsarRS485Component() = default;

  void set_pm_2_5_sensor(sensor::Sensor *pm_2_5_sensor) { this->pm_2_5_sensor_ = pm_2_5_sensor; }
  void set_cold_sensor(sensor::Sensor *cold_sensor) { this->cold_sensor_ = cold_sensor; }
  void set_hot_sensor(sensor::Sensor *hot_sensor) { this->hot_sensor_ = hot_sensor; }

  //void setup() override;
  void dump_config() override;
  void loop() override;
  void update() override;

 protected:
  sensor::Sensor *pm_2_5_sensor_{nullptr};
  sensor::Sensor *cold_sensor_{nullptr};
  sensor::Sensor *hot_sensor_{nullptr};

  uint8_t data_[14];
  uint8_t data_index_{0};

  optional<bool> check_byte_() const;
  void parse_data_();
  uint16_t get_16_bit_uint_(uint8_t start_index) const;
  uint8_t pulsar_rs485_checksum_(const uint8_t *command_data, uint8_t length) const;
};

}  // namespace pulsar_rs485
}  // namespace esphome
