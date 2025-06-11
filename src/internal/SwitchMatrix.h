#ifndef __DCSBIOS_SWITCHMATRIX_H
#define __DCSBIOS_SWITCHMATRIX_H

#include "pico/stdlib.h"
#include <vector>

class SwitchMatrix {
private:
	std::vector<uint> rowPins;
	std::vector<uint> colPins;

public:
	SwitchMatrix() {}

	// Call once to define the matrix
	void begin(const std::vector<uint>& rows, const std::vector<uint>& cols) {
		rowPins = rows;
		colPins = cols;

		// Initialize rows as outputs (default HIGH)
		for (auto pin : rowPins) {
			gpio_init(pin);
			gpio_set_dir(pin, GPIO_OUT);
			gpio_put(pin, 1);
		}

		// Initialize cols as inputs with pull-up
		for (auto pin : colPins) {
			gpio_init(pin);
			gpio_set_dir(pin, GPIO_IN);
			gpio_pull_up(pin);
		}
	}

	// Returns true if switch at [row, col] is pressed
	bool GetSwitchState(uint rowIdx, uint colIdx) {
		if (rowIdx >= rowPins.size() || colIdx >= colPins.size()) return false;

		uint rowPin = rowPins[rowIdx];
		uint colPin = colPins[colIdx];

		// Set all rows high, then pull current row low
		for (auto pin : rowPins) gpio_put(pin, 1);
		gpio_put(rowPin, 0);

		// Small delay to let signal settle
		sleep_us(10);

		// Read column state
		bool state = gpio_get(colPin) == 0;

		// Reset row to HIGH after scan
		gpio_put(rowPin, 1);
		return state;
	}

	size_t numRows() const { return rowPins.size(); }
	size_t numCols() const { return colPins.size(); }
};

#endif
