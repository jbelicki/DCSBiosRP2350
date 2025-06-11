#ifndef __DCSBIOS_POTS_H
#define __DCSBIOS_POTS_H

#include <math.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

namespace DcsBios {

	template <unsigned long pollIntervalMs = POLL_EVERY_TIME, unsigned int hysteresis = 128, unsigned int ewma_divisor = 5>
	class PotentiometerEWMA : PollingInput, public ResettableInput {
	private:
		void resetState() {
			lastState_ = (lastState_==0)?-1:0;
		}

		void pollInput() {
			uint rawValue;

			adc_select_input(adc_channel_);
			rawValue = adc_read();

			unsigned int state;
			if (reverse_)
				state = mapInt(rawValue, input_min_, input_max_, 65535, 0);
			else
				state = mapInt(rawValue, input_min_, input_max_, 0, 65535);

			accumulator += ((float)state - accumulator) / (float)ewma_divisor;
			state = (unsigned int)accumulator;

			if (((lastState_ > state && (lastState_ - state > hysteresis)))
				|| ((state > lastState_) && (state - lastState_ > hysteresis))
				|| ((state > (65535 - hysteresis) && state > lastState_))
				|| ((state < hysteresis && state < lastState_))
			) {
				char buf[6];
				utoa(state, buf, 10);
				if (tryToSendDcsBiosMessage(msg_, buf))
					lastState_ = state;
			}
		}

		const char* msg_;
		char gpio_pin_;
		uint8_t adc_channel_;
		unsigned int lastState_;
		float accumulator;
		bool reverse_;
		unsigned int input_min_;
		unsigned int input_max_;

	public:
		PotentiometerEWMA(const char* msg, char gpio_pin, bool reverse = false, unsigned int input_min = 0, unsigned int input_max = 4095)
			: PollingInput(pollIntervalMs) {
			msg_ = msg;
			gpio_pin_ = gpio_pin;
			reverse_ = reverse;
			input_min_ = input_min;
			input_max_ = input_max;

			adc_init();

			switch (gpio_pin) {
				case 26: adc_channel_ = 0; break;
				case 27: adc_channel_ = 1; break;
				case 28: adc_channel_ = 2; break;
				default: adc_channel_ = 0; break; // fallback to channel 0
			}

			adc_gpio_init(gpio_pin_);

			adc_select_input(adc_channel_);
			uint raw = adc_read();

			if (reverse_)
				lastState_ = mapInt(raw, input_min_, input_max_, 65535, 0);
			else
				lastState_ = mapInt(raw, input_min_, input_max_, 0, 65535);

			accumulator = lastState_;
		}

		void SetControl(const char* msg) {
			msg_ = msg;
		}

		void resetThisState() {
			this->resetState();
		}
	};

	typedef PotentiometerEWMA<> Potentiometer;
}

#endif
