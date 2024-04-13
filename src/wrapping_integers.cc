#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return Wrap32 { zero_point+n };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint32_t ckmod = wrap(checkpoint,zero_point).raw_value_;
  uint32_t off = raw_value_-ckmod;
  uint64_t upper = static_cast<uint64_t>(UINT32_MAX)+1;
  
  if(off <= (upper>>1) || checkpoint+off<upper)
    return static_cast<uint64_t>(checkpoint+off);

  return static_cast<uint64_t>(checkpoint+off-upper);

}
