
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h> 
#include <string.h>
#include <math.h>

#include <time.h>
#include <windows.h>
#include <windef.h>
#include <winnt.h>
#include <winbase.h>

//prototypes
FILE *open_file ( uint8_t *file, uint8_t *mode );
uint8_t read_byte(FILE * file, uint8_t * char_to_put, int * total_chars_read);
void clear_special_char(FILE * file, uint8_t * charToPut, int * totalCharsRead);
int read_line(FILE * file, uint8_t line_of_data[], long int * line_address, int * bytes_in_line,uint8_t*line_checksum);
uint8_t Ascii2Hex(uint8_t c);
int line_counter(FILE * file_to_count);
int hex2array(FILE * file);
void select_file();
void convert_file();
void Delay(int d);
void save_array(int size);
//variables
uint8_t HEX_array[32768];
bool fileselected=0;
char file_name[20]="No File";
FILE * fp;
bool arrayOnScreen=0;
		


void main()
{
	char ch[3];
	int int_ch = 0;
	mainRST: ;
	system("cls");//clear screen
	printf("\n");
	printf("Menu:    ");

	if(fileselected){printf("Selected File: %s\n",file_name);}
	else printf("No File Selected\n");
	printf("\n1. Select Hex File\n");
	printf("2. Convert Hex File\n");
	printf("3. Exit\n");
	
	scanf("%s", ch);
	int_ch = atoi(ch);

	switch (int_ch)
	{
		case 1:
			select_file();
			goto mainRST;
		
		case 2:
			convert_file();
			goto mainRST;
		case 3:
			exit(0);
			break;
		default:
			printf("Invalid. Enter Again");
			Delay(600);
			goto mainRST;
		}

}

void select_file()
{
	selRST: ;
	system("cls");//clear screen
	printf("Enter File Name:\n");
	scanf("%s", file_name);
	fp = fopen(file_name,"r");//File to be loaded.	
	if(fp)fileselected=1;
	else
	{
		fileselected=0;
		printf("Invalid. Enter Again");
		Delay(600);
		goto selRST;
		
	}
}


void Delay(int d){
	int i,j=0;
	for(j=0;j<(d*20);j++){
		for(i=0;i<32768;i++){}
	}
}
	
void convert_file()
{
	if(fileselected){
		system("cls");//clear screen
		// Data array
		printf("Attempting Conversion of..%s\n",file_name);

		// Bytes read into array. // Load the data from file
		int  HEX_array_size = hex2array(fp);
		fclose(fp);
		save_array(HEX_array_size);
		char ch2[3];
		printf("Press a Key 0-9 to end \n");
		Delay(200);
		convRST2: ;
		scanf("%c", ch2);
		scanf("%c", ch2);
		uint8_t int_ch = atoi(ch2);

		switch (int_ch)
		{
			
			case 0:case 1:case 2:case 3:case 4:case 5:
			case 6:case 7:case 8:case 9:
				exit(0);
				break;
			default:
			
			goto convRST2;
		}
		
	}
	else{ printf("Error: File Not Selected\n");}
	Delay(1000);
	fileselected=0;
}

void save_array(int size)
{
	printf("file size: %d Bytes\n",size);
	FILE *fp2;
	uint16_t i=0;
	fp2=fopen("AVRbins.h","wb");
	fprintf(fp2,"#ifndef AVRBINS_H \n #define AVRBINS_H \n");
	//fprintf(fp2,"const uint8_t AVR_binaries[%d]={ \n  0x%02X",size,HEX_array[0]);
	fprintf(fp2,"const uint8_t AVR_binaries[]={ \n  0x%02X",HEX_array[0]);
	for(i=1;i<size;i++){
		fprintf(fp2,", 0x%02X",HEX_array[i]);
		if((i+1)%16==0)fprintf(fp2,"\n");
	}
	fprintf(fp2,"\n};\n");
	fprintf(fp2,"#endif");
	fclose(fp2);
}


int hex2array(FILE * file)
{
	uint8_t line_of_data[32];
	long int line_address[4096];//array of found addresses
	int bytes_in_line[4096];//array of saved data length per line
	uint8_t line_checksum[4096];//array of corruption flags
	
	// counters
	int total_bytes_read = 0;
	int line_count = 0;
	int line_index = 0;
	int byte_index = 0;
	
	line_count = line_counter(file);//count how many lines total in file

	// Parse all lines in file.
	while(line_index < line_count)
	{//increments line counter, moving through the file
		//reads the line data whilst keeping track of how many bytes of data have been found.
		total_bytes_read+=read_line(file, line_of_data, &line_address[line_index], &bytes_in_line[line_index],&line_checksum[line_index]);
		
		while(byte_index < bytes_in_line[line_index])
		{
			HEX_array[line_address[line_index] + byte_index] = line_of_data[byte_index];//put data in array according to address
			line_of_data[byte_index] = '\0';
			byte_index++;
		}
		byte_index = 0;
		line_index++;
	}
	if(arrayOnScreen)
	{
		system("cls");//clear screen
		printf("*** HEX DATA ***\n");
		printf("line count: %d\n\n",line_count);//display line count
		// Print out parsed data.
		int j,k= 0;
		int cursor = 0;
		while (k < line_count)
		{
			printf("line %d:",k+1);//line No.
			
			while(j < bytes_in_line[k])
			{
				printf("%02X ", HEX_array[j+cursor]);//data
				j++;
			}
			cursor += bytes_in_line[k];
			j=0;
			if(k<9)printf(" ");
			printf("   data Length: %d",bytes_in_line[k]);//data in that line
			printf("    line ok:%%d\n",line_checksum[k]);
			k++;
		}	

	}
	return total_bytes_read;
}

int read_line(FILE * file, uint8_t line_of_data[], long int * line_address, int * bytes_in_line,uint8_t*line_checksum)
{
		int data_index = 0;
		uint8_t char_to_put;
		int total_chars_read = 0;
		uint8_t databyte=0;
		uint32_t totaldata=0;
		//To hold file hex values.
		uint8_t  byte_count;
		uint8_t  address1;
		uint8_t  address2;
		uint8_t  type;
		uint8_t  check_sum;

		
		byte_count = read_byte(file, &char_to_put, &total_chars_read);

		// No need to read if no data.
		if (byte_count == 0){return false;}

		//address is 2 bytes long
		address1 = read_byte(file, &char_to_put, &total_chars_read);
		address2 = read_byte(file, &char_to_put, &total_chars_read);

		
		type = read_byte(file, &char_to_put, &total_chars_read);		

		// No need to read if not data.
		if ( record_type != 0){return false;}

		*line_address = ((uint16_t) address1 << 8) |  address2;

		
		while(data_index < byte_count)
		{
			databyte=line_of_data[data_index] = read_byte(file, &char_to_put, &total_chars_read);
			totaldata+=databyte;
			data_index++;
		}			
		*bytes_in_line = data_index;

		
		check_sum = read_byte(file, &char_to_put, &total_chars_read);
		uint16_t sumation=0x0000;
		uint8_t sum=0x00;
		sumation=byte_count+address1+address2+record_type+totaldata;
		
		sum=((sumation&0xFF)^0xFF)+1;
		
		if(sum==check_sum) *line_checksum=1;
		else *line_checksum=0;
	
		return byte_count;
}

uint8_t read_byte(FILE * file, uint8_t * char_to_put, int * total_chars_read)
{
	//Holds combined nibbles.
	uint8_t hexValue;
	//Get first nibble.
	*char_to_put = fgetc (file);
	clear_special_char(file, char_to_put, total_chars_read);
	//Put first nibble in.
	hexValue = (Ascii2Hex(*char_to_put));
	//Slide the nibble.
	hexValue = ((hexValue << 4) & 0xF0);
	//Put second nibble in.
	*char_to_put = fgetc (file);
	clear_special_char(file, char_to_put, total_chars_read);
	//Put the nibbles together.
	hexValue |= (Ascii2Hex(*char_to_put));
	//Return the byte.
	*total_chars_read += 2;

	return hexValue;
}

void clear_special_char(FILE * file, uint8_t * charToPut, int * totalCharsRead)
{
	//Removes CR, LF, ':'  
	while (*charToPut == '\n' || *charToPut == '\r' || *charToPut ==':'){
		(*charToPut = fgetc (file));
		*totalCharsRead++;
	}
}

uint8_t Ascii2Hex(uint8_t c)
{
	if (c >= '0' && c <= '9')
	{
		return (uint8_t)(c - '0');
	}
	if (c >= 'A' && c <= 'F')
	{
		return (uint8_t)(c - 'A' + 10);
	}
	if (c >= 'a' && c <= 'f')
	{
        return (uint8_t)(c - 'a' + 10);
	}

	return 0;  //shouldnt be reached
}



int line_counter(FILE * file_to_count)
{
	int counter = 0;
	char got_char;

	while(got_char != EOF)
	{
		got_char = fgetc(file_to_count);
		if (got_char == ':'){counter++;}
	}
	rewind(file_to_count);
	return counter;
}


