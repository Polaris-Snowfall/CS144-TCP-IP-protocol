#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if(message.SYN)
    ISN = message.seqno;
  
  if(ISN.has_value())
  {
    uint64_t first_index = message.seqno.unwrap(*ISN,writer().bytes_pushed())+message.SYN-1;
    reassembler_.insert(first_index,message.payload,message.FIN);
  }

  if(message.RST)
  {
    reader().set_error();
  }
  
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage message {};
  if(ISN.has_value())
  {
    message.ackno = Wrap32::wrap(writer().bytes_pushed()+1+writer().is_closed(),*ISN);
  }
  message.window_size = min(writer().available_capacity(),static_cast<uint64_t>(UINT16_MAX));

  message.RST = writer().has_error(); 
  return message;
}
