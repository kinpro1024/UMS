#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <thread>
#include <math.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <memory>

#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"
#include "thermal_wrapper.hpp"


int Mlx90640::sensorInit(uint8_t refreshRateSet) {
	MLX90640_I2CInit();
	if (!MLX90640_DumpEE(MLX_ADDR, ee)) {
		if (!MLX90640_ExtractParameters(ee, &mlx_params)) {
			if (!MLX90640_SetRefreshRate(MLX_ADDR, refreshRateSet)) {
				if (!MLX90640_SetChessMode(MLX_ADDR)) {
					return 0;
				}
			}
		}
	}
	return 1;
}

int Mlx90640::dataReady(void) {

	uint16_t status;
	if (MLX90640_I2CRead(MLX_ADDR, MLX90640_STATUS_REG, 1, &status) != MLX90640_NO_ERROR) {
		return false;
	}
	return (status & MLX90640_STAT_DATA_READY_MASK) != 0;
}

std::unique_ptr<ThermalWrapperFrame> Mlx90640::requestFullFrame(int timeout_ms) {

	TimeType request_start = std::chrono::steady_clock::now();

	std::unique_ptr<ThermalWrapperFrame> final_frame =  std::make_unique<ThermalWrapperFrame>();

	subpage_received[0] = false;
	subpage_received[1] = false;

	while (!(subpage_received[0] && subpage_received[1])) {

		TimeType subpage_loop_flag = std::chrono::steady_clock::now();

		int elapsed_wait_time = std::chrono::duration_cast<std::chrono::milliseconds>(subpage_loop_flag - request_start).count();

		if(elapsed_wait_time > timeout_ms) {
			return nullptr;
		}

		if (!dataReady()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
			continue;
		}

		int status = MLX90640_GetFrameData(MLX_ADDR, mlx_subpage_buffer);

		if (status < 0) {
			continue;
		}

		int subpage = MLX90640_GetSubPageNumber(mlx_subpage_buffer);
		if (subpage == 0) {
			memcpy(mlx_subpage_0, mlx_subpage_buffer, 834*sizeof(uint16_t));
		} else if (subpage == 1) {
			memcpy(mlx_subpage_1, mlx_subpage_buffer, 834*sizeof(uint16_t));
		} else {
			return nullptr;
		}
		subpage_received[subpage] = true;
	}

	final_frame->timestamp = std::chrono::steady_clock::now();

	float ta = MLX90640_GetTa(mlx_subpage_0, &mlx_params);
	float tr = ta - 8.0f;

	MLX90640_CalculateTo( mlx_subpage_0, &mlx_params, 0.95f, tr, final_frame->temperatures.data());
	MLX90640_CalculateTo( mlx_subpage_1, &mlx_params, 0.95f, tr, final_frame->temperatures.data());

	return final_frame;
}
