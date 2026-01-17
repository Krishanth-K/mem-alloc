BUILD_DIR := build
TARGET    := $(BUILD_DIR)/mem_alloc
BENCHMARK := $(BUILD_DIR)/benchmark

.PHONY: all configure build run clean rebuild bench

all: run

configure:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake ..

build: configure
	cd $(BUILD_DIR) && cmake --build .

run: build
	clear && $(TARGET)

runnoc: build
	$(TARGET)

# Benchmark target - builds and runs benchmark separately
bench:
	@echo "Building benchmark..."
	@mkdir -p $(BUILD_DIR)
	cc -O2 -Iinclude src/benchmark.c src/mem.c -o $(BENCHMARK) -lm
	@echo "Running benchmark..."
	@clear
	@$(BENCHMARK)

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean all
