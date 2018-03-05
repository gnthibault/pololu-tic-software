#include "tic_internal.h"

// Use a current limit code as an index in this table to look up the nominal
// current limit in milliamps.  This table is generated by
// ruby/tic03a_current_table.rb.
static const uint16_t tic03a_current_table[32] =
{
  19,
  152,
  275,
  388,
  492,
  589,
  680,
  766,
  847,
  924,
  998,
  1069,
  1137,
  1203,
  1268,
  1331,
  1393,
  1455,
  1516,
  1577,
  1639,
  1701,
  1764,
  1829,
  1895,
  1964,
  2036,
  2111,
  2190,
  2274,
  2365,
  2463,
};

uint32_t tic_get_max_allowed_current(uint8_t product)
{
  switch (product)
  {
  case TIC_PRODUCT_T500:
    assert(tic03a_current_table[TIC_MAX_ALLOWED_CURRENT_CODE_T500]
      == TIC_MAX_ALLOWED_CURRENT_T500);
    return TIC_MAX_ALLOWED_CURRENT_T500;
  case TIC_PRODUCT_T834:
    return TIC_MAX_ALLOWED_CURRENT_T834;
  default:
    return TIC_MAX_ALLOWED_CURRENT_T825;
  }
}

static uint8_t fix_current_limit_code(uint8_t product, uint8_t code)
{
  if (product == TIC_PRODUCT_T500)
  {
    if (code > TIC_MAX_ALLOWED_CURRENT_CODE_T500)
    {
      code = TIC_MAX_ALLOWED_CURRENT_CODE_T500;
    }
  }
  else
  {
    uint8_t max = tic_get_max_allowed_current(product)
      / TIC_CURRENT_LIMIT_UNITS_MA;

    if (code > max) { code = max; }
    else if (code > 64) { code &= ~3; }
    else if (code > 32) { code &= ~1; }
  }
  return code;
}

// Converts the current limit code value as stored on the EEPROM to the actual
// current limit it represents, taking into account the limitations of the
// firmware and hardware.
uint32_t tic_current_limit_from_code(uint8_t product, uint8_t code)
{
  if (product == TIC_PRODUCT_T500)
  {
    return tic03a_current_table[fix_current_limit_code(product, code)];
  }
  else
  {
    return fix_current_limit_code(product, code) * TIC_CURRENT_LIMIT_UNITS_MA;
  }
}

// Converts a current limit in milliamps to a corresponding current limit code
// for the firmware, taking into account the limitations of the firmware.
// By design, this errs on the side of rounding down.
uint16_t tic_current_limit_to_code(uint8_t product, uint32_t current_limit)
{
  if (product == TIC_PRODUCT_T500)
  {
    uint8_t code = 0;
    for (uint8_t i = 0; i <= TIC_MAX_ALLOWED_CURRENT_CODE_T500; i++)
    {
      if (tic03a_current_table[i] <= current_limit) { code = i; }
    }
    return code;
  }
  else
  {
    return fix_current_limit_code(product, current_limit / TIC_CURRENT_LIMIT_UNITS_MA);
  }
}
