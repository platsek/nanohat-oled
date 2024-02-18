#pragma once

#define I2C_BUFFER_SIZE 256

/*
 * Helper functions.
 */
static int i2c_get_fd(char *dev, int *fd);
static int i2c_release_fd(int fd);

static int i2c_set_device(int fd, int i2c_addr);
static int i2c_set_device_10bit(int fd, int i2c_addr);

/*
 * I2C functions.
 */
int i2c_write(char *dev, int i2c_addr, unsigned char *buf, int count);
int i2c_writen_reg(char *dev, int i2c_addr, int reg, unsigned char *buf,
		   int count);
int i2c_write_reg(char *dev, int i2c_addr, int reg, int value);

int i2c_read(char *dev, int i2c_addr, unsigned char *buf, int count);
int i2c_readn_reg(char *dev, int i2c_addr, int reg, unsigned char *buf,
		  int count);
int i2c_read_reg(char *dev, int i2c_addr, int reg, int *value);

int i2c_mask_reg(char *dev, int i2c_addr, int reg, int mask);
