#include "pulsar_rs485.h"
#include "esphome/core/log.h"
#include <algorithm>


namespace esphome {
namespace pulsar_rs485 {

static const char *const TAG = "pulsar_rs485";

//
// Запрос
//
static const uint8_t REQUEST_ADDR[] = {
    0x09, 0x24, 0x86, 0x22, // сетевой адрес устройства (4 байта). Адрес зашит в водосчётчике, указывается в конфигурации сенсора
};
static const uint8_t REQUEST_FL[] = {
    0x01, // код функции запроса (1 байт). Код функции чтения текущих показаний у Pulsar RS485 — 0x01
    0x0e, // общая длина пакета (1 байт). Длина запроса для функции чтения текущих показаний у Pulsar RS485 — 14 байт (0x0E)
};
static const uint8_t REQUEST_MASK[] = {
    0x01, 0x00, 0x00, 0x00, // битовая маска запрашиваемых каналов (4 байта). Маска у Pulsar RS485 - 0x01, 0x00, 0x00, 0x00
};
static const uint8_t REQUEST_ID[] = {
    0xFD, 0xEC, // идентификатор запроса (2 байта). Любые два байта
};
static const uint8_t REQUEST_CRC[] = {
    0xcf, 0x78, // идентификатор запроса (2 байта). Любые два байта
};

// Полный запрос
uint8_t REQUEST[sizeof(REQUEST_ADDR) + sizeof(REQUEST_FL) + sizeof(REQUEST_MASK) + sizeof(REQUEST_ID) + sizeof(REQUEST_CRC)];
std::copy_n(
    REQUEST_FL, sizeof(REQUEST_FL), std::copy_n(
        REQUEST_ADDR, sizeof(REQUEST_ADDR), REQUEST
    )
);

//
// Ответ
//
static const uint8_t RESPONSE_ADDR[] = {
    0x09, 0x24, 0x86, 0x22, // сетевой адрес устройства (4 байта). Адрес зашит в водосчётчике, указывается в конфигурации сенсора
};
static const uint8_t RESPONSE_FL[] = {
    0x01, // код функции ответа (1 байт). Код функции чтения текущих показаний у Pulsar RS485 — 0x01
    0x0E, // общая длина пакета (1 байт). Длина ответа для функции чтения текущих показаний у Pulsar RS485 — 14 байт (0x0E)
};
static const uint8_t RESPONSE_ID[] = {
    0xFD, 0xEC, // идентификатор запроса (2 байта). В ответе возвращается идентификатор запроса, вызвавшего ответ
};

// Нарастающий размер ответа после добавления частей
static const uint8_t RESPONSE_SIZE_ADDR = sizeof(RESPONSE_ADDR);                    // 4
static const uint8_t RESPONSE_SIZE_FL   = RESPONSE_SIZE_ADDR + sizeof(RESPONSE_FL); // 6
static const uint8_t RESPONSE_SIZE_DATA = RESPONSE_SIZE_FL + 4;                     // 10 (данные - 4 байта)
static const uint8_t RESPONSE_SIZE_ID   = RESPONSE_SIZE_DATA + sizeof(RESPONSE_ID); // 12
static const uint8_t RESPONSE_SIZE_CRC  = RESPONSE_SIZE_ID + 2;                     // 14 (контрольная сумма - 2 байта)


//void PulsarRS485Component::setup() {
  // because this implementation is currently rx-only, there is nothing to setup
//}

void PulsarRS485Component::dump_config() {
  ESP_LOGCONFIG(TAG, "Pulsar RS485:");
  LOG_SENSOR("  ", "PM2.5", this->pm_2_5_sensor_);
  LOG_SENSOR("  ", "Cold Sensor", this->cold_sensor_);
  LOG_SENSOR("  ", "Hot Sensor", this->hot_sensor_);
  LOG_UPDATE_INTERVAL(this);
  this->check_uart_settings(9600);
}

void PulsarRS485Component::update() {
  ESP_LOGD(TAG, "sending measurement request");
  this->write_array(REQUEST, sizeof(REQUEST));
}

void PulsarRS485Component::loop() {
  while (this->available() != 0) {
    ESP_LOGD(TAG, "available %i", this->available());

    // Читаем следующий байт в буфер
    this->read_byte(&this->data_[this->data_index_]);

    auto check = this->check_byte_();

    // нет значения
    if (!check.has_value()) {
      // finished
      ESP_LOGD(TAG, "!check.has_value() - корректный байт, сообщение закончилось; парсить сообщение, сбросить буфер");

      this->parse_data_();

      this->data_index_ = 0;
      ESP_LOGD(TAG, "data_index_ = %i", this->data_index_);
    }
    // значение - ложь
    else if (!check.value()) {
      // wrong data
      ESP_LOGD(TAG, "!check.value() - некорректный байт; сбросить буфер");

      this->data_index_ = 0;
      ESP_LOGD(TAG, "data_index_ = %i", this->data_index_);
    }
    // значение - истина
    else {
      // next byte
      ESP_LOGD(TAG, "else - корректный байт, сообщение ещё не закончилось; читать дальше в буфер");

      this->data_index_++;
      ESP_LOGD(TAG, "data_index_ = %i", this->data_index_);
    }
  }
}

//float PulsarRS485Component::get_setup_priority() const { return setup_priority::DATA; }

uint8_t PulsarRS485Component::pulsar_rs485_checksum_(const uint8_t *command_data, uint8_t length) const {
  uint8_t sum = 0;
  for (uint8_t i = 0; i < length; i++) {
    sum += command_data[i];
  }
  return sum;
}

optional<bool> PulsarRS485Component::check_byte_() const {
  uint8_t index = this->data_index_;
  uint8_t byte = this->data_[index];

  // Адрес
  if (index < RESPONSE_SIZE_ADDR) {
    return byte == RESPONSE_ADDR[0 + index];
  }

  // Код и длина
  if (index < RESPONSE_SIZE_FL) {
    return byte == RESPONSE_FL[-RESPONSE_SIZE_ADDR + index];
  }

  // Данные - без проверки
  if (index < RESPONSE_SIZE_DATA) {
    return true;
  }

  // Идентификатор
  if (index < RESPONSE_SIZE_ID) {
    return byte == RESPONSE_ID[-RESPONSE_SIZE_DATA + index];
  }

  // Контрольная сумма — без проверки
  if (index < RESPONSE_SIZE_CRC) {
    return true;
  }

  // checksum
//  if (index == (sizeof(PULSAR_RS485_RESPONSE_HEADER) + 16)) {
//    uint8_t checksum = pulsar_rs485_checksum_(this->data_, sizeof(PULSAR_RS485_RESPONSE_HEADER) + 17);
//    if (checksum != 0) {
//      ESP_LOGW(TAG, "PULSAR_RS485 checksum is wrong: %02x, expected zero", checksum);
//      return false;
//    }
//    return {};
//  }

  return false;
}

void PulsarRS485Component::parse_data_() {
  const int pm_2_5_concentration = this->get_16_bit_uint_(5);

  ESP_LOGD(TAG, "Got PM2.5 Concentration: %d µg/m³", pm_2_5_concentration);

  if (this->pm_2_5_sensor_ != nullptr) {
    this->pm_2_5_sensor_->publish_state(pm_2_5_concentration);
  }
}

uint16_t PulsarRS485Component::get_16_bit_uint_(uint8_t start_index) const {
  return encode_uint16(this->data_[start_index], this->data_[start_index + 1]);
}

}  // namespace pulsar_rs485
}  // namespace esphome
