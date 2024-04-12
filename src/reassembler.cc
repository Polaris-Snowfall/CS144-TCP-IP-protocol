#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  //重组数据
  if(first_index>=unassembled_idx && first_index-unassembled_idx<capacity)
  {
    auto psize = min(data.size(),
    unassembled_idx+capacity-first_index);

    auto start = std::next(buffer.begin(),first_index-unassembled_idx);
    std::copy_n(data.begin(),psize,start);
    
    auto mask_start = std::next(valid_mask.begin(),first_index-unassembled_idx);
    std::fill_n(mask_start,psize,true);
  }

  //尝试output
  auto mask_iter = valid_mask.begin();
  auto& writer_ref = static_cast<Writer&>(output_);
  for(;mask_iter!=valid_mask.end()&&*mask_iter==true;++mask_iter);
  
  if(mask_iter != valid_mask.begin())
  {
    auto output_end = std::next(buffer.begin(),(mask_iter-valid_mask.begin()));
    writer_ref.push(std::string(buffer.begin(),output_end));
    unassembled_idx = writer_ref.bytes_pushed();

    //输出完成后移动unassembled的数据
    std::copy(output_end,buffer.end(),buffer.begin());
    auto invalid_iter = std::copy(mask_iter,valid_mask.end(),valid_mask.begin());
    std::fill(invalid_iter,valid_mask.end(),false);
  }

  if(is_last_substring)
    writer_ref.close();
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  uint64_t pendings = 0;
  for(const auto b : valid_mask)
  {
    if(b)
      ++pendings;
  }
  return pendings;
}


Reassembler::Reassembler( ByteStream&& output ): 
output_( std::move( output ) ),capacity(writer().available_capacity()),
buffer(capacity) , valid_mask(capacity,false) 
{

}