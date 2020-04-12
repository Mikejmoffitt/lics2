// vgm_cmp.c - VGM Compressor
//

#define _CRT_SECURE_NO_WARNINGS

#define DLL extern "C" __declspec (dllexport)

#include "vgmcmp.h"



#include <stdio.h>
#include <stdlib.h>
#include "stdbool.h"
#include <string.h>
#include <conio.h>
#include <windows.h>

//#include "zlib.h"

#include "VGMPlay.h"

static bool OpenVGMFile(const char* FileName);
static void WriteVGMFile(const char* FileName);
static void CompressVGMData(void);
bool GetNextChipCommand(void);
static void PrintMinSec(const unsigned long int SamplePos, char* TempStr);

void InitAllChips(void);
void FreeAllChips(void);
void SetChipSet(unsigned char ChipID);
bool GGStereo(unsigned char Data);
bool sn76496_write(unsigned char Command/*, unsigned char NextCmd*/);
bool ym2413_write(unsigned char Register, unsigned char Data);
bool ym2612_write(unsigned char Port, unsigned char Register, unsigned char Data);
bool ym2151_write(unsigned char Register, unsigned char Data);
bool segapcm_mem_write(unsigned short int Offset, unsigned char Data);
bool rf5c68_reg_write(unsigned char Register, unsigned char Data);
bool ym2203_write(unsigned char Register, unsigned char Data);
bool ym2608_write(unsigned char Port, unsigned char Register, unsigned char Data);
bool ym2610_write(unsigned char Port, unsigned char Register, unsigned char Data);
bool ym3812_write(unsigned char Register, unsigned char Data);
bool ym3526_write(unsigned char Register, unsigned char Data);
bool y8950_write(unsigned char Register, unsigned char Data);
bool ymf262_write(unsigned char Port, unsigned char Register, unsigned char Data);
bool ymz280b_write(unsigned char Register, unsigned char Data);
bool rf5c164_reg_write(unsigned char Register, unsigned char Data);
bool ay8910_write_reg(unsigned char Register, unsigned char Data);
bool ymf271_write(unsigned char Port, unsigned char Register, unsigned char Data);

VGM_HEADER VGMHead;
unsigned long int VGMDataLen;
unsigned char* VGMData;
unsigned long int VGMPos;
signed long int VGMSmplPos;
unsigned char* DstData;
unsigned long int DstDataLen;
char FileBase[0x100];
unsigned long int DataSizeA;
unsigned long int DataSizeB;

unsigned long int NxtCmdPos;
unsigned char NxtCmdCommand;
unsigned short int NxtCmdReg;
unsigned char NxtCmdVal;

bool JustTimerCmds;

/*int main(int argc, char* argv[])
{
	int argbase;
	int ErrVal;
	char FileName[0x100];
	unsigned long int SrcDataSize;
	unsigned short int PassNo;
	
	printf("VGM Compressor\n--------------\n\n");
	
	ErrVal = 0;
	JustTimerCmds = false;
	argbase = 0x01;
	if (argc >= argbase + 0x01)
	{
		if (! strcmp(argv[argbase + 0x00], "-justtmr"))
		{
			JustTimerCmds = true;
			argbase ++;
		}
	}
	
	printf("File Name:\t");
	if (argc <= argbase + 0x00)
	{
		gets(FileName);
	}
	else
	{
		strcpy(FileName, argv[argbase + 0x00]);
		printf("%s\n", FileName);
	}
	if (! strlen(FileName))
		return 0;
	
	if (! OpenVGMFile(FileName))
	{
		printf("Error opening the file!\n");
		ErrVal = 1;
		goto EndProgram;
	}
	printf("\n");
	
	PassNo = 0x00;
	do
	{
		printf("Pass #%hu ...\n", PassNo + 1);
		CompressVGMData();
		if (! PassNo)
			SrcDataSize = DataSizeA;
		printf("    Data Compression: %lu -> %lu (%.1f %%)\n",
				DataSizeA, DataSizeB, 100.0 * DataSizeB / (float)DataSizeA);
		if (DataSizeB < DataSizeA)
		{
			free(VGMData);
			VGMDataLen = DstDataLen;
			VGMData = DstData;
			DstDataLen = 0x00;
			DstData = NULL;
		}
		PassNo ++;
	} while(DataSizeB < DataSizeA);
	printf("Data Compression Total: %lu -> %lu (%.1f %%)\n",
			SrcDataSize, DataSizeB, 100.0 * DataSizeB / (float)SrcDataSize);
	
	if (DataSizeB < SrcDataSize)
	{
		if (argc > argbase + 0x01)
			strcpy(FileName, argv[argbase + 0x01]);
		else
			strcpy(FileName, "");
		if (! FileName[0x00])
		{
			strcpy(FileName, FileBase);
			strcat(FileName, "_optimized.vgm");
		}
		WriteVGMFile(FileName);
	}
	
	free(VGMData);
	free(DstData);
	
EndProgram:
	if (argv[0][1] == ':')
	{
		// Executed by Double-Clicking (or Drap and Drop)
		if (_kbhit())
			_getch();
		_getch();
	}
	
	return ErrVal;
}*/


unsigned char* vgmcmp_optimize(unsigned char* src,int &size)
{
	unsigned short int PassNo;
	unsigned long int CurPos;
	unsigned long int SrcDataSize;

	memcpy(&VGMHead,src,sizeof(VGMHead));

	// Header preperations
	if (VGMHead.lngVersion < 0x00000101)
	{
		VGMHead.lngRate = 0;
	}
	if (VGMHead.lngVersion < 0x00000110)
	{
		VGMHead.shtPSG_Feedback = 0x0000;
		VGMHead.bytPSG_SRWidth = 0x00;
		VGMHead.lngHzYM2612 = VGMHead.lngHzYM2413;
		VGMHead.lngHzYM2151 = VGMHead.lngHzYM2413;
	}
	if (VGMHead.lngVersion < 0x00000150)
	{
		VGMHead.lngDataOffset = 0x00000000;
	}
	if (VGMHead.lngVersion < 0x00000151)
	{
		VGMHead.lngHzSPCM = 0x0000;
		VGMHead.lngSPCMIntf = 0x00000000;
		// all others are zeroed by memset
	}
	// relative -> absolute addresses
	VGMHead.lngEOFOffset += 0x00000004;
	if (VGMHead.lngGD3Offset)
		VGMHead.lngGD3Offset += 0x00000014;
	if (VGMHead.lngLoopOffset)
		VGMHead.lngLoopOffset += 0x0000001C;
	if (! VGMHead.lngDataOffset)
		VGMHead.lngDataOffset = 0x0000000C;
	VGMHead.lngDataOffset += 0x00000034;
	
	CurPos = VGMHead.lngDataOffset;
	if (VGMHead.lngVersion < 0x00000151)
		CurPos = 0x40;
	memset((unsigned char*)&VGMHead + CurPos, 0x00, sizeof(VGM_HEADER) - CurPos);
	
	VGMDataLen = VGMHead.lngEOFOffset;
	VGMData=(unsigned char*)malloc(VGMDataLen);
	memcpy(VGMData,src,size);

	PassNo = 0x00;
	do
	{
		CompressVGMData();
		if (! PassNo) SrcDataSize = DataSizeA;

		if (DataSizeB < DataSizeA)
		{
			free(VGMData);
			VGMDataLen = DstDataLen;
			VGMData = DstData;
			DstDataLen = 0x00;
			DstData = NULL;
		}
		PassNo ++;
	} while(DataSizeB < DataSizeA);
	
	if (DataSizeB < SrcDataSize)
	{
		free(VGMData);
		size=DstDataLen;
		return DstData;
	}
	
	free(DstData);

	return VGMData;
}



const char* vgmcmp_about(void)
{
	return "VGM optimization code (vgmcmp) is from VGMTools by Valley Bell";
}



void vgmcmp_free(unsigned char *data)
{
	free(data);
}



/*static bool OpenVGMFile(const char* FileName)
{
	gzFile hFile;
	unsigned long int fccHeader;
	unsigned long int CurPos;
	char* TempPnt;
	
	hFile = gzopen(FileName, "rb");
	if (hFile == NULL)
		return false;
	
	gzseek(hFile, 0x00, SEEK_SET);
	gzread(hFile, &fccHeader, 0x04);
	if (fccHeader != FCC_VGM)
		goto OpenErr;
	
	gzseek(hFile, 0x00, SEEK_SET);
	gzread(hFile, &VGMHead, sizeof(VGM_HEADER));
	
	// Header preperations
	if (VGMHead.lngVersion < 0x00000101)
	{
		VGMHead.lngRate = 0;
	}
	if (VGMHead.lngVersion < 0x00000110)
	{
		VGMHead.shtPSG_Feedback = 0x0000;
		VGMHead.bytPSG_SRWidth = 0x00;
		VGMHead.lngHzYM2612 = VGMHead.lngHzYM2413;
		VGMHead.lngHzYM2151 = VGMHead.lngHzYM2413;
	}
	if (VGMHead.lngVersion < 0x00000150)
	{
		VGMHead.lngDataOffset = 0x00000000;
	}
	if (VGMHead.lngVersion < 0x00000151)
	{
		VGMHead.lngHzSPCM = 0x0000;
		VGMHead.lngSPCMIntf = 0x00000000;
		// all others are zeroed by memset
	}
	// relative -> absolute addresses
	VGMHead.lngEOFOffset += 0x00000004;
	if (VGMHead.lngGD3Offset)
		VGMHead.lngGD3Offset += 0x00000014;
	if (VGMHead.lngLoopOffset)
		VGMHead.lngLoopOffset += 0x0000001C;
	if (! VGMHead.lngDataOffset)
		VGMHead.lngDataOffset = 0x0000000C;
	VGMHead.lngDataOffset += 0x00000034;
	
	CurPos = VGMHead.lngDataOffset;
	if (VGMHead.lngVersion < 0x00000151)
		CurPos = 0x40;
	memset((unsigned char*)&VGMHead + CurPos, 0x00, sizeof(VGM_HEADER) - CurPos);
	
	// Read Data
	VGMDataLen = VGMHead.lngEOFOffset;
	VGMData = (unsigned char*)malloc(VGMDataLen);
	if (VGMData == NULL)
		goto OpenErr;
	gzseek(hFile, 0x00, SEEK_SET);
	gzread(hFile, VGMData, VGMDataLen);
	
	gzclose(hFile);
	
	strcpy(FileBase, FileName);
	TempPnt = strrchr(FileBase, '.');
	if (TempPnt != NULL)
		*TempPnt = 0x00;
	
	return true;

OpenErr:

	gzclose(hFile);
	return false;
}*/

/*static void WriteVGMFile(const char* FileName)
{
	FILE* hFile;
	
	hFile = fopen(FileName, "wb");
	fwrite(DstData, 0x01, DstDataLen, hFile);
	fclose(hFile);
	
	printf("File written.\n");
	
	return;
}*/

static void CompressVGMData(void)
{
	unsigned long int DstPos;
	unsigned long int CmdTimer;
	unsigned char ChipID;
	unsigned char Command;
	unsigned long int CmdDelay;
	unsigned long int AllDelay;
	unsigned char TempByt;
	unsigned short int TempSht;
	unsigned long int TempLng;
	unsigned long int ROMSize;
	unsigned long int DataStart;
	//unsigned long int DataLen;
	char TempStr[0x80];
	char MinSecStr[0x80];
	unsigned long int CmdLen;
	bool StopVGM;
	bool WriteEvent;
	unsigned long int NewLoopS;
	
	DstData = (unsigned char*)malloc(VGMDataLen + 0x100);
	AllDelay = 0;
	VGMPos = VGMHead.lngDataOffset;
	DstPos = VGMHead.lngDataOffset;
	VGMSmplPos = 0;
	NewLoopS = 0x00;
	memcpy(DstData, VGMData, VGMPos);	// Copy Header
	
	CmdTimer = 0;
	InitAllChips();
	StopVGM = false;
	while(VGMPos < VGMHead.lngEOFOffset)
	{
		if (VGMPos == VGMHead.lngLoopOffset)
			InitAllChips();	// Force resend of all commands after loopback
		CmdDelay = 0;
		CmdLen = 0x00;
		Command = VGMData[VGMPos + 0x00];
		WriteEvent = true;
		
		if (Command >= 0x70 && Command <= 0x8F)
		{
			switch(Command & 0xF0)
			{
			case 0x70:
				TempSht = (Command & 0x0F) + 0x01;
				VGMSmplPos += TempSht;
				CmdDelay = TempSht;
				WriteEvent = false;
				break;
			case 0x80:
				TempSht = Command & 0x0F;
				VGMSmplPos += TempSht;
				//CmdDelay = TempSht;
				break;
			}
			CmdLen = 0x01;
		}
		else
		{
			// Cheat Mode (to use 2 instances of 1 chip)
			ChipID = 0x00;
			switch(Command)
			{
			case 0x30:
				if (VGMHead.lngHzPSG & 0x40000000)
				{
					Command += 0x20;
					ChipID = 0x01;
				}
				break;
			case 0x3F:
				if (VGMHead.lngHzPSG & 0x40000000)
				{
					Command += 0x10;
					ChipID = 0x01;
				}
				break;
			case 0xA1:
				if (VGMHead.lngHzYM2413 & 0x40000000)
				{
					Command -= 0x50;
					ChipID = 0x01;
				}
				break;
			case 0xA2:
			case 0xA3:
				if (VGMHead.lngHzYM2612 & 0x40000000)
				{
					Command -= 0x50;
					ChipID = 0x01;
				}
				break;
			case 0xA4:
				if (VGMHead.lngHzYM2151 & 0x40000000)
				{
					Command -= 0x50;
					ChipID = 0x01;
				}
				break;
			case 0xA5:
				if (VGMHead.lngHzYM2203 & 0x40000000)
				{
					Command -= 0x50;
					ChipID = 0x01;
				}
				break;
			case 0xA6:
			case 0xA7:
				if (VGMHead.lngHzYM2608 & 0x40000000)
				{
					Command -= 0x50;
					ChipID = 0x01;
				}
				break;
			case 0xA8:
			case 0xA9:
				if (VGMHead.lngHzYM2610 & 0x40000000)
				{
					Command -= 0x50;
					ChipID = 0x01;
				}
				break;
			case 0xAA:
				if (VGMHead.lngHzYM3812 & 0x40000000)
				{
					Command -= 0x50;
					ChipID = 0x01;
				}
				break;
			case 0xAB:
				if (VGMHead.lngHzYM3526 & 0x40000000)
				{
					Command -= 0x50;
					ChipID = 0x01;
				}
				break;
			case 0xAC:
				if (VGMHead.lngHzY8950 & 0x40000000)
				{
					Command -= 0x50;
					ChipID = 0x01;
				}
				break;
			case 0xAE:
			case 0xAF:
				if (VGMHead.lngHzYMF262 & 0x40000000)
				{
					Command -= 0x50;
					ChipID = 0x01;
				}
				break;
			case 0xAD:
				if (VGMHead.lngHzYMZ280B & 0x40000000)
				{
					Command -= 0x50;
					ChipID = 0x01;
				}
				break;
			}
			SetChipSet(ChipID);
			
			NxtCmdPos = VGMPos;
			NxtCmdCommand = Command;
			switch(Command)
			{
			case 0x66:	// End Of File
				CmdLen = 0x01;
				StopVGM = true;
				break;
			case 0x62:	// 1/60s delay
				TempSht = 735;
				VGMSmplPos += TempSht;
				CmdDelay = TempSht;
				CmdLen = 0x01;
				WriteEvent = false;
				break;
			case 0x63:	// 1/50s delay
				TempSht = 882;
				VGMSmplPos += TempSht;
				CmdDelay = TempSht;
				CmdLen = 0x01;
				WriteEvent = false;
				break;
			case 0x61:	// xx Sample Delay
				memcpy(&TempSht, &VGMData[VGMPos + 0x01], 0x02);
				VGMSmplPos += TempSht;
				CmdDelay = TempSht;
				CmdLen = 0x03;
				WriteEvent = false;
				break;
			case 0x50:	// SN76496 write
				/*TempLng = GetNextChipCommand();
				TempByt = (TempLng) ? VGMData[TempLng + 0x01] : 0x00;
				WriteEvent = sn76496_write(VGMData[VGMPos + 0x01], TempByt);*/
				WriteEvent = sn76496_write(VGMData[VGMPos + 0x01]);
				CmdLen = 0x02;
				break;
			case 0x51:	// YM2413 write
				WriteEvent = ym2413_write(VGMData[VGMPos + 0x01], VGMData[VGMPos + 0x02]);
				CmdLen = 0x03;
				break;
			case 0x52:	// YM2612 write port 0
			case 0x53:	// YM2612 write port 1
				TempByt = Command & 0x01;
				WriteEvent = ym2612_write(TempByt, VGMData[VGMPos + 0x01],
											VGMData[VGMPos + 0x02]);
				CmdLen = 0x03;
				break;
			case 0x67:	// PCM Data Stream
				TempByt = VGMData[VGMPos + 0x02];
				memcpy(&TempLng, &VGMData[VGMPos + 0x03], 0x04);
				
				if (! (TempByt & 0x80))
				{
					switch(TempByt)
					{
					case 0x00:	// YM2612 PCM Data
						break;
					default:
						break;
					}
				}
				else
				{
					memcpy(&ROMSize, &VGMData[VGMPos + 0x07], 0x04);
					memcpy(&DataStart, &VGMData[VGMPos + 0x0B], 0x04);
					//DataLen = TempLng - 0x08;
					switch(TempByt)
					{
					case 0x80:	// SegaPCM ROM
						break;
					case 0x81:	// YM2608 DELTA-T ROM Image
						break;
					case 0x82:	// YM2610 ADPCM ROM Image
						break;
					case 0x83:	// YM2610 DELTA-T ROM Image
						break;
					default:
						break;
					}
				}
				CmdLen = 0x07 + TempLng;
				break;
			case 0xE0:	// Seek to PCM Data Bank Pos
				memcpy(&TempLng, &VGMData[VGMPos + 0x01], 0x04);
				CmdLen = 0x05;
				break;
			case 0x4F:	// GG Stereo
				WriteEvent = GGStereo(VGMData[VGMPos + 0x01]);
				CmdLen = 0x02;
				break;
			case 0x54:	// YM2151 write
				WriteEvent = ym2151_write(VGMData[VGMPos + 0x01], VGMData[VGMPos + 0x02]);
				CmdLen = 0x03;
				break;
			case 0xC0:	// Sega PCM memory write
				memcpy(&TempSht, &VGMData[VGMPos + 0x01], 0x02);
				WriteEvent = segapcm_mem_write(TempSht, VGMData[VGMPos + 0x03]);
				CmdLen = 0x04;
				break;
			case 0xB0:	// RF5C68 register write
				WriteEvent = rf5c68_reg_write(VGMData[VGMPos + 0x01], VGMData[VGMPos + 0x02]);
				CmdLen = 0x03;
				break;
			case 0xC1:	// RF5C68 memory write
				//memcpy(&TempSht, &VGMData[VGMPos + 0x01], 0x02);
				//WriteEvent = rf5c68_mem_write(TempSht, VGMData[VGMPos + 0x03]);
				WriteEvent = true;	// OptVgmRF works a lot better this way
				CmdLen = 0x04;
				break;
			case 0x55:	// YM2203
				WriteEvent = ym2203_write(VGMData[VGMPos + 0x01], VGMData[VGMPos + 0x02]);
				CmdLen = 0x03;
				break;
			case 0x56:	// YM2608 write port 0
			case 0x57:	// YM2608 write port 1
				TempByt = Command & 0x01;
				WriteEvent = ym2608_write(TempByt, VGMData[VGMPos + 0x01],
											VGMData[VGMPos + 0x02]);
				CmdLen = 0x03;
				break;
			case 0x58:	// YM2610 write port 0
			case 0x59:	// YM2610 write port 1
				TempByt = Command & 0x01;
				WriteEvent = ym2610_write(TempByt, VGMData[VGMPos + 0x01],
											VGMData[VGMPos + 0x02]);
				CmdLen = 0x03;
				break;
			case 0x5A:	// YM3812 write
				WriteEvent = ym3812_write(VGMData[VGMPos + 0x01], VGMData[VGMPos + 0x02]);
				CmdLen = 0x03;
				break;
			case 0x5B:	// YM3526 write
				WriteEvent = ym3526_write(VGMData[VGMPos + 0x01], VGMData[VGMPos + 0x02]);
				CmdLen = 0x03;
				break;
			case 0x5C:	// Y8950 write
				WriteEvent = y8950_write(VGMData[VGMPos + 0x01], VGMData[VGMPos + 0x02]);
				CmdLen = 0x03;
				break;
			case 0x5E:	// YMF262 write port 0
			case 0x5F:	// YMF262 write port 1
				TempByt = Command & 0x01;
				WriteEvent = ymf262_write(TempByt, VGMData[VGMPos + 0x01],
											VGMData[VGMPos + 0x02]);
				CmdLen = 0x03;
				break;
			case 0x5D:	// YMZ280B write
				WriteEvent = ymz280b_write(VGMData[VGMPos + 0x01], VGMData[VGMPos + 0x02]);
				CmdLen = 0x03;
				break;
			case 0xD0:	// YMF278B write
				TempByt = VGMData[VGMPos + 0x01];
				if (TempByt <= 0x01)
					WriteEvent = ymf262_write(TempByt, VGMData[VGMPos + 0x02],
												VGMData[VGMPos + 0x03]);
				else
					WriteEvent = true;
				CmdLen = 0x04;
				break;
			case 0xD1:	// YMF271 write
				WriteEvent = ymf271_write(VGMData[VGMPos + 0x01], VGMData[VGMPos + 0x02],
											VGMData[VGMPos + 0x03]);
				CmdLen = 0x04;
				break;
			case 0xB1:	// RF5C164 register write
				WriteEvent = rf5c164_reg_write(VGMData[VGMPos + 0x01], VGMData[VGMPos + 0x02]);
				CmdLen = 0x03;
				break;
			case 0xC2:	// RF5C164 memory write
				WriteEvent = true;	// OptVgmRF works a lot better this way
				CmdLen = 0x04;
				break;
			case 0x68:	// PCM RAM write
				WriteEvent = true;
				CmdLen = 0x0C;
				break;
			case 0xA0:	// AY8910 register write
				WriteEvent = ay8910_write_reg(VGMData[VGMPos + 0x01], VGMData[VGMPos + 0x02]);
				CmdLen = 0x03;
				break;
			case 0x90:	// DAC Ctrl: Setup Chip
				CmdLen = 0x05;
				break;
			case 0x91:	// DAC Ctrl: Set Data
				CmdLen = 0x05;
				break;
			case 0x92:	// DAC Ctrl: Set Freq
				CmdLen = 0x06;
				break;
			case 0x93:	// DAC Ctrl: Play from Start Pos
				CmdLen = 0x0B;
				break;
			case 0x94:	// DAC Ctrl: Stop immediately
				CmdLen = 0x02;
				break;
			case 0x95:	// DAC Ctrl: Play Block (small)
				CmdLen = 0x05;
				break;
			default:
				switch(Command & 0xF0)
				{
				case 0x30:
				case 0x40:
					CmdLen = 0x02;
					break;
				case 0x50:
				case 0xA0:
				case 0xB0:
					CmdLen = 0x03;
					break;
				case 0xC0:
				case 0xD0:
					CmdLen = 0x04;
					break;
				case 0xE0:
				case 0xF0:
					CmdLen = 0x05;
					break;
				default:
					printf("Unknown Command: %hX\n", Command);
					CmdLen = 0x01;
					//StopVGM = true;
					break;
				}
				break;
			}
		}
		
		if (WriteEvent || VGMPos == VGMHead.lngLoopOffset)
		{
			if (VGMPos != VGMHead.lngLoopOffset)
			{
				AllDelay += CmdDelay;
				CmdDelay = 0x00;
			}
			while(AllDelay)
			{
				if (AllDelay <= 0xFFFF)
					TempSht = (unsigned short int)AllDelay;
				else
					TempSht = 0xFFFF;
				
				if (! TempSht)
				{
					// don't do anything - I just want to be safe
				}
				if (TempSht <= 0x10)
				{
					DstData[DstPos] = 0x70 | (TempSht - 0x01);
					DstPos ++;
				}
				else if (TempSht <= 0x20)
				{
					DstData[DstPos] = 0x7F;
					DstPos ++;
					DstData[DstPos] = 0x70 | (TempSht - 0x11);
					DstPos ++;
				}
				else if ((TempSht >=  735 && TempSht <=  751) || TempSht == 1470)
				{
					TempLng = TempSht;
					while(TempLng >= 735)
					{
						DstData[DstPos] = 0x62;
						DstPos ++;
						TempLng -= 735;
					}
					TempSht -= (unsigned short int)TempLng;
				}
				else if ((TempSht >=  882 && TempSht <=  898) || TempSht == 1764)
				{
					TempLng = TempSht;
					while(TempLng >= 882)
					{
						DstData[DstPos] = 0x63;
						DstPos ++;
						TempLng -= 882;
					}
					TempSht -= (unsigned short int)TempLng;
				}
				else if (TempSht == 1617)
				{
					DstData[DstPos] = 0x63;
					DstPos ++;
					DstData[DstPos] = 0x62;
					DstPos ++;
				}
				else
				{
					DstData[DstPos + 0x00] = 0x61;
					memcpy(&DstData[DstPos + 0x01], &TempSht, 0x02);
					DstPos += 0x03;
				}
				AllDelay -= TempSht;
			}
			AllDelay = CmdDelay;
			CmdDelay = 0x00;
			
			if (VGMPos == VGMHead.lngLoopOffset)
				NewLoopS = DstPos;
			
			if (WriteEvent)
			{
				memcpy(&DstData[DstPos], &VGMData[VGMPos], CmdLen);
				DstPos += CmdLen;
			}
		}
		else
		{
			AllDelay += CmdDelay;
		}
		VGMPos += CmdLen;
		if (StopVGM)
			break;
		
		if (CmdTimer < GetTickCount())
		{
			PrintMinSec(VGMSmplPos, MinSecStr);
			PrintMinSec(VGMHead.lngTotalSamples, TempStr);
			TempLng = VGMPos - VGMHead.lngDataOffset;
			ROMSize = VGMHead.lngEOFOffset - VGMHead.lngDataOffset;
			printf("%04.3f %% - %s / %s (%08lX / %08lX) ...\r", (float)TempLng / ROMSize * 100,
					MinSecStr, TempStr, VGMPos, VGMHead.lngEOFOffset);
			CmdTimer = GetTickCount() + 200;
		}
	}
	DataSizeA = VGMPos - VGMHead.lngDataOffset;
	DataSizeB = DstPos - VGMHead.lngDataOffset;
	if (VGMHead.lngLoopOffset)
	{
		VGMHead.lngLoopOffset = NewLoopS;
		if (! NewLoopS)
			printf("Error! Failed to relocate Loop Point!\n");
		else
			NewLoopS -= 0x1C;
		memcpy(&DstData[0x1C], &NewLoopS, 0x04);
	}
	printf("\t\t\t\t\t\t\t\t\r");
	
	if (VGMHead.lngGD3Offset)
	{
		VGMPos = VGMHead.lngGD3Offset;
		memcpy(&TempLng, &VGMData[VGMPos + 0x00], 0x04);
		if (TempLng == FCC_GD3)
		{
			memcpy(&CmdLen, &VGMData[VGMPos + 0x08], 0x04);
			CmdLen += 0x0C;
			
			VGMHead.lngGD3Offset = DstPos;
			TempLng = DstPos - 0x14;
			memcpy(&DstData[0x14], &TempLng, 0x04);
			memcpy(&DstData[DstPos], &VGMData[VGMPos], CmdLen);
			DstPos += CmdLen;
		}
	}
	DstDataLen = DstPos;
	TempLng = DstDataLen - 0x04;
	memcpy(&DstData[0x04], &TempLng, 0x04);
	
	FreeAllChips();
	
	return;
}

bool GetNextChipCommand(void)
{
	unsigned long int CurPos;
	unsigned char Command;
	//unsigned char TempByt;
	//unsigned short int TempSht;
	unsigned long int TempLng;
	unsigned long int CmdLen;
	bool ReturnData;
	bool CmdIsPort;
	bool FirstCmd;
	
	CurPos = NxtCmdPos;
	FirstCmd = true;
	while(CurPos < VGMHead.lngEOFOffset)
	{
		CmdLen = 0x00;
		Command = VGMData[CurPos + 0x00];
		
		if (Command >= 0x70 && Command <= 0x8F)
		{
			CmdLen = 0x01;
		}
		else
		{
			switch(Command)
			{
			case 0x66:	// End Of File
				NxtCmdPos = CurPos;
				CmdLen = 0x01;
				return false;
				//break;
			case 0x62:	// 1/60s delay
				CmdLen = 0x01;
				break;
			case 0x63:	// 1/50s delay
				CmdLen = 0x01;
				break;
			case 0x61:	// xx Sample Delay
				CmdLen = 0x03;
				break;
			case 0x67:	// PCM Data Stream
				memcpy(&TempLng, &VGMData[CurPos + 0x03], 0x04);
				CmdLen = 0x07 + TempLng;
				break;
			case 0x68:	// PCM RAM write
				CmdLen = 0x0C;
				break;
			case 0x50:	// SN76496 write
			case 0x30:
				if (NxtCmdCommand == 0x50)
					ReturnData = true;
				CmdLen = 0x02;
				break;
			case 0x51:	// YM2413 write
			case 0xA1:
				if (NxtCmdCommand == 0x51)
					ReturnData = true;
				CmdLen = 0x03;
				break;
			case 0x52:	// YM2612 write port 0
			case 0x53:	// YM2612 write port 1
			case 0xA2:
			case 0xA3:
				if ((NxtCmdCommand & ~0x01) == 0x52)
				{
					ReturnData = true;
					CmdIsPort = true;
				}
				CmdLen = 0x03;
				break;
			case 0xE0:	// Seek to PCM Data Bank Pos
				CmdLen = 0x05;
				break;
			case 0x4F:	// GG Stereo
				if (NxtCmdCommand == 0x4F)
					ReturnData = true;
				CmdLen = 0x02;
				break;
			case 0x54:	// YM2151 write
			case 0xA4:
				if (NxtCmdCommand == 0x54)
					ReturnData = true;
				CmdLen = 0x03;
				break;
			case 0xC0:	// Sega PCM memory write
				if (NxtCmdCommand == 0xC0)
					ReturnData = true;
				CmdLen = 0x04;
				break;
			case 0xB0:	// RF5C68 register write
				if (NxtCmdCommand == 0xB0)
					ReturnData = true;
				CmdLen = 0x03;
				break;
			case 0xC1:	// RF5C68 memory write
				if (NxtCmdCommand == 0xC1)
					ReturnData = true;
				CmdLen = 0x04;
				break;
			case 0x55:	// YM2203
			case 0xA5:
				if (NxtCmdCommand == 0x55)
					ReturnData = true;
				CmdLen = 0x03;
				break;
			case 0x56:	// YM2608 write port 0
			case 0x57:	// YM2608 write port 1
			case 0xA6:
			case 0xA7:
				if ((NxtCmdCommand & ~0x01) == 0x56)
				{
					ReturnData = true;
					CmdIsPort = true;
				}
				CmdLen = 0x03;
				break;
			case 0x58:	// YM2610 write port 0
			case 0x59:	// YM2610 write port 1
			case 0xA8:
			case 0xA9:
				if ((NxtCmdCommand & ~0x01) == 0x58)
				{
					ReturnData = true;
					CmdIsPort = true;
				}
				CmdLen = 0x03;
				break;
			case 0x5A:	// YM3812 write
			case 0xAA:
				if (NxtCmdCommand == 0x5A)
					ReturnData = true;
				CmdLen = 0x03;
				break;
			case 0x5B:	// YM3526 write
			case 0xAB:
				if (NxtCmdCommand == 0x5B)
					ReturnData = true;
				CmdLen = 0x03;
				break;
			case 0x5C:	// Y8950 write
			case 0xAC:
				if (NxtCmdCommand == 0x5C)
					ReturnData = true;
				CmdLen = 0x03;
				break;
			case 0x5E:	// YMF262 write port 0
			case 0x5F:	// YMF262 write port 1
			case 0xAE:
			case 0xAF:
				if ((NxtCmdCommand & ~0x01) == 0x5E)
				{
					ReturnData = true;
					CmdIsPort = true;
				}
				CmdLen = 0x03;
				break;
			case 0x5D:	// YMZ280B write
			case 0xAD:
				if (NxtCmdCommand == 0x5D)
					ReturnData = true;
				CmdLen = 0x03;
				break;
			case 0xB1:	// RF5C164 register write
				if (NxtCmdCommand == 0xB1)
					ReturnData = true;
				CmdLen = 0x03;
				break;
			case 0xC2:	// RF5C164 memory write
				if (NxtCmdCommand == 0xC2)
					ReturnData = true;
				CmdLen = 0x04;
				break;
			case 0xA0:	// AY8910 register write
				if (NxtCmdCommand == 0xA0)
					ReturnData = true;
				CmdLen = 0x03;
				break;
			case 0x90:	// DAC Ctrl: Setup Chip
				CmdLen = 0x05;
				break;
			case 0x91:	// DAC Ctrl: Set Data
				CmdLen = 0x05;
				break;
			case 0x92:	// DAC Ctrl: Set Freq
				CmdLen = 0x06;
				break;
			case 0x93:	// DAC Ctrl: Play from Start Pos
				CmdLen = 0x0B;
				break;
			case 0x94:	// DAC Ctrl: Stop immediately
				CmdLen = 0x02;
				break;
			case 0x95:	// DAC Ctrl: Play Block (small)
				CmdLen = 0x05;
				break;
			default:
				switch(Command & 0xF0)
				{
				case 0x30:
				case 0x40:
					CmdLen = 0x02;
					break;
				case 0x50:
				case 0xA0:
				case 0xB0:
					CmdLen = 0x03;
					break;
				case 0xC0:
				case 0xD0:
					CmdLen = 0x04;
					break;
				case 0xE0:
				case 0xF0:
					CmdLen = 0x05;
					break;
				default:
					CmdLen = 0x01;
					break;
				}
				break;
			}
		}
		if (FirstCmd)
		{
			// the first command is already read and must be skipped
			FirstCmd = false;
			ReturnData = false;
			CmdIsPort = false;
		}
		if (ReturnData)
		{
			switch(CmdLen)
			{
			case 0x02:
				NxtCmdReg = 0x00;
				NxtCmdVal = VGMData[CurPos + 0x01];
				break;
			case 0x03:
				NxtCmdReg = VGMData[CurPos + 0x01];
				if (CmdIsPort)
					NxtCmdReg |= (Command & 0x01) << 8;
				NxtCmdVal = VGMData[CurPos + 0x02];
				break;
			case 0x04:
				NxtCmdReg = (VGMData[CurPos + 0x01] << 8) | (VGMData[CurPos + 0x01] << 0);
				NxtCmdVal = VGMData[CurPos + 0x03];
				break;
			default:
				NxtCmdReg = 0x00;
				NxtCmdVal = 0x00;
				break;
			}
			NxtCmdPos = CurPos;	// support consecutive searches
			return true;
		}
		
		CurPos += CmdLen;
	}
	
	return false;
}

static void PrintMinSec(const unsigned long int SamplePos, char* TempStr)
{
	float TimeSec;
	unsigned short int TimeMin;
	
	TimeSec = (float)SamplePos / (float)44100.0;
	TimeMin = (unsigned short int)TimeSec / 60;
	TimeSec -= TimeMin * 60;
	sprintf(TempStr, "%02hu:%05.2f", TimeMin, TimeSec);
	
	return;
}
