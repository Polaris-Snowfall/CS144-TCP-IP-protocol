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
  size_t ilen = min(data.size(),available_capacity());
  if(ilen)
  {
    data.resize(ilen);

    buffer_.push(data);
    bytes_pushed_ += ilen;
    size_ += ilen;

    if(buffer_.size() == 1)
    {
      view_buf_ = std::string(buffer_.front());
    }

  }

  return;
}

void Writer::close()
{
  closed_ = true;
}

uint64_t Writer::available_capacity() const
{
  return capacity_-size_;
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
  return view_buf_;
}

void Reader::pop( uint64_t len )
{
  size_t rlen = min(len,size_);
  size_t rlen_remaind = rlen;
  if(rlen)
  {
    while(!buffer_.empty() && rlen_remaind >= buffer_.front().size())
    {
      rlen_remaind -= buffer_.front().size();
      buffer_.pop();
    }

    bytes_popped_ += rlen;
    size_ -= rlen;

    if(!buffer_.empty())
    {
      buffer_.front() = buffer_.front().substr(rlen_remaind);
      view_buf_ = std::string(buffer_.front());
    }
    else
      view_buf_ = std::string();
  }
}

uint64_t Reader::bytes_buffered() const
{
  return size_;
}
