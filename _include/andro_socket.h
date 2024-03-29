#ifndef __ADNROD_SOCKET_H__
#define __ADNROD_SOCKET_H__

#include <pthread.h>
#include <semaphore.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <atomic>
#include <list>
#include <map>
#include <vector>

#include "andro_packet.h"

#define ANDRO_LISTEN_BACKLOG 511
#define ANDRO_MAX_EPOLL_WAIT_EVENTS 512

#define ANDRO_MSG_HEADER_LEN sizeof(andro_message_header_s)

typedef struct andro_message_header_s message_header_t, *lp_message_header_t;
typedef struct andro_listening_s listening_t, *lp_listening_t;
typedef struct andro_connection_s connection_t, *lp_connection_t;
typedef class CSocket CSocket;

typedef void (CSocket::*event_handler)(lp_connection_t c);

struct andro_message_header_s {
  lp_connection_t conn_ptr;
  uint64_t sequence;// When a data packet is received, the corresponding connection number is recorded.
  // This can be used in the future to determine whether the connection has been invalidated.
};

struct andro_listening_s {
  int port;
  int fd;
  lp_connection_t connection;// This is a connection from the connection pool.
};

struct andro_connection_s {
  andro_connection_s();
  virtual ~andro_connection_s();
  void GetOneToUse();
  void PutOneToFree();

  int fd{};
  lp_listening_t listening{};

  unsigned instance: 1;// 0: valid, 1: invalid.
  uint64_t sequence;
  struct sockaddr socket_addr{};

  uint8_t w_ready{};

  event_handler rhandler{};// This handler for reading.
  event_handler whandler{};// This handler for writing.

  uint32_t events{};// Related to epoll event.

  unsigned char packet_stat{};                  // The current status of the packet.
  char packet_head_info[ANDRO_DATA_BUFF_SIZE]{};// Used to store the header information of the packet.
  char *packet_recv_buf_ptr
	  {};                  // The head pointer of the reception buffer, which is particularly useful for handling partially received packets.
  unsigned int packet_recv_len
	  {};               // The amount of data to be received is specified by this variable and is used in conjunction with `packet_recv_buf_ptr`.

  // bool is_memory_allocated_for_packet;  // If we successfully receive the packet header,
  //                                       // we need to allocate memory to store the packet header, message header, and packet body content.
  //                                       // This flag is used to indicate whether we have allocated memory using `new` because memory allocated with `new` needs to be released.

  char *allocated_packet_recv_mem_ptr
	  {};// The memory address allocated for receiving packets using `new`, used in conjunction with the `is_memory_allocated_for_packet`.

  pthread_mutex_t logic_proc_mutex{};

  std::atomic<int> throw_send_count{};
  char *packet_send_buf_ptr{};
  unsigned int packet_send_len{};
  char *allocated_packet_send_mem_ptr{};

  time_t recy_time{};

  time_t last_ping_time{};

  // related to net security
  uint64_t flood_kick_last_time{};
  int flood_attack_count{};
  std::atomic<int> send_count;

  lp_connection_t next_conn{};// This is a pointer and assign to element of singal way linked-list

  void DataOffset(ssize_t len) {
	packet_recv_buf_ptr = packet_recv_buf_ptr + len;
	packet_recv_len = packet_recv_len - len;
  }

  void ResetRecv() {
	packet_stat = ANDRO_PKG_STAT_HEAD_INIT;
	packet_recv_buf_ptr = packet_head_info;
	packet_recv_len = ANDRO_PKG_HEADER_LEN;
  }
};

class CSocket {
 public:
  CSocket();
  virtual ~CSocket();
  virtual bool Init();
  virtual bool InitSubProc();
  virtual void ShutdownSubProc();

  void PrintThreadInfo();

 public:
  virtual void ThreadRecvProcFunc(char *msg_buffer);
  virtual void HeartBeatTimeoutChecking(lp_message_header_t msg_header_ptr, time_t cur_time);

 public:
  int EpollInit();
  int EpollOperateEvent(int fd, uint32_t event_type, uint32_t other_flag, int bcation, lp_connection_t conn_ptr) const;
  int EpollProcessEvents(int timer);

 protected:
  void msg_send(char *send_buffer);
  void kick(lp_connection_t conn_ptr);

 private:
  void read_conf();
  bool open_listening_sockets();
  void close_listening_sockets();
  static bool set_nonblocking(int socket_fd);

  void event_accept(lp_connection_t old_conn_ptr);// Build for new connection.
  void read_request_handler(lp_connection_t conn_ptr);
  void write_request_handler(lp_connection_t conn_ptr);
  void close_connection(lp_connection_t conn_ptr);

  ssize_t recv_proc(lp_connection_t conn_ptr, char *buffer, ssize_t buf_len);
  void proc_header_handler(lp_connection_t conn_ptr, bool &is_flood);
  void proc_data_handler(lp_connection_t conn_ptr, bool &is_flood);
  void clear_msg_send_queue();

  static ssize_t send_proc(lp_connection_t conn_ptr, char *buffer, ssize_t size);

  static size_t sock_ntop(struct sockaddr *sock_addr, int port, u_char *text, size_t len);

  void init_connection_pool();
  void clear_connection_pool();
  lp_connection_t get_connection(int socket_fd);
  void free_connection(lp_connection_t conn_ptr);
  void push_to_recy_connect_queue(lp_connection_t conn_ptr);

  void push_to_timer_queue(lp_connection_t conn_ptr);
  lp_message_header_t get_over_time_timer(time_t cur_time);
  time_t get_earliest_time();
  lp_message_header_t remove_first_timer();
  void delete_from_timer_queue(lp_connection_t conn_ptr);
  void clear_timer_queue();

  // related to net security
  bool detect_flood(lp_connection_t conn_ptr);

  static void *ServerSendQueueThread(void *thread_data);
  static void *ServerRecyConnectionThread(void *thread_data);
  static void *ServerTimerQueueMonitorThread(void *thread_data);

 protected:
  int timeout_kick_enable;
  int timeout_wait_time;

 private:
  typedef struct andro_thread_s {
	pthread_t handle{};
	CSocket *This;
	bool is_running;

	explicit andro_thread_s(CSocket *sock) : This(sock), is_running(false) {}

	~andro_thread_s() = default;

  } thread_t, *lp_thread_t;

  int worker_max_connections;// The epoll max connections.
  int listen_port_count;     // The number of listening ports.
  int epoll_handler;         // The handle returned by epoll_create().

  std::list<lp_connection_t> connection_pool;
  std::list<lp_connection_t> free_connection_pool;
  std::atomic<int> total_connection_size{};
  std::atomic<int> free_connection_size{};
  pthread_mutex_t connection_mutex{};
  pthread_mutex_t recy_conn_queue_mutex{};
  std::list<lp_connection_t> recy_connection_pool;
  std::atomic<int> total_recy_connection_size{};

  int recy_connection_wait_time;

  std::vector<lp_listening_t> listen_socket_list;
  struct epoll_event epoll_events[ANDRO_MAX_EPOLL_WAIT_EVENTS]{};

  std::list<char *> send_msg_queue;
  std::atomic<int> send_msg_queue_size{};

  std::vector<lp_thread_t> thread_container;
  pthread_mutex_t send_msg_queue_mutex{};
  sem_t sem_event_send_queue{};

  // related to timer
  int kick_timer_enable;
  pthread_mutex_t timer_queue_mutex;
  std::multimap<time_t, lp_message_header_t> timer_queue_map;
  size_t timer_queue_map_size;
  time_t timer_value;

  std::atomic<int> online_user_count;

  // related to net security
  int flood_attack_detection_enable;
  int flood_time_interval;
  int flood_kick_count;

  // related to metric
	time_t last_print_time;
	int discard_send_pkg_count;
};

#endif