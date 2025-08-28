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

		bool getState() const {
			return lastState_ != 0;
		}
	};
	typedef Switch2PosT<> Switch2Pos;

	// --- SwitchWithCover2PosT and SwitchMultiPosT remain unchanged below ---

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


	template <unsigned long pollIntervalMs = POLL_EVERY_TIME>
	class Switch3Pos2PinT : PollingInput, public ResettableInput {
	private:
		const char* msg_;
		bool useExpander_;
		AW9523B* expander_;
		uint8_t pins_[2];                 // pins_[0] = A, pins_[1] = B

		int8_t lastState_;                // -1 unknown, else 0/1/2
		int8_t debounceCandidate_;        // candidate logical state awaiting debounce
		unsigned long debounceDelay_;     // like other classes
		unsigned long lastDebounceTime_;  // when candidate was (re)observed

		// Center watchdog (forces center after stable HIGH/HIGH for debounceDelay_)
		unsigned long centerStableSince_;

		// Active-LOW read (external pull-ups)
		inline char readInput(uint8_t idx) {
			if (useExpander_ && expander_) return expander_->readPin(pins_[idx]);
			return gpio_get(pins_[idx]);
		}

		// Map raw A/B to logical position; return -1 if invalid (both active)
		inline int8_t mapLogical(bool aActive, bool bActive) {
			if (aActive && bActive)   return -1; // transient during travel: ignore, hold last
			if (aActive && !bActive)  return 0;  // A thrown
			if (!aActive && !bActive) return 1;  // center
			/* !aActive && bActive */ return 2;  // B thrown
		}

		void resetState() {
			lastState_ = -1;
			debounceCandidate_ = -1;
			lastDebounceTime_ = 0;
			centerStableSince_ = 0;
		}

		void pollInput() {
			const unsigned long now = to_ms_since_boot(get_absolute_time());

			// Read raw pins (active when LOW)
			const bool aActive = (readInput(0) == 0);
			const bool bActive = (readInput(1) == 0);

			// Center watchdog: if both HIGH and stable for debounceDelay_, force center=1
			const bool isCenterPhysical = (!aActive && !bActive);
			if (isCenterPhysical) {
				if (centerStableSince_ == 0) centerStableSince_ = now;
				if ((now - centerStableSince_) >= debounceDelay_) {
					if (lastState_ != 1) {
						char msgBuffer[3];
						snprintf(msgBuffer, sizeof(msgBuffer), "%d", 1);
						if (tryToSendDcsBiosMessage(msg_, msgBuffer)) {
							lastState_ = 1;
						}
					}
					// Align logical debounce state with enforced center and exit
					debounceCandidate_ = 1;
					lastDebounceTime_  = now;
					return;
				}
			} else {
				centerStableSince_ = 0; // reset when not physically centered
			}

			// Determine current logical candidate
			const int8_t logical = mapLogical(aActive, bActive);

			// If invalid combo (both active), do not change candidate; just hold last.
			if (logical < 0) return;

			// Debounce the logical state (like your other classes)
			if (logical != debounceCandidate_) {
				debounceCandidate_ = logical;
				lastDebounceTime_  = now;
			}

			if ((now - lastDebounceTime_) >= debounceDelay_) {
				if (debounceCandidate_ != lastState_) {
					char msgBuffer[3];
					snprintf(msgBuffer, sizeof(msgBuffer), "%d", debounceCandidate_);
					if (tryToSendDcsBiosMessage(msg_, msgBuffer)) {
						lastState_ = debounceCandidate_;
					}
				}
			}
		}

	public:
		// GPIO constructor (two discrete pins)
		Switch3Pos2PinT(const char* msg, uint8_t pinA, uint8_t pinB, unsigned long debounceDelay = 50) :
			PollingInput(pollIntervalMs),
			msg_(msg),
			useExpander_(false),
			expander_(nullptr),
			lastState_(-1),
			debounceCandidate_(-1),
			debounceDelay_(debounceDelay),
			lastDebounceTime_(0),
			centerStableSince_(0)
		{
			pins_[0] = pinA; pins_[1] = pinB;
			gpio_init(pins_[0]); gpio_pull_up(pins_[0]); gpio_set_dir(pins_[0], GPIO_IN);
			gpio_init(pins_[1]); gpio_pull_up(pins_[1]); gpio_set_dir(pins_[1], GPIO_IN);
			resetState();
		}

		// AW9523B constructor (two pins on expander)
		Switch3Pos2PinT(const char* msg, AW9523B* expander, uint8_t pinA, uint8_t pinB, unsigned long debounceDelay = 50) :
			PollingInput(pollIntervalMs),
			msg_(msg),
			useExpander_(true),
			expander_(expander),
			lastState_(-1),
			debounceCandidate_(-1),
			debounceDelay_(debounceDelay),
			lastDebounceTime_(0),
			centerStableSince_(0)
		{
			pins_[0] = pinA; pins_[1] = pinB;
			expander_->setPinInput(pins_[0]); // no internal pulls; external pull-ups present
			expander_->setPinInput(pins_[1]);
			resetState();
		}

		void SetControl(const char* msg) { msg_ = msg; }
		void resetThisState() { resetState(); }
	};
	typedef Switch3Pos2PinT<> Switch3Pos2Pin;
} // namespace DcsBios

#endif
