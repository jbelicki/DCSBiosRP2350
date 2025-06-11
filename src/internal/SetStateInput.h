#ifndef __DCSBIOS_SET_STATE_INPUT_H
#define __DCSBIOS_SET_STATE_INPUT_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "PollingInput.h"
#include "ExportStreamListener.h"

namespace DcsBios {

class SetStateInput : public PollingInput {
private:
	unsigned int (*readState_)();
	const char* msg_;
	unsigned int lastState_;
	bool isFirstPoll_;

public:
	SetStateInput(const char* msg, unsigned int (*readState)())
		: PollingInput(POLL_EVERY_TIME),
		  readState_(readState),
		  msg_(msg),
		  isFirstPoll_(true) {}

	void pollInput() override {
		if (isFirstPoll_) {
			lastState_ = readState_();
			isFirstPoll_ = false;
			return;
		}

		unsigned int state = readState_();
		if (state != lastState_) {
			char buf[12];
			snprintf(buf, sizeof(buf), "%u", state);
			if (tryToSendDcsBiosMessage(msg_, buf))
				lastState_ = state;
		}
	}
};

} // namespace DcsBios

#endif
