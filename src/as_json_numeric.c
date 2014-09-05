#include <R.h>
#include <Rdefines.h>

// Return the offset of the string without any trailing 0's that come after a
// decimal.
int strip_trailing_zeros(char* str, int len) {
  int len_tmp = len; // Tentative end of string, while trying to remove trailing 0's
  char tmp;

  while(1) {
    if (len_tmp == 0) {
      // We've backed all the way to the beginning and there was no decimal.
      break;
    }

    tmp = str[len_tmp-1];
    if (tmp == '0') {
      len_tmp--;
    } else if (tmp == '.') {
      len = len_tmp - 1;
      break;
    } else {
      len = len_tmp;
      break;
    }
  }

  // If the output was "-0", replace it with "0"
  if (len == 2 && str[0] == '-' && str[1] == '0') {
    str[0] = '0';
    len = 1;
  }

  str[len] = '\0';
  return len;
}


SEXP C_as_json_numeric_collapsed(SEXP x, SEXP digits, SEXP round, SEXP na) {
  if (!isReal(x))
    error("x must be a numeric vector.");
  if (!isInteger(digits) && !isReal(digits))
    error("digits must be a number.");
  if (!isString(round))
    error("round must be a string.");
  if (!isString(na))
    error("na must be a string.");

  double* nums = REAL(x);
  int len = length(x);

  // Set up the format string for snprintf
  char* format_specifier;
  if (strcmp(CHAR(asChar(round)), "decimal") == 0)
    format_specifier = "f";
  else if (strcmp(CHAR(asChar(round)), "signif") == 0)
    format_specifier = "g";
  else
    error("round must be either 'decimal' or 'signif'.");

  char format_str[20];
  snprintf(format_str, 20, "%%.%d%s", asInteger(digits), format_specifier);

  // How to handle NA's
  int na_null;
  if (strcmp(CHAR(asChar(na)), "string") == 0)
    na_null = 0;
  else if (strcmp(CHAR(asChar(na)), "null") == 0)
    na_null = 1;
  else
    error("na must be either 'string' or 'null'.");

  // Allocate a buffer for output
  int outlen = len * (asInteger(digits) + 4) + 100;
  char* out = (char*)malloc(outlen);
  
  int n = 0;    // Number of byes used so far
  out[n] = '[';
  n++;

  int inc;      // How many characters were added in last iteration
  double num;   // Current numeric value
  for (int i = 0; i < len; i++) {
    // Grow if necessary
    if (n + 40 > outlen) {
      outlen = outlen * 2;
      out = (char*) realloc(out, outlen);
    }

    num = nums[i];

    if (ISNA(num)) {
      if (na_null) {
        strcpy(out + n, "null");
        n += 4;
      } else {
        strcpy(out + n, "\"NA\"");
        n += 4;
      }
    } else if (ISNAN(num)) {
      if (na_null) {
        strcpy(out + n, "null");
        n += 4;
      } else {
        strcpy(out + n, "\"NaN\"");
        n += 5;
      }
    } else if (!R_FINITE(num)) {
      if (na_null) {
        strcpy(out + n, "null");
        n += 4;
      } else {
        if (nums[i] > 0) {
          strcpy(out + n, "\"Inf\"");
          n += 5;
        } else {
          strcpy(out + n, "\"-Inf\"");
          n += 6;
        }
      }
    } else {
      inc = snprintf((char*)(out + n), 40, format_str, num);
      // Remove trailing 0's, if they're after a decimal point.
      inc = strip_trailing_zeros((char*)(out + n), inc);
      n += inc;
    }

    out[n] = ',';
    n++;
  }

  out[n-1] = ']';
  out[n] = '\0';

  SEXP outstring = PROTECT(mkString(out));
  free(out);
  UNPROTECT(1);
  return outstring;
}