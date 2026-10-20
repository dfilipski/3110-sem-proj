CXX = clang++
CPPFLAGS = -Isrc
CXXFLAGS = -std=c++17 -Wall -Wextra -g

BUILD_DIR = build
TARGET = $(BUILD_DIR)/main
MODEL_SOURCES = src/Movie.cpp src/Rating.cpp src/User.cpp src/MovieDatabase.cpp src/MovieDatabaseMap.cpp src/MovieDatabaseTreap.cpp
SOURCES = src/main.cpp $(MODEL_SOURCES)
OBJECTS = $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
TEST_OBJECT = $(BUILD_DIR)/models_test.o
TEST_TARGET = $(BUILD_DIR)/models_test
BENCHMARK_DIR = $(BUILD_DIR)/benchmark_objects
BENCHMARK_TARGET = $(BUILD_DIR)/benchmark
BENCHMARK_CXXFLAGS = -std=c++17 -Wall -Wextra -O2
BENCHMARK_OBJECTS = $(patsubst src/%.cpp,$(BENCHMARK_DIR)/%.o,src/benchmark.cpp $(MODEL_SOURCES))
DEPS = $(BENCHMARK_OBJECTS:.o=.d) $(OBJECTS:.o=.d) $(TEST_OBJECT:.o=.d)

.PHONY: all clean run test benchmark

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(TEST_OBJECT): tests/models_test.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(TEST_TARGET): $(TEST_OBJECT) $(filter-out $(BUILD_DIR)/main.o,$(OBJECTS))
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

-include $(DEPS)

# Separate optimized objects keep benchmark timings independent of debug builds.
$(BENCHMARK_DIR): | $(BUILD_DIR)
	mkdir -p $@

$(BENCHMARK_DIR)/%.o: src/%.cpp | $(BENCHMARK_DIR)
	$(CXX) $(CPPFLAGS) $(BENCHMARK_CXXFLAGS) -MMD -MP -c $< -o $@

$(BENCHMARK_TARGET): $(BENCHMARK_OBJECTS)
	$(CXX) $(BENCHMARK_CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

benchmark: $(BENCHMARK_TARGET)
	./$(BENCHMARK_TARGET)
