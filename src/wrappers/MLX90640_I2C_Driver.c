#include "MLX90640_I2C_Driver.h"

#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <linux/i2c-dev.h>
#include <linux/i2c.h>

#define I2C_BUS "/dev/i2c-1"
static int i2c_fd = -1;

void MLX90640_I2CInit(void) {
    if(i2c_fd >= 0) {
        return;
    }

    i2c_fd = open(I2C_BUS, O_RDWR);
}

int MLX90640_I2CRead(uint8_t slaveAddr, uint16_t startAddress, uint16_t nMemAddressRead, uint16_t* data) {
    if (i2c_fd < 0) {
        return -1;
    }

    uint8_t address[2];

    address[0] = (uint8_t)(startAddress >> 8);
    address[1] = (uint8_t)(startAddress & 0xFF);

    uint8_t buffer[nMemAddressRead * 2];

    struct i2c_msg messages[2];

    messages[0].addr = slaveAddr;
    messages[0].flags = 0;
    messages[0].len = 2;
    messages[0].buf = address;

    messages[1].addr = slaveAddr;
    messages[1].flags = I2C_M_RD;
    messages[1].len = nMemAddressRead * 2;
    messages[1].buf = buffer;

    struct i2c_rdwr_ioctl_data message_set;
    message_set.msgs = messages;
    message_set.nmsgs = 2;

    if (ioctl(i2c_fd, I2C_RDWR, &message_set) < 0) {
        return -1;
    }
    
    for (uint16_t i = 0; i < nMemAddressRead; ++i) {
        data[i] = ((uint16_t)buffer[2*i] << 8) | ((uint16_t)buffer[(2*i)+1]);
    }

    return 0;
}

int MLX90640_I2CWrite(uint8_t slaveAddr, uint16_t writeAddress, uint16_t data) {
    if (i2c_fd < 0) {
        return -1;
    }
    
    uint8_t buffer[4];

    buffer[0] = (uint8_t)(writeAddress >> 8);
    buffer[1] = (uint8_t)(writeAddress & 0xFF);

    buffer[2] = (uint8_t)(data >> 8);
    buffer[3] = (uint8_t)(data & 0xFF);

    struct i2c_msg message;

    message.addr = slaveAddr;
    message.flags = 0;
    message.len = 4;
    message.buf = buffer;

    struct i2c_rdwr_ioctl_data message_set;
    message_set.msgs = &message;
    message_set.nmsgs = 1;

    if (ioctl(i2c_fd, I2C_RDWR, &message_set) < 0) {
        return -1;
    }

    return 0;
}

void MLX90640_I2CFreqSet(int freq) {
    (void)freq;
}

int MLX90640_I2CGeneralReset(void) {
    return 0;
}