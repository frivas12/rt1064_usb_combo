/**
 * @file helper.c
 *
 * @brief Functions for ???
 *
 */
#include <asf.h>
#include "helper.h"
#include<stdio.h>
#include<math.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

/// @brief The number that, when added to an uppercase letter, will make it lower case.
static const char LOWER_CASE_OFFSET = 'a' - 'A';

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/
// C program for implementation of ftoa()
// reverses a string 'str' of length 'len'
void reverse(char *str, int len)
{
	int i = 0, j = len - 1, temp;
	while (i < j)
	{
		temp = str[i];
		str[i] = str[j];
		str[j] = temp;
		i++;
		j--;
	}
}

// Converts a given integer x to string str[].  d is the number
// of digits required in output. If d is more than the number
// of digits in x, then 0s are added at the beginning.
int intToStr(int x, char str[], int d)
{
	int i = 0;
	while (x)
	{
		str[i++] = (x % 10) + '0';
		x = x / 10;
	}

	// If number of digits required is more, then
	// add 0s at the beginning
	while (i < d)
		str[i++] = '0';

	reverse(str, i);
	str[i] = '\0';
	return i;
}

// Converts a floating point number to string.
void ftoa(float n, char *res, int afterpoint)
{
	// Extract integer part
	int ipart = (int) n;

	// Extract floating part
	float fpart = n - (float) ipart;

	// convert integer part to string
	int i = intToStr(ipart, res, 0);

	// check for display option after point
	if (afterpoint != 0)
	{
		res[i] = '.';  // add dot

		// Get the value of fraction part upto given no.
		// of points after dot. The third parameter is needed
		// to handle cases like 233.007
		fpart = fpart * pow(10, afterpoint);

		intToStr((int) fpart, res + i + 1, afterpoint);
	}
}

double atof_m(char *num)
{
	if (!num || !*num)
		return 0;
	double integerPart = 0;
	double fractionPart = 0;
	int divisorForFraction = 1;
	int sign = 1;
	bool inFraction = false;

	/*Take care of +/- sign*/
	if (*num == '-')
	{
		++num;
		sign = -1;
	}
	else if (*num == '+')
	{
		++num;
	}
	while (*num != '\0')
	{
		if (*num >= '0' && *num <= '9')
		{
			if (inFraction)
			{
				/*See how are we converting a character to integer*/
				fractionPart = fractionPart * 10 + (*num - '0');
				divisorForFraction *= 10;
			}
			else
			{
				integerPart = integerPart * 10 + (*num - '0');
			}
		}
		else if (*num == '.')
		{
			if (inFraction)
				return sign * (integerPart + fractionPart / divisorForFraction);
			else
				inFraction = true;
		}
		else
		{
			return sign * (integerPart + fractionPart / divisorForFraction);
		}
		++num;
	}
	return sign * (integerPart + fractionPart / divisorForFraction);
}

void rev_memcpy(void *dest, void *src, size_t n)
{
	// Typecast src and dest addresses to (char *)
	char *csrc = (char*) src;
	char *cdest = (char*) dest;

	for (uint8_t index = 0; index < n; index++)
	{
		cdest[index + n - 1] = csrc[index];
	}
}

void cstring_to_lowercase(char * c_str)
{
	while (*c_str != '\0')
	{
		if (*c_str >= 'A' && *c_str <= 'Z')
		{
			*c_str += LOWER_CASE_OFFSET;
		}
		++c_str;
	}
}

void cstring_to_uppercase(char * c_str)
{
	while (*c_str != '\0')
	{
		if (*c_str >= 'a' && *c_str <= 'z')
		{
			*c_str -= LOWER_CASE_OFFSET;
		}
		++c_str;
	}
}

