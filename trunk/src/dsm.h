
#define ORTX_USE_DSMX	0x01 //- dsmX flag, else dsm2 use
#define ORTX_USE_11mS	0x02 //- 11 mSec flag, else 22 mSec
#define ORTX_USE_11bit	0x04 //- 11 bit (2048) flag, else 10 bit (1024) resolution
#define ORTX_USE_TM		0x08 //- have a telemetry answer
#define ORTX_BIND_FLAG	0x80

void generateDSMXchannel_list(void);
unsigned char generateBINDchannel(void);
void generateDSM2channel(void);
unsigned char BIND_procedure(void);
unsigned char transmit_receive(unsigned char channel, unsigned char control);
void buildTransmitBuffer(unsigned char top);