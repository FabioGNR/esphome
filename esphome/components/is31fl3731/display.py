import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import i2c, display

from esphome.const import (
    CONF_ID,
    CONF_LAMBDA,
)

DEPENDENCIES = ["i2c"]

is31fl3731_ns = cg.esphome_ns.namespace("is31fl3731")

IS31FL3731Component = is31fl3731_ns.class_(
    "IS31FL3731Component",
    i2c.I2CDevice,
    display.Display,
    cg.PollingComponent,
)

IS31FL3731ComponentRef = IS31FL3731Component.operator("ref")


CONFIG_SCHEMA = (
    display.BASIC_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(IS31FL3731Component),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x76))
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await i2c.register_i2c_device(var, config)
    await display.register_display(var, config)

    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(IS31FL3731ComponentRef, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))
