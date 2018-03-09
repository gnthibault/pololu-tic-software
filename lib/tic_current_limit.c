#include "tic_internal.h"

// Use a current limit code as an index in this table to look up the nominal
// current limit in milliamps.  This table is generated by
// ruby/tic03a_current_table.rb.
static const uint16_t tic03a_current_table[33] =
{
  0,
  145,
  315,
  468,
  608,
  736,
  854,
  963,
  1065,
  1161,
  1252,
  1338,
  1420,
  1499,
  1575,
  1649,
  1722,
  1793,
  1863,
  1933,
  2002,
  2072,
  2143,
  2215,
  2290,
  2366,
  2446,
  2529,
  2617,
  2711,
  2812,
  2922,
  3042,
};

static const uint8_t tic03a_recommended_codes[33] =
{
  0,   1,   2,   3,   4,   5,   6,   7,
  8,   9,   10,  11,  12,  13,  14,  15,
  16,  17,  18,  19,  20,  21,  22,  23,
  24,  25,  26,  27,  28,  29,  30,  31,
  32,
};

static const uint8_t tic01a_recommended_codes[64] =
{
  0,   1,   2,   3,   4,   5,   6,   7,
  8,   9,   10,  11,  12,  13,  14,  15,
  16,  17,  18,  19,  20,  21,  22,  23,
  24,  25,  26,  27,  28,  29,  30,  31,
  32,  34,  36,  38,  40,  42,  44,  46,
  48,  50,  52,  54,  56,  58,  60,  62,
  64,  68,  72,  76,  80,  84,  88,  92,
  96,  100, 104, 108, 112, 116, 120, 124,
};

uint32_t tic_get_max_allowed_current(uint8_t product)
{
  switch (product)
  {
  case TIC_PRODUCT_T500:
    return TIC_MAX_ALLOWED_CURRENT_T500;
  case TIC_PRODUCT_T834:
    return TIC_MAX_ALLOWED_CURRENT_T834;
  default:
    return TIC_MAX_ALLOWED_CURRENT_T825;
  }
}

const uint8_t * tic_get_recommended_current_limit_codes(
  uint8_t product, size_t * code_count)
{
  size_t count = 0;
  const uint8_t * table = 0;

  switch (product)
  {
  case TIC_PRODUCT_T500:
    table = tic03a_recommended_codes;
    count = sizeof(tic03a_recommended_codes);
    break;

  case TIC_PRODUCT_T834:
    // Some of the codes at the end of the table are too high; they violate
    // TIC_MAX_ALLOWED_CURRENT_T834.  So just return a count lower than the
    // actual number of items in the table.
    table = tic01a_recommended_codes;
    count = 60;
    break;

  case TIC_PRODUCT_T825:
    table = tic01a_recommended_codes;
    count = sizeof(tic01a_recommended_codes);
    break;

  default:
    table = NULL;
    count = 0;
    break;
  }

  if (code_count) { *code_count = count; }
  return table;
}

uint32_t tic_current_limit_code_to_ma(uint8_t product, uint8_t code)
{
  if (product == TIC_PRODUCT_T500)
  {
    if (code > TIC_MAX_ALLOWED_CURRENT_CODE_T500)
    {
      code = TIC_MAX_ALLOWED_CURRENT_CODE_T500;
    }
    return tic03a_current_table[code];
  }
  else
  {
    uint8_t max = tic_get_max_allowed_current(product)
      / TIC_CURRENT_LIMIT_UNITS_MA;

    if (code > max) { code = max; }
    else if (code > 64) { code &= ~3; }
    else if (code > 32) { code &= ~1; }

    return code * TIC_CURRENT_LIMIT_UNITS_MA;
  }
}

uint8_t tic_current_limit_ma_to_code(uint8_t product, uint32_t ma)
{
  size_t count;
  const uint8_t * table = tic_get_recommended_current_limit_codes(product, &count);

  // Assumption: The table is an ascending order, so we want to return the last
  // one that is less than or equal to the desired current.
  // Assumption: 0 is a valid code and a good default to use.
  uint8_t code = 0;
  for (size_t i = 0; i < count; i++)
  {
    if (tic_current_limit_code_to_ma(product, table[i]) <= ma)
    {
      code = table[i];
    }
  }
  return code;
}
