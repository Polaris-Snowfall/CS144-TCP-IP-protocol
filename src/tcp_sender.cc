#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return in_flight_cnt;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return retransmissions_cnt;
}

void TCPSender::push( const TransmitFunction& transmit )
{


  while (!syned || ((reader().bytes_buffered() || (reader().is_finished() && !fined)) && *window_size) )
  {
    TCPSenderMessage msg {};
    uint64_t tmp_window_size = window_size.has_value() ? *window_size : 1;

    if(!syned)
      msg.SYN = true;
    
    msg.seqno = Wrap32::wrap(reader().bytes_popped()+syned,isn_);

    uint64_t max_payload_len = min(TCPConfig::MAX_PAYLOAD_SIZE,tmp_window_size);
    max_payload_len -= msg.SYN;
    read(input_.reader(),max_payload_len,msg.payload);

    if(reader().is_finished() && ((tmp_window_size-msg.SYN) > msg.sequence_length()))
    {
      msg.FIN = true;  
      fined = true;
    }
    
    if(reader().has_error())
      msg.RST = true;

    rt_segments.insert( {Wrap32::wrap(reader().bytes_popped()+syned+msg.SYN+msg.FIN,isn_), {cur_ms+RTO_ms,msg} } );
    transmit(msg);

    syned = true;
    in_flight_cnt += msg.sequence_length();
    if(window_size.has_value())
      *window_size -= msg.sequence_length();
  }
  


}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage message {};
  
  message.seqno = Wrap32::wrap(reader().bytes_popped()+syned+fined,isn_);
  if(reader().has_error())
    message.RST = true;

  return message;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  bool newdata_flag = (msg.ackno.has_value() && msg.ackno >= (*rt_segments.begin()).first);
  if(msg.ackno.has_value())
  {
    if(!rt_segments.empty() && msg.ackno <= (*rt_segments.rbegin()).first)
    {
      for(auto iter = rt_segments.begin();iter!=rt_segments.upper_bound(*msg.ackno);++iter)
      {
        in_flight_cnt -= (*iter).second.second.sequence_length();
        rt_segments.erase(iter);
      }
    }
  }

  if(newdata_flag)
  {
    RTO_ms = initial_RTO_ms_;
    retransmissions_cnt = 0;
    for(auto& f : rt_segments)
    {
      f.second.first = cur_ms+RTO_ms;
    }
  }
  window_size = msg.ackno->unwrap(isn_,reader().bytes_popped()+syned+fined)  - (reader().bytes_popped()+syned+fined) + (msg.window_size ? msg.window_size : 1);

  zero_window = !msg.window_size;
  if(msg.RST)
    input_.reader().set_error();
}



void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  cur_ms += ms_since_last_tick;
  if(!rt_segments.empty())
  {
    auto& f = *rt_segments.begin();
    if(f.second.first <= cur_ms)
    {
      if(!zero_window)
        RTO_ms += RTO_ms;
      f.second.first = cur_ms+RTO_ms;
      transmit(f.second.second);
      ++retransmissions_cnt;
    }
  }
}


