#include <avr/io.h>
#include <compat/twi.h>
#include <avr/interrupt.h>

#define MAX_TRIES 50

#define DS1307_ID    0xD0        // I2C DS1307 Device Identifier
#define DS1307_ADDR  0x00        // I2C DS1307 Device Address

#define I2C_START 0
#define I2C_DATA 1
#define I2C_DATA_ACK 2
#define I2C_STOP 3
#define ACK 1
#define NACK 0

void i2c_stop(void);
char i2c_write(char data);
char i2c_read(char *data,char ack_type);
unsigned char i2c_transmit(unsigned char type);
char i2c_start(unsigned int dev_id, unsigned int dev_addr, unsigned char rw_type);
