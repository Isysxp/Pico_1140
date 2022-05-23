#include <stdio.h>
//
#include "f_util.h"
#include "ff.h"
#include "pico/stdlib.h"
#include "rtc.h"
#include "string.h"
#include <string>
//
#include "hw_config.h"

int startup(int argc, char *argv);

using namespace std;
char *ReadLine(bool fullDuplex = true, char lineBreak = '\n');

string Fnames[20];
int SelFile;

// List contents of SDCard.

void ls(const char *dir) {
	char cwdbuf[FF_LFN_BUF] = { 0 };
	FRESULT fr; /* Return value */
	char const *p_dir;
	int ndx = 0;

	if (dir[0]) {
		p_dir = dir;
	}
	else {
		fr = f_getcwd(cwdbuf, sizeof cwdbuf);
		if (FR_OK != fr) {
			printf("f_getcwd error: %s (%d)\n", FRESULT_str(fr), fr);
			return;
		}
		p_dir = cwdbuf;
	}
	printf("Directory Listing: %s\n", p_dir);
	DIR dj; /* Directory object */
	FILINFO fno; /* File information */
	memset(&dj, 0, sizeof dj);
	memset(&fno, 0, sizeof fno);
	fr = f_findfirst(&dj, &fno, p_dir, "*");
	if (FR_OK != fr) {
		printf("f_findfirst error: %s (%d)\n", FRESULT_str(fr), fr);
		return;
	}
	while (fr == FR_OK && fno.fname[0]) {
		/* Repeat while an item is found */
	    /* Create a string that includes the file name, the file size and the
	     attributes string. */
		const char *pcWritableFile = "writable file",
		           *pcReadOnlyFile = "read only file",
		           *pcDirectory = "directory";
		const char *pcAttrib;
		/* Point pcAttrib to a string that describes the file. */
		if (fno.fattrib & AM_DIR) {
			pcAttrib = pcDirectory;
		}
		else if (fno.fattrib & AM_RDO) {
			pcAttrib = pcReadOnlyFile;
		}
		else {
			pcAttrib = pcWritableFile;
		}
		/* Create a string that includes the file name, the file size and the
		 attributes string. */
		Fnames[ndx] = fno.fname;
		printf("Number:%d\t%s\t[%s]\t[size=%llu]\n", ++ndx, fno.fname, pcAttrib, fno.fsize);

		fr = f_findnext(&dj, &fno); /* Search for next item */
	}
	f_closedir(&dj);
}
static void run_ls() {
	const char *arg1 = strtok(NULL, " ");
	if (!arg1) arg1 = "";
	ls(arg1);
}

int main() {
	char *bfr;

	set_sys_clock_khz(200000, true);
	stdio_flush();
	stdio_init_all();
	while (!stdio_usb_connected()) ;
	time_init();
	sd_card_t *pSD = sd_get_by_num(0);
	FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
	if (FR_OK != fr)
		panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
	
	run_ls();
	printf("Enter index of image:");
	bfr = ReadLine(true, '\r');
	sscanf(bfr, "%d", &SelFile);
	char arr[Fnames[SelFile - 1].length() + 1]; 
	strcpy(arr, Fnames[SelFile - 1].c_str());
	printf("\nBooting file:%s\r\n", arr);
	startup(1, arr);
}
