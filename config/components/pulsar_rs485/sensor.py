import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, uart
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    CONF_PM_2_5,
    DEVICE_CLASS_PM25,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    DEVICE_CLASS_WATER,
    UNIT_MICROGRAMS_PER_CUBIC_METER,
    UNIT_CUBIC_METER,
    ICON_BLUR,
    ICON_WATER,
)

DEPENDENCIES = ["uart"]

pulsar_rs485_ns = cg.esphome_ns.namespace("pulsar_rs485")
PulsarRS485Component = pulsar_rs485_ns.class_(
    "PulsarRS485Component", uart.UARTDevice, cg.PollingComponent
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PulsarRS485Component),
            cv.Optional(CONF_PM_2_5): sensor.sensor_schema(
                unit_of_measurement=UNIT_MICROGRAMS_PER_CUBIC_METER,
                icon=ICON_BLUR,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_PM25,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional("cold"): sensor.sensor_schema(
                unit_of_measurement=UNIT_CUBIC_METER,
                icon=ICON_WATER,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_WATER,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
            cv.Optional("hot"): sensor.sensor_schema(
                unit_of_measurement=UNIT_CUBIC_METER,
                icon=ICON_WATER,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_WATER,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
        }
    )
    # .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.polling_component_schema("1m"))
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "pulsar_rs485",
    baud_rate=9600,
    require_tx=True,
    require_rx=True,
    data_bits=8,
    parity="NONE",
    stop_bits=1,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_PM_2_5 in config:
        sens = await sensor.new_sensor(config[CONF_PM_2_5])
        cg.add(var.set_pm_2_5_sensor(sens))

    if "cold" in config:
        sens = await sensor.new_sensor(config["cold"])
        cg.add(var.set_cold_sensor(sens))

    if "hot" in config:
        sens = await sensor.new_sensor(config["hot"])
        cg.add(var.set_hot_sensor(sens))

