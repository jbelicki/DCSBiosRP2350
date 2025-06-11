#ifndef __DCSBIOS_SWITCHES_H
#define __DCSBIOS_SWITCHES_H

#include <math.h>
#include "pico/stdlib.h"
#include "aw9523b.h"

namespace DcsBios {

	template <unsigned long pollIntervalMs = POLL_EVERY_TIME>
	class Switch2PosT : PollingInput, public ResettableInput {
	private:
		const char* msg_;
		char pin_;
		bool useExpander_;
		AW9523B* expander_;
		uint8_t expanderPin_;
		char debounceSteadyState_;
		char lastState_;
		bool reverse_;
		unsigned long debounceDelay_;
		unsigned long lastDebounceTime = 0;

		char readInput() {
			if (useExpander_ && expander_) return expander_->readPin(expanderPin_);
			return gpio_get(pin_);
		}

		void resetState() {
			lastState_ = (lastState_ == 0) ? -1 : 0;
		}

		void pollInput() {
			char state = readInput();
			if (reverse_) state = !state;
			unsigned long now = to_ms_since_boot(get_absolute_time());
			if (state != debounceSteadyState_) {
				lastDebounceTime = now;
				debounceSteadyState_ = state;
			}
			if ((now - lastDebounceTime) >= debounceDelay_) {
				if (debounceSteadyState_ != lastState_) {
					if (tryToSendDcsBiosMessage(msg_, state == 1 ? "0" : "1")) {
						lastState_ = debounceSteadyState_;
					}
				}
			}
		}

	public:
		Switch2PosT(const char* msg, char pin, bool reverse = false, unsigned long debounceDelay = 50) :
			PollingInput(pollIntervalMs), useExpander_(false)
		{
			msg_ = msg;
			pin_ = pin;
			gpio_init(pin_);
			gpio_pull_up(pin_);
			gpio_set_dir(pin_, GPIO_IN);
			debounceDelay_ = debounceDelay;
			reverse_ = reverse;
			lastState_ = gpio_get(pin_);
			if (reverse_) lastState_ = !lastState_;
		}

		Switch2PosT(const char* msg, AW9523B* expander, uint8_t pin, bool reverse = false, unsigned long debounceDelay = 50) :
			PollingInput(pollIntervalMs), useExpander_(true), expander_(expander), expanderPin_(pin)
		{
			msg_ = msg;
			debounceDelay_ = debounceDelay;
			reverse_ = reverse;
			expander_->setPinInput(expanderPin_);
			lastState_ = readInput();
			if (reverse_) lastState_ = !lastState_;
		}

		void SetControl(const char* msg) { msg_ = msg; }
		void resetThisState() { this->resetState(); }
	};
	typedef Switch2PosT<> Switch2Pos;

	template <unsigned long pollIntervalMs = POLL_EVERY_TIME, unsigned long coverDelayMs = 200>
	class SwitchWithCover2PosT : PollingInput, public ResettableInput {
	private:
		const char* switchMsg_;
		const char* coverMsg_;
		char pin_;
		bool useExpander_;
		AW9523B* expander_;
		uint8_t expanderPin_;
		char lastState_;
		char switchState_;
		bool reverse_;
		unsigned long debounceDelay_;
		unsigned long lastDebounceTime = 0;

		enum switchCoverStateEnum { stOFF_CLOSED = 0, stOFF_OPEN = 1, stON_OPEN = 2 };
		switchCoverStateEnum switchCoverState_;
		unsigned long lastSwitchStateTime;

		char readInput() {
			if (useExpander_ && expander_) return expander_->readPin(expanderPin_);
			return gpio_get(pin_);
		}

		void resetState() {
			lastState_ = (lastState_ == 0) ? -1 : 0;
			if (switchState_ && !reverse_) switchCoverState_ = stOFF_CLOSED;
			else switchCoverState_ = stON_OPEN;
			lastSwitchStateTime = to_ms_since_boot(get_absolute_time());
		}

		void pollInput() {
			char state = readInput();
			if (reverse_) state = !state;
			if (state != lastState_) lastDebounceTime = to_ms_since_boot(get_absolute_time());
			if (state != switchState_ &&
				(to_ms_since_boot(get_absolute_time()) - lastDebounceTime) > debounceDelay_) {
				if (to_ms_since_boot(get_absolute_time()) - lastSwitchStateTime > coverDelayMs) {
					if (state) {
						if (switchCoverState_ == stON_OPEN && tryToSendDcsBiosMessage(switchMsg_, "0")) {
							switchCoverState_ = stOFF_OPEN;
							lastSwitchStateTime = to_ms_since_boot(get_absolute_time());
						} else if (switchCoverState_ == stOFF_OPEN && tryToSendDcsBiosMessage(coverMsg_, "0")) {
							switchCoverState_ = stOFF_CLOSED;
							lastSwitchStateTime = to_ms_since_boot(get_absolute_time());
							switchState_ = state;
						}
					} else {
						if (switchCoverState_ == stOFF_CLOSED && tryToSendDcsBiosMessage(coverMsg_, "1")) {
							switchCoverState_ = stOFF_OPEN;
							lastSwitchStateTime = to_ms_since_boot(get_absolute_time());
						} else if (switchCoverState_ == stOFF_OPEN && tryToSendDcsBiosMessage(switchMsg_, "1")) {
							switchCoverState_ = stON_OPEN;
							lastSwitchStateTime = to_ms_since_boot(get_absolute_time());
							switchState_ = state;
						}
					}
				}
			}
			lastState_ = state;
		}

	public:
		SwitchWithCover2PosT(const char* switchMsg, const char* coverMsg, char pin, bool reverse = false, unsigned long debounceDelay = 50) :
			PollingInput(pollIntervalMs), useExpander_(false)
		{
			switchMsg_ = switchMsg;
			coverMsg_ = coverMsg;
			pin_ = pin;
			gpio_init(pin_);
			gpio_pull_up(pin_);
			gpio_set_dir(pin_, GPIO_IN);
			switchState_ = gpio_get(pin_);
			lastState_ = switchState_;
			reverse_ = reverse;
			debounceDelay_ = debounceDelay;
			resetState();
		}

		SwitchWithCover2PosT(const char* switchMsg, const char* coverMsg, AW9523B* expander, uint8_t pin, bool reverse = false, unsigned long debounceDelay = 50) :
			PollingInput(pollIntervalMs), useExpander_(true), expander_(expander), expanderPin_(pin)
		{
			switchMsg_ = switchMsg;
			coverMsg_ = coverMsg;
			reverse_ = reverse;
			expander_->setPinInput(expanderPin_);
			debounceDelay_ = debounceDelay;
			switchState_ = readInput();
			lastState_ = switchState_;
			resetState();
		}

		void resetThisState() { this->resetState(); }
	};
	typedef SwitchWithCover2PosT<> SwitchWithCover2Pos;

	template <unsigned long pollIntervalMs = POLL_EVERY_TIME, int numPositions = 3>
	class SwitchMultiPosT : PollingInput, public ResettableInput {
	private:
		const char* msg_;
		const uint8_t* pins_;
		bool useExpander_;
		AW9523B* expander_;
		uint8_t expanderPins_[5];
		char lastState_;
		unsigned long debounceDelay_;
		unsigned long lastDebounceTime_ = 0;
	
		char readInput(uint8_t index) {
			if (useExpander_ && expander_)
				return expander_->readPin(expanderPins_[index]);
			return gpio_get(pins_[index]);
		}
	
		void resetState() {
			lastState_ = -1;
		}
	
		void pollInput() {
			unsigned long now = to_ms_since_boot(get_absolute_time());
			for (int i = 0; i < numPositions; ++i) {
				char state = readInput(i);
				if (state == 0) {  // active-low
					if (i != lastState_ && (now - lastDebounceTime_) >= debounceDelay_) {
						char msgBuffer[3];
						snprintf(msgBuffer, sizeof(msgBuffer), "%d", i);
						if (tryToSendDcsBiosMessage(msg_, msgBuffer)) {
							lastState_ = i;
							lastDebounceTime_ = now;
						}
					}
					break;
				}
			}
		}
	
	public:
		SwitchMultiPosT(const char* msg, const uint8_t* pins, unsigned long debounceDelay = 50) :
			PollingInput(pollIntervalMs), msg_(msg), pins_(pins), useExpander_(false), debounceDelay_(debounceDelay) {
			for (int i = 0; i < numPositions; ++i) {
				gpio_init(pins[i]);
				gpio_pull_up(pins[i]);
				gpio_set_dir(pins[i], GPIO_IN);
			}
			resetState();
		}
	
		SwitchMultiPosT(const char* msg, AW9523B* expander, const uint8_t* pins, unsigned long debounceDelay = 50) :
			PollingInput(pollIntervalMs), msg_(msg), useExpander_(true), expander_(expander), debounceDelay_(debounceDelay) {
			for (int i = 0; i < numPositions; ++i) {
				expanderPins_[i] = pins[i];
				expander_->setPinInput(pins[i]);
			}
			resetState();
		}
	
		void SetControl(const char* msg) { msg_ = msg; }
		void resetThisState() { resetState(); }
	};
	

	typedef SwitchMultiPosT<POLL_EVERY_TIME, 3> Switch3Pos;
	typedef SwitchMultiPosT<POLL_EVERY_TIME, 4> Switch4Pos;
	typedef SwitchMultiPosT<POLL_EVERY_TIME, 5> Switch5Pos;

} // namespace DcsBios

#endif
