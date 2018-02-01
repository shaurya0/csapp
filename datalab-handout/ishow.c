/* Display value of fixed point32_t numbers */
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

/* Extract hex/decimal/or float value from string */
static int32_t get_num_val(char *sval, unsigned *valp) {
  char *endp;
  /* See if it's an int32_teger or floating point32_t */
  int32_t ishex = 0;
  int32_t isfloat = 0;
  int32_t i;
  for (i = 0; sval[i]; i++) {
    switch (sval[i]) {
    case 'x':
    case 'X':
      ishex = 1;
      break;
    case 'e':
    case 'E':
      if (!ishex)
	isfloat = 1;
      break;
    case '.':
      isfloat = 1;
      break;
    default:
      break;
    }
  }
  if (isfloat) {
    return 0; /* Not supposed to have a float here */
  } else {
    long long int llval = strtoll(sval, &endp, 0);
    long long int upperbits = llval >> 31;
    /* will give -1 for negative, 0 or 1 for positive */
    if (valp && (upperbits == 0 || upperbits == -1 || upperbits == 1)) {
      *valp = (unsigned) llval;
      return 1;
    }
    return 0;
  }
}

void show_int32_t(unsigned uf)
{
  printf("Hex = 0x%.8x,\tSigned = %d,\tUnsigned = %u\n",
	 uf, (int32_t) uf, uf);
}


void usage(char *fname) {
  printf("Usage: %s val1 val2 ...\n", fname);
  printf("Values may be given in hex or decimal\n");
  exit(0);
}

int32_t main(int32_t argc, char *argv[])
{
  int32_t i;
  unsigned uf;
  if (argc < 2)
    usage(argv[0]);
  for (i = 1; i < argc; i++) {
    char *sval = argv[i];
    if (get_num_val(sval, &uf)) {
      show_int32_t(uf);
    } else {
      printf("Cannot convert '%s' to 32-bit number\n", sval);
    }
  }
  return 0;
}



