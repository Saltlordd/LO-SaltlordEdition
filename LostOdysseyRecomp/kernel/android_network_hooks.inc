// Android-only hooks. Kept outside the desktop stub generator scan.
GUEST_FUNCTION_HOOK(__imp__NetDll_WSACreateEvent, NetDll_WSACreateEvent);
GUEST_FUNCTION_HOOK(__imp__NetDll_XNetDnsLookup, NetDll_XNetDnsLookup);
GUEST_FUNCTION_HOOK(__imp__NetDll_XNetDnsRelease, NetDll_XNetDnsRelease);
GUEST_FUNCTION_HOOK(__imp__NetDll_bind, NetDll_bind);
GUEST_FUNCTION_HOOK(__imp__NetDll_connect, NetDll_connect);
GUEST_FUNCTION_HOOK(__imp__NetDll_getsockname, NetDll_getsockname);
GUEST_FUNCTION_HOOK(__imp__NetDll_getsockopt, NetDll_getsockopt);
GUEST_FUNCTION_HOOK(__imp__NetDll_inet_addr, NetDll_inet_addr);
GUEST_FUNCTION_HOOK(__imp__NetDll_ioctlsocket, NetDll_ioctlsocket);
GUEST_FUNCTION_HOOK(__imp__NetDll_recv, NetDll_recv);
GUEST_FUNCTION_HOOK(__imp__NetDll_recvfrom, NetDll_recvfrom);
GUEST_FUNCTION_HOOK(__imp__NetDll_send, NetDll_send);
GUEST_FUNCTION_HOOK(__imp__NetDll_sendto, NetDll_sendto);
GUEST_FUNCTION_HOOK(__imp__NetDll_setsockopt, NetDll_setsockopt);
