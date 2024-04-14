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
  while(!syned || (reader().bytes_buffered() && window_size>0 && !fined))
  {
    TCPSenderMessage message {};
    if(!fined)
    {
      uint64_t tmp_window_size = window_size.has_value() ? *window_size : 1;

      message.seqno = Wrap32::wrap(reader().bytes_popped()+syned,isn_);

      if(!syned)
        message.SYN = true;

      uint64_t sendsize = min(TCPConfig::MAX_PAYLOAD_SIZE-message.SYN,tmp_window_size-message.SYN);
      read(input_.reader(),sendsize,message.payload);
      
      if(input_.reader().is_finished() && message.sequence_length()<tmp_window_size)
        message.FIN = true;
      
      //该消息有效,有SYN或FIN或payload,发送并启动计时
      if(message.sequence_length())
      {
          outstanding_segments.insert( {Wrap32::wrap(reader().bytes_popped()+syned+message.SYN+message.FIN,isn_), {cur_ms+RTO_ms,message} } );
          transmit(message);

          in_flight_cnt += message.sequence_length();
          *window_size -= message.sequence_length();
          if(!syned)
            syned = true;

          if(message.FIN)
            fined = true;
      }
    }
  };
  
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage message {};
  
  message.seqno = Wrap32::wrap(reader().bytes_popped()+syned+fined,isn_);
  return message;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  bool newdata_flag = (msg.ackno >= (*outstanding_segments.begin()).first);
  if(msg.ackno.has_value())
  {
    if(msg.ackno <= (*outstanding_segments.rbegin()).first)
    {
      for(auto iter = outstanding_segments.begin();iter!=outstanding_segments.upper_bound(*msg.ackno);++iter)
      {
        in_flight_cnt -= (*iter).second.second.sequence_length();
        outstanding_segments.erase(iter);
      }
    }
  }

  if(newdata_flag)
  {
    RTO_ms = initial_RTO_ms_;
    retransmissions_cnt = 0;
    for(auto& f : outstanding_segments)
    {
      f.second.first = cur_ms+RTO_ms;
    }
  }
  window_size = msg.ackno->unwrap(isn_,reader().bytes_popped()+syned+fined)  - (reader().bytes_popped()+syned+fined) + msg.window_size;


}



void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  cur_ms += ms_since_last_tick;
  auto& f = *outstanding_segments.begin();
  if(f.second.first <= cur_ms)
  {
    RTO_ms += RTO_ms;
    f.second.first = cur_ms+RTO_ms;
    transmit(f.second.second);
    ++retransmissions_cnt;
  }
}


