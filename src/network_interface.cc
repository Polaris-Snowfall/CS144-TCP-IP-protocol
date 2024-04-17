#include <iostream>
#include <ranges>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  EthernetFrame ef {};
  uint32_t target_ip = next_hop.ipv4_numeric();



  auto iter = ARP_table.find(target_ip);
  if(iter != ARP_table.end())
  {
    ef.header.src = ethernet_address_;
    ef.header.type = EthernetHeader::TYPE_IPv4;
    ef.header.dst = (*iter).second.second;
    ef.payload = move(serialize(dgram));

    transmit(ef);
  }
  else
  {
    auto iter2 = ips_waiting.find(target_ip);
    if(iter2 != ips_waiting.end() && cur_ms - (*iter2).second <= 5000)
      return;
    
    ARPMessage msg {};
    msg.opcode = ARPMessage::OPCODE_REQUEST;
    msg.sender_ip_address = ip_address_.ipv4_numeric();
    msg.sender_ethernet_address = ethernet_address_;
    msg.target_ip_address = target_ip;

    ef.header.src = ethernet_address_;
    ef.header.type =EthernetHeader::TYPE_ARP;
    ef.header.dst = ETHERNET_BROADCAST;
    ef.payload = move(serialize(msg));

    ips_waiting.insert_or_assign(target_ip,cur_ms);
    dgrams_waiting.emplace(target_ip,dgram);
    transmit(ef);



  }

}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if(frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST)
    return;

  switch (frame.header.type)
  {
    case EthernetHeader::TYPE_IPv4 :
    {
      InternetDatagram ip_dgram;
      if(!parse(ip_dgram,frame.payload))
        return;
      datagrams_received_.emplace(move(ip_dgram));
    }break;

    case EthernetHeader::TYPE_ARP :
    {
      ARPMessage msg;
      if(!parse(msg,frame.payload))
        return;
      
      ARP_table.insert_or_assign(msg.sender_ip_address,make_pair(cur_ms,msg.sender_ethernet_address));
      ips_waiting.erase(msg.sender_ip_address);

      EthernetFrame ef {};
      ef.header.src = ethernet_address_;
      ef.header.dst = msg.sender_ethernet_address;
      ef.header.type = EthernetHeader::TYPE_IPv4;

      auto [head,tail] = dgrams_waiting.equal_range(msg.sender_ip_address);
      for_each(head,tail,[this,&ef](auto& e) mutable -> void{
        ef.payload = move(serialize(e.second));
        transmit(ef);
      });

      if(head != dgrams_waiting.end())
        dgrams_waiting.erase(msg.sender_ip_address);

      if(msg.opcode == ARPMessage::OPCODE_REQUEST && msg.target_ip_address == ip_address_.ipv4_numeric())
      {
        ARPMessage arp_reply {};
        arp_reply.opcode = ARPMessage::OPCODE_REPLY;
        arp_reply.sender_ethernet_address = ethernet_address_;
        arp_reply.sender_ip_address = ip_address_.ipv4_numeric();
        arp_reply.target_ethernet_address = msg.sender_ethernet_address;
        arp_reply.target_ip_address = msg.sender_ip_address;
        
        EthernetFrame reply {};
        reply.header.src = ethernet_address_;
        reply.header.dst = msg.sender_ethernet_address;
        reply.header.type = EthernetHeader::TYPE_ARP;
        reply.payload = move(serialize(arp_reply));

        transmit(reply);
      }
        
    }break;

    default:
      return;
  }

}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  cur_ms += ms_since_last_tick;
  for(auto iter = ARP_table.begin();iter!=ARP_table.end();)
  {
    if(cur_ms - (*iter).second.first > 30000)
      iter = ARP_table.erase(iter);
    else
      iter++;
  }

}
