#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/dir.h>
#include <dirent.h>
#include <linux/types.h>
#include <linux/input.h>

#include <ncurses.h>

#define READ	0x01
#define WRITE	0x02
//#define WRITE_BYTE 0x04
//#define WRITE_4_BYTE 0x08

#define CORRECT 0
#define ERROR -1

//#define	IOCTL_SET_COMMAND	0x01
//#define	IOCTL_READ_COMMAND	0x02
//#define	IOCTL_WRITE_COMMAND	0x03
#define IOC_MAGIC 'i'

#define IOCTL_SET_COMMAND    _IOW(IOC_MAGIC, 0, int)
#define IOCTL_READ_COMMAND   _IOR(IOC_MAGIC, 1, int)
#define IOCTL_WRITE_COMMAND  _IOW(IOC_MAGIC, 2, int)

static unsigned char statMode = 0;
static unsigned long int read_start_address;
static unsigned long int read_end_address;
static unsigned long int write_target_address;
static unsigned long int write_value;

static unsigned long int *read_value;
static int read_number;
static int read_column_length;
static int read_row_length;

struct iomap_address{
	unsigned long int start;
	unsigned long int end;
};

struct write_wrapper{
	unsigned long int target;
	unsigned long int value;
};

struct read_wrapper{
	unsigned long int value;
	int offset;
};

//static unsigned int height;   // window height
//static unsigned int moreheaders = TRUE;

#define DATA_BYTE				 4
#define DATA_DISPLAY_PER_ROW			 4
#define IO_MEMORY_DATA_LENGTH			10
#define IO_MEMORY_ADDRESS_WPRINT_OFFSET_LENGTH	 7
#define X_CURSOR_POS_START IO_MEMORY_ADDRESS_WPRINT_OFFSET_LENGTH * 2 + IO_MEMORY_DATA_LENGTH * 2 - 1
#define Y_CURSOR_POS_START 			 1	
 
#define IO_MEMORY_ADDRESS_PRINT_OFFSET printf("       "); // 7 space
#define IO_MEMORY_ADDRESS_WPRINT_OFFSET printw("       ");

const char diag_version[] = "Title : Diagnosis Program for I/O Mapping\nAuthor : KNight Weng";

/* Function Prototype Start */
static void usage(void);
//static void print_header(void);
static void print_header_cursor(void);
static void read_command(unsigned long int, unsigned long int);
static void read_command_test(unsigned long int, unsigned long int);
static void write_command(unsigned long int, unsigned long int);
void diag_display_version(void);
void cursor_page_layout(void);
void init_cursor(void);
//void clr(void);
/* Function Prototype End */

/* Function Implementation Start */

/* Print Help */
static void usage(void) {
  	fprintf(stderr,"usage:  reg_dumper   [-r range_start range_end]\n"); 
	fprintf(stderr,"                     [-w target_address value ]\n"); 
	fprintf(stderr,"		     [-v] [-q] [-r]\n");
	fprintf(stderr,"                      -r read command, this will enter interaction mode.\n");
	fprintf(stderr,"                      -w write command.\n");
  	fprintf(stderr,"                      -v prints version.\n");
	fprintf(stderr,"At interaction mode,  keyboard 'q' quit from program\n");
	fprintf(stderr,"		      keyboard 'r' refresh data showed in screen\n");				
  	//fprintf(stderr,"                      -c the number of updates.\n");
  	exit(EXIT_FAILURE);
}

/* Print Information Header */
/*static void print_header(void){
	//clr();
	system("clear");
  	printf("--I/0 Memory Address--  -----------------Dump Data-----------------\n");
}*/

static void print_header_cursor(void){
	clear();
  	printw("--I/0 Memory Address--  -----------------Dump Data-----------------\n");
}

static void read_command(unsigned long int start, unsigned long int end)
{
	int i;
	int devfd,retval;
	//unsigned long int *readValueArray;
	int offset = (end - start + 1) / DATA_BYTE;
	if( ((end - start + 1) % DATA_BYTE) != 0 )
		offset++; 

	/* Decide total number */
	read_number = offset;

	/* Decide cursor row length */
	read_row_length = offset / DATA_DISPLAY_PER_ROW;
	if( (offset % DATA_DISPLAY_PER_ROW) != 0 )
		read_row_length++;	

	/* Decide cursor column length */
	if(offset < DATA_DISPLAY_PER_ROW)
		read_column_length = offset;
	else
		read_column_length = DATA_DISPLAY_PER_ROW;

	/* IO control start ... */
	devfd = open("/dev/iomem", O_RDWR);
    	if (devfd == -1) {
		printf("Can't open /dev/iomem\n");
		exit(ERROR);
    	}
	//print_header();	

	struct iomap_address *iomap;
	iomap = malloc(sizeof(struct iomap_address));
	if(!iomap){
		printf("No more memory\n");
		exit(ERROR);
	}
	iomap->start = start;
	iomap->end = end;

    	retval = ioctl(devfd, IOCTL_SET_COMMAND, iomap);
	if(retval == -1){
		printf("ioctl(devfd, IOCTL_SET_COMMAND, iomap) fault\n");
		exit(ERROR);
	}
    	//printf("IOCTL_SET_COMMAND Done. Wait 1 seconds...\n");
    	//sleep(1);

	struct read_wrapper *wrap;
	wrap = malloc(sizeof(struct read_wrapper));
	if(!wrap){
		printf("No more memory\n");
		exit(ERROR);
	}

	read_value = (unsigned long int *)malloc(offset * sizeof(unsigned long int));
	if(!read_value){
		printf("No more memory\n");
		exit(ERROR);
	}

	/*retval = ioctl(devfd, IOCTL_READ_COMMAND, read_value);
	if(retval == -1){
		printf("ioctl(devfd, IOCTL_READ_COMMAND, read_value) fault\n");
		exit(ERROR);
	}*/

	for( i = 0 ; i < offset ; i++)
	{
		wrap->value = 0;
		wrap->offset = (unsigned int)i;
		retval = ioctl(devfd, IOCTL_READ_COMMAND, wrap);
		if(retval == -1){
			printf("ioctl(devfd, IOCTL_READ_COMMAND, wrap) fault\n");
			exit(ERROR);
		}	
		read_value[i] = wrap->value;
	}

	close(devfd);

	/************* back up code ******************/
		/*if( (i % DATA_DISPLAY_PER_ROW) == 0 ){
			if( i != 0 )
				printf("\n");
			IO_MEMORY_ADDRESS_PRINT_OFFSET;
			printf("0x%08lX", (ulong)start + i * DATA_DISPLAY_PER_ROW);
			IO_MEMORY_ADDRESS_PRINT_OFFSET;
		}	
		printf("0x%08lX ",wrap->value);			
	}
	printf("\n");*/
	/*readValueArray = (unsigned long int *)malloc(offset * sizeof(unsigned long int));
	if(!readValueArray){
		printf("No more memory\n");
		exit(ERROR);
	}
	retval = ioctl(devfd, IOCTL_READ_COMMAND, readValueArray);
	if(retval == -1){
		printf("ioctl(devfd, IOCTL_READ_COMMAND, readValueArray) fault\n");
		exit(ERROR);
	}	

	for( i = 0 ; i < offset ; i++)
	{
		if( (i % 4) == 0 ){
			if( i != 0 )
				printf("\n");
			IO_MEMORY_ADDRESS_PRINT_OFFSET;
			printf("0x%08lX", (unsigned long int)start + i * 4);
			//printf("0x%08lX", (long unsigned int)start + i);
			IO_MEMORY_ADDRESS_PRINT_OFFSET;
		}	
		printf("0x%08lX ",readValueArray[i]);
		//printf("0x%08lX ",wrap->value);	
		//printf("0x%02X     ", value[i]);			
	}
	printf("\n");*/
}

static void read_command_test(unsigned long int start, unsigned long int end)
{
	//ulong j;
	int i;
	//unsigned long int *read_value;
	//int devfd,retval;
	int offset = (end - start + 1) / DATA_BYTE;
	if( ((end - start + 1) % DATA_BYTE) != 0 )
		offset++; 

	read_number = offset;

	read_row_length = offset / DATA_DISPLAY_PER_ROW;
	if( (offset % DATA_DISPLAY_PER_ROW) != 0 )
		read_row_length++;	

	if(offset < DATA_DISPLAY_PER_ROW)
		read_column_length = offset;
	else
		read_column_length = DATA_DISPLAY_PER_ROW;
	
	read_value = (unsigned long int *)malloc(offset * sizeof(unsigned long int));	
	if(!read_value){
		printf("No more memory\n");
		exit(ERROR);
	}

	for( i = 0 ; i < offset ; i++)
		read_value[i] = 0;	
	

	/*for(j = 0 ; j < num_updates ; j++){

	print_header();

	FILE *fileTmp;
	fileTmp = fopen("fileTmp", "w+");
	fprintf(fileTmp,"--I/0 Memory Address--  -----------------Dump Data-----------------\n");
	
	for( i = 0 ; i < offset ; i++)
	{
		if( (i % DATA_DISPLAY_PER_ROW) == 0 ){
			if( i != 0 ){
				printf("\n");
				fprintf(fileTmp,"\n");
			}
			//IO_MEMORY_ADDRESS_PRINT_OFFSET;
			//printf("0x%08lX", (ulong)start + i * 4);
			//IO_MEMORY_ADDRESS_PRINT_OFFSET;
			IO_MEMORY_ADDRESS_FPRINT_OFFSET;
			fprintf(fileTmp,"0x%08lX", (ulong)start + i * DATA_DISPLAY_PER_ROW);
			IO_MEMORY_ADDRESS_FPRINT_OFFSET;
		}	
		//printf("0x%08lX ", read_value[i]);	
		fprintf(fileTmp,"0x%08lX ", read_value[i]);	
	}
	printf("\n");
	fclose(fileTmp);
	for( i = 0 ; i < offset ; i++)
		read_value[i]++;
	sleep(1);
	}*/
}

static void write_command(unsigned long int target, unsigned long int value)
{
	int devfd,retval;

	devfd = open("/dev/iomem", O_RDWR);
    	if (devfd == -1) {
		printf("Can't open /dev/iomem\n");
		exit(ERROR);
    	}	

	struct write_wrapper *wrap;
	wrap = malloc(sizeof(struct write_wrapper));
	if(!wrap){
		printf("No more memory\n");
		exit(ERROR);
	}
	wrap->target = target;
	wrap->value = value;

    	retval = ioctl(devfd, IOCTL_WRITE_COMMAND, wrap);
	if(retval == -1){
		printf("ioctl(devfd, IOCTL_WRITE_COMMAND, wrap) fault\n");
		exit(ERROR);
	}
    	printf("IOCTL_WRITE_COMMAND Done.\n");
	close(devfd);
}

void diag_display_version(void){
	fprintf(stdout, "%s\n", diag_version);
} 

int main(int argc, char *argv[]) {
	(void)argc;
	char *ptr;

  	for (argv++;*argv;argv++) {
    		if ('-' ==(**argv)) {
      			switch (*(++(*argv))) {
				case 'v':
					diag_display_version();
					exit(CORRECT);
				case 'r':	
					if((ptr = strstr(*++argv, "0x")) != NULL){
						read_start_address = strtoul(ptr,&ptr,16);
						//printf("start_address = 0x%08lX\n",(long unsigned int)read_start_address);
						if((ptr = strstr(*++argv, "0x")) != NULL){
							read_end_address = strtoul(ptr,&ptr,16);
							//printf("end_address = 0x%08lX\n",(long unsigned int)read_end_address);
						}
						else{
							fprintf(stderr, "end address prefix '0x' requires\n");	
							exit(ERROR);
						} 
					}else{
						fprintf(stderr, "start address prefix '0x' requires\n"); 
						exit(ERROR);
					}
					long start_address_value = (long)read_start_address;	
					long end_address_value = (long)read_end_address;	
					if((end_address_value - start_address_value) < 0){
						fprintf(stderr, "start address is bigger than end address\n");
						exit(ERROR);				
					}
					statMode |= READ;
					break;
				case 'w':	
					if((ptr = strstr(*++argv, "0x")) != NULL){
						write_target_address = strtoul(ptr,&ptr,16);
						//printf("target_address = 0x%08lX\n",(long unsigned int)write_target_address);
						if((ptr = strstr(*++argv, "0x")) != NULL){
							write_value = strtoul(ptr,&ptr,16);
							//printf("write_value = 0x%08lX\n",(long unsigned int)write_value);
						}
						else{
							fprintf(stderr, "write_value prefix '0x' requires\n");	
							exit(ERROR);
						}
					}else{
						fprintf(stderr, "target_address prefix '0x' requires\n"); 
						exit(ERROR);
					}
					statMode |= WRITE;
					break;
				/*case 'c':					
					num_updates = atol(*++argv);
					printf("num_updates = %lu\n",num_updates);
       					break;*/
      				default:
					usage(); 
      			}
   		}
	}

  	switch(statMode){
		case(READ): 
			//read_command_test(read_start_address, read_end_address);
			read_command(read_start_address, read_end_address);
			init_cursor();
			break;
		case(WRITE): 
			//write_command(write_target_address, write_value, statMode);	
			write_command(write_target_address, write_value);	
			break;
		default:	
			usage();
			break;
  	}
  	return 0;
}

void cursor_page_layout(void)
{
	int i;

	print_header_cursor();
	for( i = 0 ; i < read_number; i++)
	{
		if( (i % DATA_DISPLAY_PER_ROW) == 0 ){
			if( i != 0 )
				printw("\n");
			IO_MEMORY_ADDRESS_WPRINT_OFFSET;
			printw("0x%08lX", (ulong)read_start_address + i * DATA_DISPLAY_PER_ROW);
			IO_MEMORY_ADDRESS_WPRINT_OFFSET;
		}	
		printw("0x%08lX ", read_value[i]);	
	}	
}

void init_cursor(void)
{	
	int ch;
	int x,y;
	unsigned long int column_cur = 0;
	unsigned long int row_cur = 0;
	
	unsigned long int scan_write_value = 0;
	char *ptr;
	ptr = (char *)malloc(8 * sizeof(char));
	if(!ptr){
		printf("No more memory\n");
		exit(ERROR);
	}

	initscr();			
	curs_set(1);
	cbreak();			
	keypad(stdscr, TRUE);		
	noecho();

	cursor_page_layout();

	move(Y_CURSOR_POS_START,X_CURSOR_POS_START);
	for(;;){
		ch = getch();
		switch(ch){		
			case KEY_UP:
			{
				getyx(stdscr, y, x);
				if(y <= Y_CURSOR_POS_START)
					break;	
				else{	 
					move(y-1,x);
					row_cur--;
				}
				break;
			}
			case KEY_DOWN:
			{
				getyx(stdscr, y, x);
				if(y >= Y_CURSOR_POS_START + read_row_length - 1)
					break;
				else{					
					move(y+1,x);
					row_cur++;
				}
				break;
			}	
			case KEY_RIGHT:
			{
				getyx(stdscr, y, x);
				if(x >= X_CURSOR_POS_START + (read_column_length - 1) * (IO_MEMORY_DATA_LENGTH + 1))
					break;
				else{	 
					move(y,x + (IO_MEMORY_DATA_LENGTH + 1));
					column_cur++;
				}
				break;
			}
			case KEY_LEFT:
			{
				getyx(stdscr, y, x);
				if(x <= X_CURSOR_POS_START)
					break;	
				else{	 				
					move(y,x - (IO_MEMORY_DATA_LENGTH + 1));
					column_cur--;
				}
				break;
			}
			case 'q':
			{
				goto end_point;
			}
			case 'r':
			{
				read_command(read_start_address, read_end_address);
				cursor_page_layout();
				move(Y_CURSOR_POS_START,X_CURSOR_POS_START);
				column_cur = 0;
				row_cur = 0;	
			}
			case 10: /* Enter */
			{
				getyx(stdscr, y, x);
				move(y,x - IO_MEMORY_DATA_LENGTH + 1);
				printw("0x");
				echo();
				getstr(ptr);
				noecho();
				scan_write_value = strtoul(ptr,&ptr,16);
				//move(y+10,x+10);
				//printw("scan_write_value = 0x%08lX",scan_write_value);

				/* ...........write.......... */
				unsigned long int write_address = read_start_address + (column_cur * DATA_BYTE) + (row_cur * DATA_DISPLAY_PER_ROW * DATA_BYTE);	
				
				write_command(write_address, (unsigned long int)scan_write_value);
				sleep(1);
				read_command(read_start_address, read_end_address);
				cursor_page_layout();
				//move(y+10,x+10);
				//printw("write_address = 0x%08lX", write_address);
				//printw("column_cur = %d, row_cur = %d",column_cur, row_cur);
				move(y,x);
				break;
			}
		}
	}

end_point:
	endwin();
	return;
}

/*void clr(void)
{
  	char *clear;
  	char clbuf[1024];
  	char *clbp = clbuf;

  	if (tgetent(clbuf, getenv("TERM")) == ERROR) {
     		perror("tgetent");
     		exit(ERROR);
  	}

  	if ((clear = tgetstr("cl", &clbp)) == NULL) {
     		perror("tgetstr");
     		exit(ERROR);
  	}

  	if (clear)
     		tputs(clear, tgetnum("li"), putchar);

}*/




