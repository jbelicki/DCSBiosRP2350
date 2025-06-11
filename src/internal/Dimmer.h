#ifndef __DCSBIOS_DIMMER_H
#define __DCSBIOS_DIMMER_H

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "ExportStreamListener.h"

namespace DcsBios {

	class Dimmer : public Int16Buffer {
	private:
		uint pin_;
		const char* msg_;
		int minOutput_;
		int maxOutput_;
		unsigned int (*map_function_)(unsigned int newValue);
		uint slice_num_;

		unsigned int map(unsigned int x, unsigned int in_min, unsigned int in_max, unsigned int out_min, unsigned int out_max) {
			return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
		}

		void init_pwm() {
			gpio_set_function(pin_, GPIO_FUNC_PWM);
			slice_num_ = pwm_gpio_to_slice_num(pin_);
			pwm_set_wrap(slice_num_, 65535); // Full 16-bit range
			pwm_set_enabled(slice_num_, true);
		}

	public:
		Dimmer(unsigned int address, uint pin, int minOutput = 0, int maxOutput = 65535) : Int16Buffer(address) {
			pin_ = pin;
			minOutput_ = minOutput;
			maxOutput_ = maxOutput;
			map_function_ = NULL;
			init_pwm();
		}

		Dimmer(unsigned int address, uint pin, unsigned int (*map_function)(unsigned int newValue)) : Int16Buffer(address) {
			pin_ = pin;
			map_function_ = map_function;
			minOutput_ = 0;
			maxOutput_ = 65535;
			init_pwm();
		}

		virtual void loop() {
			if (hasUpdatedData()) {
				pwm_set_gpio_level(pin_, mapValue(getData()));
			}
		}

		unsigned int mapValue(unsigned int value) {
			if (map_function_) {
				return map_function_(value);
			} else {
				return map(value, 0, 65535, minOutput_, maxOutput_);
			}
		}

		void SetControl(const char* msg) {
			msg_ = msg;
		}
	};

}

#endif
