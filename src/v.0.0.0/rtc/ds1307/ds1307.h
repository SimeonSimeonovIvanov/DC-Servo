#define DS1307_ID    0xD0        // I2C DS1307 Device Identifier
#define DS1307_ADDR  0x00        // I2C DS1307 Device Address

#define HOUR_24 0
#define HOUR_12 1

char dec2bcd(char num);
char bcd2dec(char num);
void Read_DS1307(void);
void Write_DS1307(void);
