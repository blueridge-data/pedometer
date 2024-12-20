# Get the present working directory
PWD_DIR := $(shell pwd)

# Define variables
BOARD=teensy:avr:teensy40
BUILD_PATH=$(PWD_DIR)/build
PORT=$(shell arduino-cli board list | grep -i "teensy" | awk '{print $$1}')

# Declare phony targets
.PHONY: all build flash clean

# Default target: compile and upload
all: build

# Step 1: Compile the sketch
build:
	@echo "Compiling the sketch..."

# use this command if you want to control build path:
# arduino-cli compile --fqbn=$(BOARD) --build-path ./build

# otherwise, let arduino-cli use the default build path:
	arduino-cli compile --fqbn=$(BOARD) $(PWD_DIR)

	@echo "Done."

# Step 2: Upload the sketch to the Teensy 4.0
flash:
	@if [ -z "$(PORT)" ]; then \
		echo "Teensy 4.0 not found. Please connect your Teensy and try again."; \
		exit 1; \
	fi
	@echo "Uploading the sketch to Teensy 4.0 on port $(PORT)..."

# use this command for custom build path:
#@/Users/ryanwalsh/Library/Arduino15/packages/teensy/tools/teensy-tools/1.59.0/teensy_post_compile -file=pedometer.ino -path=$(BUILD_PATH) -tools=/Users/ryanwalsh/Library/Arduino15/packages/teensy/tools/teensy-tools/1.59.0 -board=TEENSY40 -reboot -port=$(PORT) -portlabel={serial.port.label} -portprotocol={serial.port.protocol}

# otherwise, use custom build path:
	@arduino-cli upload -p $(PORT) --fqbn teensy:avr:teensy40 $(PWD_DIR)

	@echo "Done."

# Step 3: Clean up temporary files (optional)
clean:
	@echo "Cleaning up temporary files..."

# only use this for custom build path:
#@rm -r $(BUILD_PATH)

	@echo "Done."
