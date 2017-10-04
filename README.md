# Programming an AVR from a PIC32 in circuit.

The idea here is having just the chips on a PCB, connected via SPI. This should also apply to using an Arduino board and its ISP connection. The code should be relatively easy to modify to different PICs and AVRs. The AVR used in this is the ATmega328p
# Step 1: Hex File Understanding
First a hex file is needed of the program which you want to program to the AVR. This can be done in the Arduino IDE by Sketch>Export compiled Binary. This will output 2 files in the project’s directory. I used the one without the bootloader. Once this is done the hex file needs to be converted into an array of values as the hex file itself is a compilation of ASCII characters rather than actual values, in that if you see a 2 in the hex file it is the character ‘2’ rather than the value 0b0010.
The ASCII can be converted using a relatively simple function;
However first an understanding of the Intel Hex Files is needed. 
Each line is seen as something like the following;
:100070002A0000000000250028002B0004040404CE
this is broken up into certain values.
The format of this line is :VVAAAATT[DDD…]CC
: - the colon shows the start of a new line
VV -The first 2 characters are the amount of data bytes shown on the line
AAAA -the next 4 are the address the data belongs to
TT - these 2 characters are the type of the following bytes. 00 is data, 01 is the end file marker, with 02 and 04 showing the line is an extended address line.
[DDD…] - the following characters are the data, there are as many bytes as dictated by BB
CC – the last 2 characters are the checksum. This is the 2’s compliment of all the previous bytes of data.
Example:
:0300700012FEA1DC
:03  0070  00 12FEA1 DC
 BB  AAAA TT  DDDDDD CC
The BB shows that there will be 3 bytes of data with the address showing as 0070

The check sum can be calculated by adding all of the previous bytes, XOR with 0xFF,add 1 and then AND with 0xFF as the Checksum is only one byte long.
So    - 0x03 + 0x00 + 0x70 + 0x00 + 0x12 + 0xFE + 0xA1 =0x224
XOR   - 0x224^0xFF =0x2DB
add 1 - 0x2DB+1=0x2DC
AND   - 0x2DC&0xFF=0xDC
With this understanding, a small program can be written to convert a hex file into binaries

# Step 2: Hex file Conversion

A program can be written and then compiled to do this for you. I wrote a .c file and then used minGW to compile it using the command prompt using cd to change directory to where the c file is. Then use     gcc -o hex2array hexToarray.c
to compile it giving you a exe to run.
There are other ways of achieving this so you can do this in a way that is comfortable for you. 
My program opens a menu that allows you to select a file and then convert it although not necessary. Note : to select an option in the menu press the number that corresponds to it and hit enter.
You could write your main with : int main(int argc,char *argv[]) instead which would allow you to run the program from the command line, allowing you to change the code so that you do not have to manually select the file and manually choose to convert it. 
The program outputs a file named AVRbins.h which contains the conversions in an array.

NOTE: Using file manipulation you can open the file with fopen and then proceed to read the bytes of data in to variables such as the BB AAAA etc. shown in step one. The main workings of this can be seen in the source code, function read_line().

# Step 3: Programming the AVR

The AVR I used was the ATmega328p and so the commands and timings used are from the datasheet of that device. If you are using this as a guideline for another AVR then check the datasheet of that device first. 
Timings taken from Table31-16 with instructions taken from Table 31-17. Found in the Serial Downloading section, section 31.8 for this device datasheet.
Note of Connections:
MOSI  to  MOSI
MISO  to  MISO
SCLK  to  SCLK
RESET to  RESET
V+    to  V+
GND   to  GND
Commands needed will be the erase, program enable, write flash.
(for a BYTE PROGRAMMING MODE AVR)
for ATmega328p:
erase cmd:       0xAC800000
program enable:  0xAC530000
write low  byte: 0x60%%%%$$
write high byte: 0x68%%%%$$
where %%%% is the hexadecimal address and the $$ is the hexadecimal byte of data.

Commands needed will be the erase, program enable, load page, write page.
(for a PAGE PROGRAMMING MODE AVR)
erase cmd:              0xAC800000
program enable:         0xAC530000
load to page low  byte: 0x40##%%$$
load to page high byte: 0x48##%%$$
write page:             0x4C%%%###
where ## is irrelevant, should be 00, %% is the hexadecimal address and the $$ is the hexadecimal byte of data.

NOTE: there are more commands than this, including read/write of eeprom and flash etc. can be found in the tables in the datasheet.
The program enable command must first be sent to the target. The target must then be erased, with the designated wait after the erase command is sent. 
NOTE: In the In-System-Programming Application Note it states that with the programming enable command sent AC 53 xx yy (putting 0’s in x and y) you will receive zz AC 53 xx(00AC5300). In my experience this is not the case, 00 00 53 00 is returned. Again this was in my system with a ATmeg328p and so your experience may differ. 
It is possible to check the Device ID with 0x30 00 00 00 with the ID being returned in the final byte (response[3] in the provided code)  1E or 00 is returned, if FF is returned the device did not initialise so a reset may be required. Using this will show if programming mode has been entered, regardless of a 00AC5300 or 00005300 response. 
erase command is then sent.
After the erase is complete you can begin writing the bytes of the AVR program, stored in the array.
I achieved this by incrementing through the addresses and adding the next byte from the array to that address.
This is not quite as simple as it seems because the intel files are 8bit word addresses where as the AVR chips are 16bit words, having a high byte and a low byte at each address.

For a AVR that uses byte programming mode;
In the for loop, int i is incremented by 1 each loop, to go through all elements of the array. The address for each element can be gained by shifting i to the right by 1, giving the same address for 2 elements at a time. So 0>>1 is 0. 1>>1 is 0. So the first 2 elements have the address 0. The command initialiser 0x60 or 0x68 puts it in either the low byte or the high byte. I achieved the distinction between the two by doing a bitwise operation on the LSB of i and shifting it to the left by 3. So if there is a 1 it gets shifted by 3 to become an 8. Therefore the command initialiser is :
0x60|((i &0x01)<<3).  So for the values of i 2&3;
2= 0b10; shifted right by 1 gives the address 1.
3= 0b11; shifted right by 1 gives the address 1.

for 2 : 0b10 &0x01 =0;  0<<3 is 0 therefore command 0x60|0x0 is : 0x60
for 3 : 0b11 &0x01 =1;  1<<3 is 8 therefore command 0x60|0x8 is : 0x68
distributing the 2 elements of the array(AVRarray[2] and AVRarray[3]) to the low byte and high byte respectively.
For an AVR that uses Page programming mode it is a little different. The shifting for high and low bytes still applies however the commands are 40 &48. The functionality is that you write to the ram of the chip first (in pages) then send the command to shift the buffer into the flash one page at a time. In my code I used an if statement to see if the byte just sent was the last of a page, if so then send the command to write the page. The address for each byte is the 6 bits to the right (LSBs) of the address meaning 00-3F is the range of 1 page. The page address can then be considered as the 8 bits to the left of those. This is used for the write page command 4C------.This can be seen in the code with addrMSB and addrLSB being the 2 bytes that make up the address for the page. The chip will ignore the 6 bits to the right (LSBs) in the address for the write page command. 

NOTE: page sizes may differ from chip to chip, I have not checked other chips for this detail. If you do have a page size difference the code can be altered to accommodate this by changing the write command if statement. Currently 0x7F is used. 0x7F being 127. The size of the page in bytes. In the datasheet for the ATmega328p it states it has a 64 word page size, 2 bytes per work thus 128 bytes per page meaning range for the first page is 0-127 thus 0x7F being the last byte. If there is 0x7F in the byte number it is a multiple of 128… -1.

# Step 4: Finalising
The code given will function if the program() function is called in your code but a few edits are required first. The pins will need to be changed at the top of the file to match those of your PIC. Also the delay may need editing for your devices internal clock speed etc. The right code for your chip needs to be uncommented, there is BYTE programming and PAGE programming, both are in the code. The provided file has the PAGE code uncommented.

