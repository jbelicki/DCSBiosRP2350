#ifndef __DCSBIOS_LEDS_H
#define __DCSBIOS_LEDS_H

#include <stdint.h>
#include "pico/stdlib.h"
#include "ExportStreamListener.h"
#include "aw9523b.h"
#include "ws2812.h"

namespace DcsBios {

	class LED : public Int16Buffer {
	private:
		unsigned int mask;
		bool reverse;
		bool useExpander;

		// For GPIO
		unsigned char gpioPin;

		// For AW9523B
		AW9523B* expander;
		uint8_t expanderPin;

		// For WS2812
		bool useWs2812 = false;
		uint8_t ws2812Pixel = 0;
		WS2812* ws2812Strip = nullptr;
		uint32_t rgbColor = 0;
		uint8_t brightness_ = 255;

	public:
		// GPIO LED
		LED(unsigned int address, unsigned int mask, uint8_t pin, bool reverse = false)
			: Int16Buffer(address), mask(mask), reverse(reverse),
			  useExpander(false), gpioPin(pin),
			  expander(nullptr), expanderPin(0), brightness_(255) {
			gpio_init(pin);
			gpio_set_dir(pin, GPIO_OUT);
		}

		// AW9523B LED
		LED(unsigned int address, unsigned int mask, AW9523B* expander, uint8_t pin, bool reverse = false, uint8_t brightness = 255)
			: Int16Buffer(address), mask(mask), reverse(reverse),
			useExpander(true), gpioPin(0),
			expander(expander), expanderPin(pin), brightness_(brightness) {
			if (expander) {
				expander->enableLED(pin);
				expander->setPinOutput(pin);
				// Do not pre-set brightness here — wait for loop()
			}
		}

		// WS2812 LED
		LED(unsigned int address, unsigned int mask, WS2812* strip, uint8_t pixelIndex, uint32_t rgb, uint8_t brightness = 255)
			: Int16Buffer(address), mask(mask), reverse(false),
			  useExpander(false), gpioPin(0),
			  expander(nullptr), expanderPin(0),
			  useWs2812(true), ws2812Pixel(pixelIndex),
			  ws2812Strip(strip), rgbColor(rgb), brightness_(brightness) {}

		virtual void loop() {
			if (!hasUpdatedData()) return;
			bool state = getData() & mask;
			if (reverse) state = !state;

			if (useWs2812 && ws2812Strip) {
				uint8_t r = (rgbColor >> 8) & 0xFF;
				uint8_t g = (rgbColor >> 16) & 0xFF;
				uint8_t b = rgbColor & 0xFF;

				r = (r * brightness_) / 255;
				g = (g * brightness_) / 255;
				b = (b * brightness_) / 255;

				ws2812Strip->setPixel(ws2812Pixel, state ? ws2812Strip->rgb(r, g, b) : 0);
				ws2812Strip->show();
			}
			else if (useExpander && expander) {
				expander->writePin(expanderPin, state);
				expander->setLEDBrightness(expanderPin, state ? brightness_ : 0);
			}
			else {4
				gpio_put(gpioPin, state);
			}
		}
	};

}

#endif
