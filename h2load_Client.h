#ifndef H2LOAD_CLIENT_H
#define H2LOAD_CLIENT_H


#include <vector>
#include <unordered_map>
#include <deque>

#include <list>

#include <ev.h>

extern "C" {
#include <ares.h>
}

#include "memchunk.h"
#include "template.h"

#include "h2load_Config.h"
#include "h2load_Worker.h"
#include "h2load_session.h"
#include "h2load_stats.h"
#include "h2load_Cookie.h"
#include "h2load_utils.h"
#include "Client_Interface.h"


namespace h2load
{

class Client: public Client_Interface
{
public:
    enum { ERR_CONNECT_FAIL = -100 };

    Client(uint32_t id, Worker* worker, size_t req_todo, Config* conf,
           Client* parent = nullptr, const std::string& dest_schema = "",
           const std::string& dest_authority = "");
    virtual ~Client();
    virtual size_t send_out_data(const uint8_t* data, size_t length);
    virtual void signal_write() ;
    virtual bool any_pending_data_to_write();
    virtual void try_new_connection();
    virtual void start_conn_active_watcher(Client_Interface* client);
    virtual std::unique_ptr<Client_Interface> create_dest_client(const std::string& dst_sch, const std::string& dest_authority);
    virtual int connect_to_host(const std::string& schema, const std::string& authority);
    virtual int connect();
    virtual void disconnect();
    virtual void clear_default_addr_info();
    virtual void setup_connect_with_async_fqdn_lookup();
    virtual void feed_timing_script_request_timeout_timer();
    virtual void graceful_restart_connection();
    virtual void restart_timeout_timer();
    virtual void start_rps_timer();
    virtual void start_stream_timeout_timer();
    virtual void start_connect_to_preferred_host_timer();
    virtual void start_timing_script_request_timeout_timer(double duration);
    virtual int select_protocol_and_allocate_session();

    void report_tls_info();

    int do_read();
    int do_write();

    // low-level I/O callback functions called by do_read/do_write
    int connected();
    int read_clear();
    int write_clear();
    int tls_handshake();
    int read_tls();
    int write_tls();

    int on_read(const uint8_t* data, size_t len);
    int on_write();

    int resolve_fqdn_and_connect(const std::string& schema, const std::string& authority,
                                 ares_addrinfo_callback callback = ares_addrinfo_query_callback);

    bool reconnect_to_alt_addr();

    void init_timer_watchers();

    bool probe_address(ares_addrinfo* ares_address);

    int write_clear_with_callback();

    int do_connect();

    void restore_connectfn();
    int connect_with_async_fqdn_lookup();
    void init_ares();

    template<class T>
    int make_socket(T* addr);

    
    DefaultMemchunks wb;
    ev_io wev;
    ev_io rev;
    std::function<int(Client&)> readfn, writefn;
    std::function<int(Client&)> connectfn;
    SSL* ssl;
    ev_timer request_timeout_watcher;
    addrinfo* next_addr;
    // Address for the current address.  When try_new_connection() is
    // used and current_addr is not nullptr, it is used instead of
    // trying next address though next_addr.  To try new address, set
    // nullptr to current_addr before calling connect().
    addrinfo* current_addr;
    ares_addrinfo* ares_address;
    int fd;
    ev_timer conn_active_watcher;
    ev_timer conn_inactivity_watcher;
    // rps_watcher is a timer to invoke callback periodically to
    // generate a new request.
    ev_timer rps_watcher;
    ev_timer stream_timeout_watcher;
    ev_timer connection_timeout_watcher;
    // The timestamp that starts the period which contributes to the
    // next request generation.
    ev_tstamp rps_duration_started;
    // The number of requests allowed by rps, but limited by stream
    // concurrency.
    ev_timer send_ping_watcher;
    ares_channel channel;
    std::map<int, ev_io> ares_io_watchers;
    ev_timer delayed_request_watcher;
    ev_timer delayed_reconnect_watcher;
    ev_timer connect_to_preferred_host_watcher;
    ev_io probe_wev;
    int probe_skt_fd;
};

class Submit_Requet_Wrapper
{
public:
    Client* client;

    Submit_Requet_Wrapper(Client* this_client, Client* next_client)
    {
        if (next_client != this_client && next_client)
        {
            client = next_client;
        }
        else
        {
            client = nullptr;
        }
    };
    ~Submit_Requet_Wrapper()
    {
        if (client &&
            !client->rps_mode() &&
            client->state == CLIENT_CONNECTED)
        {
            client->submit_request();
            client->signal_write();
        }

    };
};

}
#endif
