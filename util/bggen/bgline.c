#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	if (argc <= 2)
	{
		printf("Usage: %s wordc [word0 word1 word2]\n", argv[0]);
		return 0;
	}

	const int words_per_line = atoi(argv[1]);
	if (words_per_line < 0)
	{
		fprintf(stderr, "Words per line %d is invalid\n", words_per_line);
		return 1;
	}

	int arg_idx = 2;
	for (int i = 0; i < words_per_line; i++)
	{
		unsigned int val = strtoul(argv[arg_idx], NULL, 0);
		const uint8_t word_high = (val & 0x0000FF00) >> 8;
		const uint8_t word_low = val & 0x000000FF;
		fputc(word_high, stdout);
		fputc(word_low, stdout);
		arg_idx++;
		if (arg_idx >= argc) arg_idx = 2;
	}
}
