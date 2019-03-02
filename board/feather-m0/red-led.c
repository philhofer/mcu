#include <gpio.h>
#include <board.h>
#include <drivers/sam/port.h>
#include <red-led.h>

/* Feather-M0 red LED light is on PA17 */
PORT_GPIO_PIN(red_led, PINGRP_A, 17);

