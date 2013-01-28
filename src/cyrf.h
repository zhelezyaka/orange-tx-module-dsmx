
#define CYRF_RESET	0x01
#define CYRF_IRQ	0x02
#define CYRF_MISO	0x40
#define CYRF_MOSI	0x20
#define CYRF_SCK	0x80
#define CYRF_SS		0x10

void CYRF_init(char bind);
void CYRF_write(unsigned char reg, unsigned char value);
unsigned char CYRF_read(unsigned char reg);
void CYRF_read_block(unsigned char reg, unsigned char *pbStrPtr, unsigned char len);
void CYRF_write_block(unsigned char reg, unsigned char *pbStrPtr, unsigned char len);
void CYRF_write_block_const(unsigned char reg, const unsigned char *pbStrPtr, unsigned char len);
void CYRF_read_mnfctID(void);