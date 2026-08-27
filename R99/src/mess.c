/*
** mess.c -- message functions
*/

/* Print a literal string without carriage return to console */
putls(str)
char *str;
{
	while(*str)
		putchar(*str++);
}

puts2(str1, str2)
char *str1;
char *str2;
{
  putls(str1);
  putls(str2);
  }

puts3(str1, str2, str3)
char *str1;
char *str2;
char *str3;
{
  putls(str1);
  putls(str2);
  putls(str3);
  }
cant(str)
char *str;
{
  error2(str, " - Can't Open");
  }
error2(str1, str2)
char *str1;
char *str2;
{
  putls(str1);
  error(str2);
  }
error(reason)
char *reason;
{
  putls(reason);
  exit(0);
  }

/*
** memcpy -- Small-C byte copy
**
** The TMS9900/TMS99105 is big endian, but memcpy copies bytes exactly
** as stored; no byte swapping is performed.
*/
memcpy(dest, src, count)
char *dest;
char *src;
unsigned count;
{
	while(count--)
		*dest++ = *src++;
}

/*
** memset -- Small-C byte fill
*/
memset(dest, value, count)
char *dest;
int value;
unsigned count;
{
	while(count--)
		*dest++ = value;
}

