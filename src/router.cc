#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  // Your code here.
  RouterTable.emplace(prefix_info(prefix_length,route_prefix),
                                make_pair(interface_num,next_hop));
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  for(auto pNetInterface : _interfaces)
  {
    auto& incoming_dgrams = pNetInterface->datagrams_received();
    while(!incoming_dgrams.empty())
    {
      auto& dgram = incoming_dgrams.front();
      if(dgram.header.ttl <= 1)
      {
        incoming_dgrams.pop();
        continue;
      }
      
      for(auto& RTE : RouterTable)
      {
        if((dgram.header.dst & RTE.first.mask) == RTE.first.prefix)
        {
          --dgram.header.ttl;
          dgram.header.compute_checksum();

          const auto& [interface_num,next_hop] = RTE.second;
          _interfaces[interface_num]->send_datagram(dgram,(next_hop.has_value() ? *next_hop : Address::from_ipv4_numeric(dgram.header.dst)));

          incoming_dgrams.pop();
          break;
        }
      }
    }
  }
}
