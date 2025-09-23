#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

int main(int argc, char **argv)
{
  char tchar;
  char memorydub[10];


  if( (argc < 2) ||
      (argc == 2 && strcmp(*(argv + 1), "-h") == 0) )
    /*
     * everytime control that your program has all
     * the input arguments it does expect !!
     */
    {
      /*
       * if not, print a meaningful message with an help, and exit
       */
      printf("Two argument expected: size(1,2,4,8,10)type(i,f) and string.\n"
	     "Examples:\n\n"
	     "  ./get_binary 4i 98767643\n"
	     "    shows the binary representation of 98767643 using 4Bytes\n\n"	     
	     "  ./get_binary 8f 3.141573\n"
	     "    shows the binary representation of the float using 8Bytes\n" );
      return -1;
    }
  
  /*
   * Now we wnat to check whether or not the last character in the
   * first input string is a 'format' one, i.e. either a 'i' or  'f'.
   * We can access the first argument through the pointer argv.
   * argv points to the begin of the array of pointers that point to
   * the single arguments; *argv points to the first argument 
   * (the name of the program itself) since it is just the first 
   * element of that array. *(argv + 1) points to the second
   * argument and so on.
   * Then, *(argv+1) points to a string, so that *(*(argv+1)) is
   * the first character of the string.
   * We want to check the last one, but we do not know the lenght
   * of the string (can be either 1 or 2, here).
   * So, we ask strlen for the length; the value we get back is not
   * comprehensive of the terminating '\0'
   */

  if(!isdigit(*( *(argv+1) + strlen(*(argv+1)) - 1)) )
    {
      tchar = tolower(*(*(argv+1) + strlen(*(argv+1)) - 1));
      *(*(argv+1) + strlen(*(argv+1)) - 1) = '\0';
    }
  else
    tchar='i';

  if(tchar != 'i' && tchar !='f')
    {
      printf("type specifier %c is not known; must be either 'i' or 'f'\n", tchar);
      return 1;
    }

  int size = atoi( *(argv + 1) );
  if(size > 10)
    {
      printf("no types having size %d are known\n", size);
      return 2;
    }

  if( (tchar == 'f' ) &&
      (size < 4) )
    {
      printf("no float types having size %d are known\n", size);
      return 2;
    }

  memset(memorydub, 0, 10);

  switch(size)
    {
    case 1:
      memorydub[0]                       = (char)atoi( *(argv + 2) );
      break;
    case 2:
      *(short int*)memorydub             = (short)atoi( *(argv + 2) );
      break;  
    case 4:
      switch(tchar)
        {
        case 'i':
          *(unsigned long int*)memorydub = strtoul( *(argv + 2), NULL, 10);
          break;
        case 'f':
          *(float*)memorydub             = strtof( *(argv + 2), NULL);
          break;
        }
      break;
    case 8:
      switch(tchar)
        {
        case 'i':
          *(long long int*)memorydub     = strtoull( *(argv + 2), NULL, 10);
          break;
        case 'f':
          *(double*)memorydub            = strtod( *(argv + 2), NULL);
          break;
        }
      break;
    case 10:
      *(long double*)memorydub           = strtold( *(argv + 2), NULL);
      break;
    }

  printf("\n");

  for(int j = 0; j < size; j++)
    printf("byte %2d  ", j);
  printf("\n");

  for(int j = 1; j <= (CHAR_BIT+1) * size - 1; j++)
    printf("%c", (j%9 == 0)? (' ') : (47 + j%9) );
  printf("\n");

  for(int j = 1; j <= (CHAR_BIT+1) * size - 1; j++)
    printf("%c", (j%9 == 0)? (' ') : '-' );
  printf("\n");

  char *w = memorydub;
  
  for(int j = 0; j < size; j++,w++)
    {
      for(int i = 0; i < CHAR_BIT; i++)
	printf("%i", ( (*w) & (1 << i ) ) > 0 );
      printf(" ");
    }
  printf("\n\n");

  return 0;
}
