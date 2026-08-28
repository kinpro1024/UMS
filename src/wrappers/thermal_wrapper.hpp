#pragma once

#include <vector>
#include <chrono>
#include <memory>

#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"

#define MLX_ADDR 0x33

typedef std::chrono::steady_clock::time_point TimeType;

class ThermalFrame {
	public:
	std::vector<float> temperatures;
	TimeType timestamp;

	ThermalFrame()
	:  temperatures(768)
	{}
};

class Mlx90640 {
private:
	uint16_t ee[832];
	paramsMLX90640 mlx_params;
	uint16_t mlx_subpage_buffer[834];
	uint16_t mlx_subpage_0[834];
	uint16_t mlx_subpage_1[834];
	bool subpage_received[2];

public:
	int sensorInit(uint8_t refreshRateSet);
	int dataReady(void);
	std::unique_ptr<ThermalFrame> requestFullFrame(int timeout_ms);	
};