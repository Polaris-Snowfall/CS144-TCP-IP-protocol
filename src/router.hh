#pragma once

#include <memory>
#include <optional>
#include <map>

#include "exception.hh"
#include "network_interface.hh"

// \brief A router that has multiple network interfaces and
// performs longest-prefix-match routing between them.
class Router
{
public:
  // Add an interface to the router
  // \param[in] interface an already-constructed network interface
  // \returns The index of the interface after it has been added to the router
  size_t add_interface( std::shared_ptr<NetworkInterface> interface )
  {
    _interfaces.push_back( notnull( "add_interface", std::move( interface ) ) );
    return _interfaces.size() - 1;
  }

  // Access an interface by index
  std::shared_ptr<NetworkInterface> interface( const size_t N ) { return _interfaces.at( N ); }

  // Add a route (a forwarding rule)
  void add_route( uint32_t route_prefix,
                  uint8_t prefix_length,
                  std::optional<Address> next_hop,
                  size_t interface_num );

  // Route packets between the interfaces
  void route();

private:
  // The router's collection of network interfaces
  std::vector<std::shared_ptr<NetworkInterface>> _interfaces {};

  struct prefix_info
  {
    prefix_info(const uint8_t prefix_lenth,const uint32_t route_prefix) :
    mask(~(UINT32_MAX >> (prefix_lenth))),prefix(route_prefix) {}

    auto operator>(const prefix_info& other) 
    {
      return mask != other.mask ? (mask > other.mask)
                 : (prefix > other.prefix);
    }
    uint32_t mask;
    uint32_t prefix;
  };
  
  using RouterT = std::multimap<prefix_info,std::pair<size_t,optional<Address> > >;

  RouterT RouterTable {};
};
