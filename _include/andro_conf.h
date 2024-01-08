#ifndef __ADNROD_CONF_H__
#define __ADNROD_CONF_H__

#include <vector>

#include "andro_global.h"

#define ANDRO_CONF_FILE "andromeda.conf"
#define ANDRO_CONF_MASTER_PROCESSES_NAME "Master"
#define ANDRO_CONF_WORK_PROCESSES_NAME "Worker"
#define ANDRO_CONF_WORK_PROCESSES "WorkerProcesses"
#define ANDRO_CONF_DAEMON_MODE "daemon"
#define ANDRO_CONF_LISTEN_PORT_COUNT "PortCount"
#define ANDRO_CONF_PORT_PRIFIX "Port"
#define ANDRO_CONF_WORK_MAX_CONNECTIONS "WorkerConnections"
#define ANDRO_CONF_WORK_THREAD_COUNT "WorkerThreadCount"
#define ANDRO_CONF_SOCK_RECY_WAIT_TIME "SocketRecyWaitTime"
#define ANDRO_CONF_SOCK_KICK_TIMER_ENABLE "SocketKickTimerEnable"
#define ANDRO_CONF_SOCK_TIMEOUT_KICK_ENABLE "SocketTimeoutKick"
#define ANDRO_CONF_SOCK_TIMEOUT_KICK_DEFAULT_VALUE (20 * 3 + 10)
#define ANDRO_CONF_SOCK_MAX_WAIT_TIME "SocketMaxWaitTime"
#define ANDRO_CONF_SOCK_MAX_WAIT_TIME_DEFAULT_VALUE 5
#define ANDRO_CONF_SECURITY_FLOOD_ATTACK_DETECTION_ENABLE "SecurityFloodAttackDetectionEnable"
#define ANDRO_CONF_SECURITY_FLOOD_TIME_INTERVAL "SecurityFloodTimeInterval"
#define ANDRO_CONF_SECURITY_FLOOD_KICK_COUNTER "SecurityFloodKickCounter"

class CConfig {
 private:
  CConfig();

 public:
  ~CConfig();

 private:
  static CConfig *instance;

 public:
  std::vector<LPCConfItem> ConfigItems;

 public:
  bool Load(const char *conf_name);
  const char *GetString(const char *item_name);
  int GetIntDefault(const char *item_name, int def);

 public:
  static CConfig *GetInstance() {
	if (instance == nullptr) {
	  instance = new CConfig();
	  static CRelease cr;
	}
	return instance;
  }

  class CRelease {
   public:
	~CRelease() {
	  if (CConfig::instance) {
		delete CConfig::instance;
		CConfig::instance = nullptr;
	  }
	}
  };
};
#endif