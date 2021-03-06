#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "gif.h"
#include "utils.h"

int printTable(byte * buffer, int count) {
	int i;

	printf("\n\t\t%-4s %-4s %-4s", "Red", "Green", "Blue");
	for(i = 0; i < count; i++) {
		if(i%3 == 0) printf("\n\t%u\t", (i/3));
		printf("0x%02X ", buffer[i]);
	}

	printf("\n");
	return 0;
}

int contReading(FILE *filePtr) {
	byte buffer;
	while (fread(&buffer, sizeof(byte), 1, filePtr) >= sizeof(byte)) {

		switch (buffer) {
		case 0x21:
			printExtension(filePtr);
			// Graphic Control Extension 0xF9
			// Comment Extension 0xFE
			// Plain Text 0x01
			// Application Extension 0xFF
			break;
		case 0x2C:
			printImageInfo(filePtr);
			break;
		case 0x3B:
			printf("Caught end-of-GIF tag (0x%02X)\n", buffer);
			break;
		default:
			break;
		}
	}
	return 0;
}

int printColorTable(FILE *filePtr, byte flags)
{	
	/*  As per GIF89A Specification, the Global COlor Table "contains a number of bytes equal to 
				3 x 2^(Size of Global Color Table+1)"
		Size is designated by the 3 least-significant bits of the Packed Fields byte (4th byte) in the  Logical Screen Descriptor

		Global Color Table Size
			  / \	
		1111 1111
		^
		Global Color Table Flag
	*/
	unsigned int tableSize = (pow((double)2, (double)((flags & 0x07) + 1))) * 3;
	byte *colorTable = (byte *)malloc(tableSize*sizeof(byte));

	fread(colorTable, tableSize*sizeof(byte), 1, filePtr);
	
	if(flags&0x08)
		printf("\n\t%-17s\t%u bytes:\n", "Sorted Global Color Table", tableSize);
	else
		printf("\n\t%-17s\t%u bytes:\n", "Global Color Table", tableSize);

	printTable(colorTable, tableSize*sizeof(byte));
	contReading(filePtr);

	return 0;
}


int printImageInfo(FILE *filePtr) {
	struct IMAGE_DESCRIPTOR imgDesc;
	ID_TAG imgTag;
	ID_FLAGS imgFlags;
	byte lzwSize;
	byte imgSize;
	byte *imgData;
	int i, blockCounter;

	fseek(filePtr, -1, SEEK_CUR);


	//IMAGE_DESCRIPTOR struct would have two different sizes of member, and is then byte-padded to align the data by the compiler.
	// To keep this from mucking up the data, we needed to split off the two smaller members, and thus needed to read them in separately.
	fread(&imgTag, sizeof(ID_TAG), 1, filePtr);
	fread(&imgDesc, sizeof(struct IMAGE_DESCRIPTOR), 1, filePtr);
	fread(&imgFlags, sizeof(ID_FLAGS), 1, filePtr);
	fread(&lzwSize, sizeof(byte), 1, filePtr);
	fread(&imgSize, sizeof(byte), 1, filePtr);

	printf("Image Descriptor (Tag 0x%02X)\n", imgTag);
	printf("\tCoords: (%u,%u)\n", imgDesc.left_coord, imgDesc.top_cord);
	printf("\tSize: %ux%u\n", imgDesc.width, imgDesc.height);
	printf("\tFlags: 0x%02X\n", imgFlags);
	printf("\tLZW Size: %u\n\n", lzwSize);
	printf("Image Sub-Blocks\n");

	blockCounter = 0;
	while (imgSize > 0) {
		blockCounter++;
		imgData = (byte *)malloc(imgSize);
		fread(imgData, sizeof(byte), imgSize, filePtr);
		printf("Block Size: %u\n", imgSize);
		printf("Block Data:\n");
		for (i = 0; i < imgSize; i++) {
			if (i % 10 == 0) printf("\n\t%u\t", (i / 10));
			printf("0x%02X ", imgData[i]);
		}
		printf("\n");
		fread(&imgSize, sizeof(byte), 1, filePtr);
	}
	printf("Found %i Image Sub-Blocks\n", blockCounter);
	return 0;
}


// Need to finish the Extensions
int printExtension(FILE *filePtr){
	byte buffer;
	struct GRAPHIC_CONTROL_EXTENSION gce;
	fread(&buffer, sizeof(byte),1,filePtr);
	fseek(filePtr, -2, SEEK_CUR);

	switch(buffer){
	case 0xF9:
		fread(&gce, sizeof(struct GRAPHIC_CONTROL_EXTENSION), 1, filePtr);
		printf("\n\nGraphic Control, %u\n", gce.flags);
		break;
	case 0xFE:
		break;
	case 0x01:
		break;
	case 0xFF:
		break;
	default:
		break;
	}
	return 0;
}

int printGIF(FILE *filePtr){
	struct GIF_HEADER gifHeader;
	byte flags;

	//fseek(filePtr, 0, SEEK_SET);
	rewind(filePtr);
	fread(&gifHeader, sizeof(struct GIF_HEADER),1,filePtr);

	flags = (byte) gifHeader.l_screen_desc[4];

	printf("GIF %c%c%c File\n\n", gifHeader.header[3], gifHeader.header[4], gifHeader.header[5]);

	Print_bArray("Header", gifHeader.header, 6);
	Print_bArray("L. Screen Descriptor", gifHeader.l_screen_desc, 7);
		
	printf("\t%-17s\t%u bit\n", "Color Resolution", ((flags & 0x70) >> 4)+1);

	if(gifHeader.l_screen_desc[4] & GCOLORTABLE) {
		printColorTable(filePtr,flags);
	}

	contReading(filePtr);

	return 0;
}