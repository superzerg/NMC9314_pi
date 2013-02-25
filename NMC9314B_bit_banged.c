#include <bcm2835.h>
#include <stdio.h>
#include <time.h>
#include <getopt.h>
#include <string.h>

//Compile with :
// gcc -o NMC9314B_bit_banged -l rt NMC9314B_bit_banged.c -l bcm2835 -O3

#define MOSI RPI_V2_GPIO_P1_19
#define MISO RPI_V2_GPIO_P1_21 
#define CLK RPI_V2_GPIO_P1_23
#define CE2 RPI_V2_GPIO_P1_24
#define CE1 RPI_V2_GPIO_P1_26

#define HALF_T_CLK 10//half clock period in ms (fastest=0.001)

static int verbose_flag=1;

void wait_half_clock()
{
	if(HALF_T_CLK >= 0.001 && HALF_T_CLK < 1)
		bcm2835_delayMicroseconds(HALF_T_CLK*1000);
	if(HALF_T_CLK >= 1)
		bcm2835_delay(HALF_T_CLK);
}

void init(void)
{
	if (!bcm2835_init())
		return;

	// Set the input/output pins
	bcm2835_gpio_fsel(MISO, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_set_pud(MISO, BCM2835_GPIO_PUD_UP);
	bcm2835_gpio_fsel(MOSI, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(CLK, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(CE2, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(CE1, BCM2835_GPIO_FSEL_OUTP);
	//make colck and MOSI start from 0
	bcm2835_gpio_write(CLK, LOW);
	bcm2835_gpio_write(MOSI, LOW);

	//Set CS to 0 (CS=1 to activate NMC9314)
	bcm2835_gpio_write(CE2, LOW);
	bcm2835_gpio_write(CE1, LOW);

	//1 clock period
	bcm2835_gpio_write(CLK, HIGH);
	wait_half_clock();
	bcm2835_gpio_write(CLK, LOW);
}
void reset(void)
{
	// reset the input/output pins
	bcm2835_gpio_set_pud(MISO, BCM2835_GPIO_PUD_DOWN);
	bcm2835_gpio_fsel(MISO, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(MOSI, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(CLK, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(CE2, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(CE1, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_close();
}

void send8bit(uint8_t data)
{
	//MOSI LOW by default
	bcm2835_gpio_write(MOSI, LOW);
	//Set CS to 1 
	bcm2835_gpio_write(CE1, HIGH);
	//wait for 2 us (spec is >0.4)
	bcm2835_delayMicroseconds(2);
	//1 clock period
	bcm2835_gpio_write(CLK, HIGH);
	wait_half_clock();
	bcm2835_gpio_write(CLK, LOW);
	//Send 1 as first bit
 //   printf("write %02X:0b1",data);
	bcm2835_gpio_write(MOSI, HIGH);
	wait_half_clock();
	bcm2835_gpio_write(CLK, HIGH);
	wait_half_clock();
	bcm2835_gpio_write(CLK, LOW);
	
	uint8_t mask;
	//mask start at from MSB and decrease to FSB
	//The bit corresponding to mask is sent
	for (mask=1<<7;mask>0;mask>>=1)
	{
		if (mask==1<<7) printf(" ");
//		printf("%X",(mask & data)/mask);
		bcm2835_gpio_write(MOSI, (mask & data)/mask);
		wait_half_clock();
		bcm2835_gpio_write(CLK, HIGH);
		wait_half_clock();
		bcm2835_gpio_write(CLK, LOW);
	}
//	printf("\n");
	bcm2835_gpio_write(MOSI, LOW);
}

void send24bit(uint32_t data)
{
	if (data>1<<24)
	{
		printf("ERROR: data too big\n");
		return;
	}
	//MOSI LOW by default
	bcm2835_gpio_write(MOSI, LOW);
	//Set CS to 1 
	bcm2835_gpio_write(CE1, HIGH);
	//wait for 2 us (spec is >0.4)
	bcm2835_delayMicroseconds(2);
	//1 clock period
	bcm2835_gpio_write(CLK, HIGH);
	wait_half_clock();
	bcm2835_gpio_write(CLK, LOW);
	//Send 1 as first bit
	printf("write24b %02X:0b1",data);
	bcm2835_gpio_write(MOSI, HIGH);
	wait_half_clock();
	bcm2835_gpio_write(CLK, HIGH);
	wait_half_clock();
	bcm2835_gpio_write(CLK, LOW);
	
	uint32_t mask;
	//mask start at from MSB and decrease to FSB
	//The bit corresponding to mask is sent
	for (mask=1<<23;mask>0;mask>>=1)
	{
		if (mask==1<<7) printf(" ");
		if (mask==1<<15) printf(" ");
		if (mask==1<<23) printf(" ");
//		printf("%X",(mask & data)/mask);
		bcm2835_gpio_write(MOSI, (mask & data)/mask);
		wait_half_clock();
		bcm2835_gpio_write(CLK, HIGH);
		wait_half_clock();
		bcm2835_gpio_write(CLK, LOW);
	}
//	printf("\n");
	bcm2835_gpio_write(MOSI, LOW);
}

uint16_t read16bit(void)
{
	//MISO HIGH by default
	bcm2835_gpio_set_pud(MISO, BCM2835_GPIO_PUD_UP);
	//Wait for 0 as first bit
	int i=0;
	while (bcm2835_gpio_lev(MISO)==1 && i<10)
	{
		wait_half_clock();
		bcm2835_gpio_write(CLK, HIGH);
		wait_half_clock();
		bcm2835_gpio_write(CLK, LOW);
		i++;
	}
	if(i!=0 && i<10)
		printf("delay for input = %i T_CLOCK\n",i);
	if(i==10)
	{
		printf("read failed\n");
		return 0;
	}//else
//		printf("read=0b%X",bcm2835_gpio_lev(MISO));
	
	wait_half_clock();
	bcm2835_gpio_write(CLK, HIGH);
	wait_half_clock();
	bcm2835_gpio_write(CLK, LOW);
	uint16_t data=0;
	uint16_t mask;
	//mask start at from MSB and decrease to FSB
	//The bit corresponding to mask is sent
	for (mask=1<<15;mask>0;mask>>=1)
	{
		if (mask==1<<15) printf(" ");
		if (mask==1<<7) printf(" ");
		data+=mask * bcm2835_gpio_lev(MISO);
//		printf("%X",bcm2835_gpio_lev(MISO));
		wait_half_clock();
		bcm2835_gpio_write(CLK, HIGH);
		wait_half_clock();
		bcm2835_gpio_write(CLK, LOW);
	}
//	printf("\n");
	return data;
}

void start_self_programming(void)
{
	wait_half_clock();
	bcm2835_gpio_write(CE1, LOW);
	bcm2835_gpio_write(CLK, HIGH);
	bcm2835_delayMicroseconds(2);//wait 2 us (spec is 1us min)
}

//check status after self programming cycle started
int check_status(void)
{
	//MISO pull down (we want 0 if HR state)
	bcm2835_gpio_set_pud(MISO, BCM2835_GPIO_PUD_DOWN);
	bcm2835_gpio_write(CE1, HIGH);
	bcm2835_delayMicroseconds(2);//wait 2 us (spec is 1us min)
	int status=bcm2835_gpio_lev(MISO);
	bcm2835_gpio_set_pud(MISO, BCM2835_GPIO_PUD_UP);//return to the default pull up
	return status; //0 is busy, 1 is ready
}

//wait until self programming cycle stoped or 20000 check status (spec is 15ms max)
void wait_until_ready(void)
{
	int i=0;
	while (!check_status()&& i<20000)
	{
		bcm2835_delayMicroseconds(1);
		i++;
	}
	if(i==20000 && !check_status() ) printf("ERROR: wait_until_ready failed (i=%i).\n",i);
	else printf("waited %i cycles for chip to be ready\n",i);
}

/**************************************************************
*				High level functions
**************************************************************/
void printf_binarry(uint32_t data,uint8_t nbit)
{
	int i;
	uint32_t mask;
	for (i=nbit-1;i>=0;i--)
	{
		mask=1<<i;
		printf("%i",(data&mask)/mask);
		if(i%8==0) printf(" ");
	}
	printf("\n");
}
//read 2 bytes at addr
uint16_t read_address(uint8_t addr)
{
	if (addr>=64)
	{
		printf("Address non valid\n");
		return 0;
	}
	//Read instruction: 10xxxxxx where xxxxxx is the address
	uint8_t sdata=(1<<7)+addr; 
	init();
	//Send read instruction
	send8bit(sdata);
	//Read 16 bits response
	uint16_t rdata = read16bit();
	 //Set CS to 0 
	start_self_programming();
	reset();
	return rdata;
}

//write data at addr in NMC9314
void write_address(uint8_t addr,uint16_t data)
{
	if (addr>=64)
	{
		printf("ERROR: Address non valid in  write_address(%X,%X)\n",addr,data);
		return;
	}
	//get present data at addr
	uint16_t present=read_address(addr);
	//don t write if not needed
	if( present==data )
	{
		printf("at 0x%X nothing to write\n");
		return;
	}
	//We can only write 0 on top of 1
/*	printf("present: ");printf_binarry(present,16);
	printf("data: ");printf_binarry(data,16);
	printf("present|data: ");printf_binarry(present|data,16);
	printf("~present: ");printf_binarry(~present,16);
	printf("(present|data) & (~present)): ");
	printf_binarry((present|data) & (~present),16);*/
	if( ((present|data) & (~present))>0 ) 
	{
		//if any bit of present==0 and corresponding bit in data ==1
		printf("ERROR: at 0x%X cant write 0x%2X: memory is 0x%2X\n",addr,data,present);
		printf("Erase that adrress prior to write\n");
		return;
	}
	//Write instruction: 01xxxxxxD...D where xxxxxx is the address
	//D...D are the 16 bits to write at that address
	uint32_t sdata= (((1<<6)+addr)<<16) + data;
	init();
	//Send read instruction
	send24bit(sdata);
	//Set CS to 0 for 1 us
	start_self_programming();
	//Wait for NMC9314 to be ready
	wait_until_ready();
	bcm2835_gpio_write(CE1, LOW);
	reset();
}

//read full memory of NMC9314 and put it into "memory" array
void read_memory(uint16_t* memory)
{
	int i;
	for (i=0;i<64;i++)
	{
		memory[i]=read_address(i);
	}
}

//write all addresses in memory with the 2 bytes of data
void write_all(uint16_t data)
{
	int i;
	//get present data 
	uint16_t memory[64];
	read_memory(memory);
	uint16_t present=0xFF;
	//a bit of present is 0 if any coresponding bit in memory is 0;
	for (i=0;i<64;i++)
			present&=memory[i];
	//We can only write 0 on top of 1
	if( ((present|data) & (~present))>0 ) 
	{
		//if any bit of present==0 and corresponding bit in data ==1
		printf("ERROR: cant write 0x%2X\n",data,present);
		printf("Erase the memory prior to write_all\n");
		return;
	}
	//Write_all instruction: 0001xxxxD...D where xxxx does not matter
	//D...D are the 16 bits to write everywhere
	uint32_t sdata= ((1<<4)<<16) + data;
	init();
	//Set CS to 1 
	bcm2835_gpio_write(CE1, HIGH);
	//Send read instruction
	send24bit(sdata);
	//Set CS to 0 for 1 us
	start_self_programming();
	//Wait for NMC9314 to be ready
	wait_until_ready();
	bcm2835_gpio_write(CE1, LOW);
	reset();
}

// Enable writing (off by default)
void enable_write(void)
{
	//Enable write instruction: 0011xxxx where xxxx does not matter
	uint8_t sdata=(1<<5)+(1<<4); 
	init();
	//Send read instruction
	send8bit(sdata);
	//Set CS to 0 
	start_self_programming();
	reset();
}

// Disable writting
void disable_write(void)
{
	//Enable write instruction: 0000xxxx where xxxx does not matter
	uint8_t sdata=0; 
	init();
	//Send read instruction
	send8bit(sdata);
	//Set CS to 0 
	start_self_programming();
	reset();
}

//Erase the memory at the specified address (data is then 0xFF)
void erase_address(uint8_t addr)
{
	if (addr>=64)
	{
		printf("Address non valid\n");
		return;
	}
	//Erase instruction: 11xxxxxx where xxxxxx is the address
	uint8_t sdata=(1<<7)+(1<<6)+addr; 
	init();
	//Send read instruction
	send8bit(sdata);
	//Set CS to 0 
	start_self_programming();
	wait_until_ready();
	bcm2835_gpio_write(CE1, LOW);
	reset();
}

//Erase the full memory (data is then 0xFF everywhere)
void erase_all(void)
{
	//ERAL instruction: 0010xxxx where xxxx does not matter
	uint8_t sdata=(1<<5); 
	init();
	//Send read instruction
	send8bit(sdata);
	//Set CS to 0 
	start_self_programming();
	wait_until_ready();
	bcm2835_gpio_write(CE1, LOW);
	reset();
}

//write full memory of NMC9314 with values in "memory" array
void write_memory(uint16_t* memory)
{
	int i;
	for (i=0;i<64;i++)
	{
		write_address(i,memory[i]);
	}
}

int write_memory_check(uint16_t* memory)
{
	int i,nerror;
	uint16_t read;
	for (i=0;i<64;i++)
	{
		write_address(i,memory[i]);
		read=read_address(i);
		if (read!=memory[i])
		{
			printf("ERROR write_memory_check: read diff from what asked (%04X insteat of %04X)\n");
			nerror++;
		}
	}
	return;
}

//Hex representation of values in memory 
void print_memory(uint16_t* memory)
{
	char char1,char2;
	int addr,i;
	for (addr=0;addr<64;addr++)
	{
		if(addr%8==0) printf("0x%02X: ",addr);
		printf("%04X ",memory[addr]);
		if(addr%4==3) printf(" ");
		if(addr%8==7) 
		{
			for (i=0;i<8;i++)
			{
				if (i==4) printf(" ");
				char1=memory[addr-7+i]>>8;
				char2=memory[addr-7+i]%256;
				if(char1>31 && char1<127) 
						printf("%c",char1);
				else printf(".");
				if(char2>31 && char2<127) 
						printf("%c",char2);
				else printf(".");
			}
			printf("\n");
		}
	}	
}

int check_memory(uint16_t *memory)
{
	int addr;
	uint16_t read;
	int nerror=0;
	for (addr=0;addr<64;addr++)
	{
		read=read_address(addr);
		if(read!=memory[addr]);
		{
			printf("ERROR at address 0x%X: read %04X instead of %04X\n",addr,read,memory[addr]);
			nerror++;
		}
	}
}

void test_read_speed(void)
{

	uint16_t mem[64];
	float dif;
	struct timespec start;
	struct timespec end;
	clock_gettime(CLOCK_MONOTONIC,&start);
	read_memory(mem);
	clock_gettime(CLOCK_MONOTONIC,&end);
	dif=(float)(end.tv_sec-start.tv_sec + ((float)(end.tv_nsec-start.tv_nsec))/1000000000);
	printf("write 64*9bits, read 64*17bits in %.2fs.\n",dif);
	printf("bus speed=%.1f kb/s.\n",(64*9+64*17)/dif/1024);
	printf("read speed=%.1f kb/s.\n",64*16/dif/1024);
	print_memory(mem);
}

void test_write_speed(void)
{
	uint8_t i;
	uint16_t mem[64];
	float dif;
	struct timespec start;
	struct timespec end;
	uint8_t nerror=0;
	enable_write();
	erase_all();
	for(i=0;i<64;i++)
		mem[i]=i<<8;
	clock_gettime(CLOCK_MONOTONIC,&start);
	nerror=write_memory_check(mem);
	clock_gettime(CLOCK_MONOTONIC,&end);
	dif=(float)(end.tv_sec-start.tv_sec + ((float)(end.tv_nsec-start.tv_nsec))/1000000000);
	disable_write();
	printf("write 64*25+64*8 bits, read 64*17bits in %.2fs.\n",dif);
	printf("bus speed=%.1f kb/s.\n",(64*25+64*8+64*17)/dif/1024);
	printf("write speed=%.1f kb/s.\n",64*16/dif/1024);
	printf("%i write errors detected\n",nerror);
	print_memory(mem);
}

void usage(char *program_name)
{
	printf("usage: %s [-h] [n] [-E | -a addr -e] [-d data [-W | -a addr -w]] [-s] [-R | -a addr -r].\n",program_name);
	printf("program to handle the NMC9314 memory (64 address of 2 bytes memory), other microwire compatible devices may need little program modification.\nparameters are:\n",program_name);
	printf("\t -h|--help : this help\n");
	printf("\t -n|--enable : enable write on NMC9314 (enabled by default if any modification to NMC9314 memory is aked for).\n");
	printf("\t -E|--eraseall : erase all NMC9314.\n");
	printf("\t -e|--erase : erase address given by -a|--address.\n");
	printf("\t -W|--writeall : write all address with the data given by -d|--dat.a\n");
	printf("\t -w|--write : write address given by -a|--address with the data given by -d|--data.\n");
	printf("\t -s|--disable : disable write on NMC9314 (enabled by default if any modification to NMC9314 memory is aked for).\n");
	printf("\t -R|--readall : read all addresses and display.\n");
	printf("\t -r|--read : read address given by -a|--address and display.\n");
	printf("\t -a |--address addr: address addr to be erased/wrote/read (integer between 0 and 63) eg -a 1. If more than 1 operation is performed, same address will be used each time.\n");
	printf("\t -d|--data data: 4 hexadecimal caracters to be written eg. \"-d FE01\".\n");
	printf("\t -v|--verbose : be verbose (default).\n");
	printf("\t -b|--brief : be brief (not verbose, not implemented).\n");
}


int main(int argc, char **argv)
{
	uint8_t addr = 0;
	uint16_t data =0;
	int bflag = 0;
	char *cvalue = NULL;
	int index;
	int c;
	static int read_flag=0,erase_flag=0,write_flag=0,enable_flag=0;
	static int readall_flag=0,eraseall_flag=0,writeall_flag=0,disable_flag=0;
	static int addr_flag=0,data_flag=0,help_flag=0;
	char input;
	opterr = 0;
	char prog_name[50];
	strcpy(prog_name,argv[0]);

	while (1)
	{
		static struct option long_options[] =
		{
			{"help", no_argument, &help_flag, 1},
			{"enable", no_argument, &enable_flag, 1},
			{"eraseall", no_argument, &eraseall_flag, 1},
			{"erase", no_argument, &erase_flag, 1},
			{"writeall", no_argument, &writeall_flag, 1},
			{"write", no_argument,  &write_flag, 1},
			{"disable", no_argument,  &disable_flag, 1},
			{"readall", no_argument, &readall_flag, 1},
			{"read", no_argument, &read_flag, 1},
			{"address", required_argument, NULL, 'a'},
			{"data", required_argument, NULL, 'd'},
			{"verbose", no_argument, &verbose_flag, 1},
			{"brief",   no_argument, &verbose_flag, 0},
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;
	 
		c = getopt_long (argc, argv, "hnEeWwsRra:d:vb",long_options, &option_index);
	 
		if (c == -1)
			break;

		switch (c)
		{
			case 'h': //help
				help_flag = 1;
				break;
			case 'n': //enable write
				enable_flag = 1;
				break;
			case 'E': //erase all
				enable_flag = 1;
				eraseall_flag=1;
				disable_flag = 1;
				break;
			case 'e': //erase address
				enable_flag = 1;
				erase_flag=1;
				disable_flag = 1;
				break;
			case 'W': //write all
				enable_flag = 1;
				writeall_flag = 1;
				disable_flag = 1;
				break;
			case 'w': //write address
				enable_flag = 1;
				write_flag = 1;
				disable_flag = 1;
				break;
			case 's': //disable write
				disable_flag = 1;
				break;
			case 'R': //read all
				readall_flag=1;
				break;
			case 'r': //read address
				read_flag=1;
				break;
			case 'a'://address
				addr = strtoll(optarg,NULL,0);
				addr_flag=1;
				break;
			case 'd'://data
				data = strtoll(optarg,NULL,16);
				data_flag=1;
				break;
			case 'v': //verbose
				verbose_flag=1;
				break;
			case 'b': //brief
				verbose_flag=0;
				break;
			case '?':
				if (optopt == 'a')
					fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (optopt == 'd')
					fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
				usage(prog_name);
				return 1;
			default:
				usage(prog_name);
				return 1;
		}
	}
	for (index = optind; index < argc; index++)
		printf ("Non-option argument %s\n", argv[index]);

	if(help_flag)
	{
		usage(prog_name);
		return 1;
	}

//Check for incompatibilities in paresed options
	if(!enable_flag && !eraseall_flag && !erase_flag && !writeall_flag && !write_flag && !readall_flag && !read_flag && !disable_flag)
	{
		printf("%s: no action given.\n",prog_name);
		usage(prog_name);
		return 1;
	}
	if(eraseall_flag && erase_flag)
	{
		usage(prog_name);
		printf("--eraseall and --erase can t be used at the same time\n");
		return 1;
	}
	if(erase_flag && !(addr_flag) )
	{
		usage(prog_name);
		printf("--erase needs --address to be given\n");
		return 1;
	}
	if(writeall_flag && write_flag)
	{
		usage(prog_name);
		printf("--writeall and --write can t be used at the same time\n");
		return 1;
	}
	if(writeall_flag && !data_flag )
	{
		usage(prog_name);
		printf("--writeall needs --data to be given\n");
		return 1;
	}
	if(write_flag && !(data_flag&&addr_flag) )
	{
		usage(prog_name);
		printf("--write needs both --address and --data to be given\n");
		return 1;
	}
	if(read_flag && !addr_flag )
	{
		usage(prog_name);
		printf("--read needs --address to be given\n");
		return 1;
	}
    
    if(verbose_flag)
	    printf("The setup must be: \n");
	if(verbose_flag)
	    printf("Pi's GPIO%i connected to CS (pin 1 of NMC9314)\n",CE1);
	if(verbose_flag)
	    printf("Pi's GPIO%i connected to SK (pin 2 of NMC9314)\n",CLK);
	if(verbose_flag)
	    printf("Pi's GPIO%i connected to DI (pin 3 of NMC9314)\n",MOSI);
	if(verbose_flag)
	    printf("Pi's GPIO%i connected to DO (pin 4 of NMC9314)\n",MISO);
	if(verbose_flag)
	    printf("I used 74126 as buffer for Pi's outputs (CS,SK,DI) and Volage divisor for Pi's input (DO).\nYou can use 5V<->3.3V level converter as well but DON'T connect directly NMC9314's DO to Pi's GPIO%i)\n",MISO);

//display an ordered list of the action to be taken
	if(verbose_flag)
	    printf("The following actions will be performed:\n\n");
	int noperation=1;
	if(verbose_flag)
	{
	    if(enable_flag)
		    printf("%i) enable writing\n",noperation++);
	    if(eraseall_flag)
		    printf("%i) erase all memory\n",noperation++);
	    if(erase_flag)
		    printf("%i) erase 0x%02X\n\n",noperation++,addr);
	    if(writeall_flag)
		    printf("%i) write all memory with 0x%04X\n",noperation++,data);
	    if(write_flag)
		    printf("%i) write at 0x%02X with 0x%04X\n",noperation++,addr,data);
	    if(disable_flag)
		    printf("%i) disable writing\n",noperation++);
	    if(readall_flag)
		    printf("%i) read all memory\n",noperation++);
	    if(read_flag)
		    printf("%i) read at 0x%02X\n",noperation++,addr);
	    printf("NOTE: read operration are performed before and after any write to check writability and good writing). I offer no waranty, this program may (unlikely) damage your raspberry pi, your memory and/or any electronic device connected. Use it at your own risk.\n");
	    printf("Continue [Y/n]?\n");
	    input=getchar();
	    switch(input)
	    {
		    case 'n':
		    case 'N':
			    return 0;
			    break;
		    default:
			    break;
	    }
	}
//Print warning if willing to write to NMC9314
/*	if(verbose && (eraseall_flag || erase_flag || writeall_flag || write_flag) )
	{
		printf("memory of NMC9030 to be changed, continue [y/N]?\n");
		input=getchar();
		switch(input)
		{
			case 'y':
			case 'Y':
				break;
			default:
				printf("recived \"%c\", exit\n",input);
				return 0;
		}
	}*/
	if(enable_flag)
	{
		enable_write();
		
	}
	if(eraseall_flag)
	{
		erase_all();
	}
	if(erase_flag)
	{
		erase_address(addr);
	}
	if(writeall_flag)
	{
		write_all(data);
	}
	if(write_flag)
	{
		write_address(addr,data);
	}
	if(readall_flag)
	{
	    printf("read memory start\n");
		uint16_t mem[64];
		read_memory(mem);
		printf("\n");
		print_memory(mem);
	}
	if(read_flag)
	{
		data=read_address(addr);
		printf("read 0x%04X at 0x%02X\n",data,addr);
	}
	if(disable_flag)
	{
		disable_write();
	}
	if(verbose_flag)
	    printf("Done\n");
	return 0;
}

