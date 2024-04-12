#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : 
capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return closed_;
}

void Writer::push( string data )
{
  for(const auto ch : data)
  {
    if(available_capacity())
    {
      buffer_.push_back(ch);
      bytes_pushed_++;
    }
  }
  tmp_buf_ = string(buffer_.begin(),buffer_.end());
  return;
}

void Writer::close()
{
  closed_ = true;
}

uint64_t Writer::available_capacity() const
{
  return capacity_-buffer_.size();
}

uint64_t Writer::bytes_pushed() const
{
  return bytes_pushed_;
}

bool Reader::is_finished() const
{
  return closed_&&!bytes_buffered();
}

uint64_t Reader::bytes_popped() const
{
  return bytes_popped_;
}

string_view Reader::peek() const
{
  return string_view(tmp_buf_);
}

void Reader::pop( uint64_t len )
{
  while ((buffer_.size()&&len--))
  {
    buffer_.pop_front();
    bytes_popped_++;
  }
  tmp_buf_ = string(buffer_.begin(),buffer_.end());
}

uint64_t Reader::bytes_buffered() const
{
  return buffer_.size();
}
