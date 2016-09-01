#ifndef ZP_MASTER_SERVER_H
#define ZP_MASTER_SERVER_H

#include <stdio.h>
#include <string>
#include "zp_options.h"
#include "zp_const.h"
#include "slash_status.h"
#include "slash_mutex.h"

#include "floyd.h"
#include "zp_meta_dispatch_thread.h"
#include "zp_meta_worker_thread.h"
#include "zp_meta_update_thread.h"

using slash::Status;


typedef std::unordered_map<std::string, struct timeval> NodeAliveMap;

class ZPMetaServer {
 public:

  explicit ZPMetaServer(const ZPOptions& option);
  virtual ~ZPMetaServer();
  Status Start();
  
  Status Set(const std::string &key, const std::string &value);
  Status Get(const std::string &key, std::string &value);
  Status Delete(const std::string &key);

  std::string seed_ip() {
    return options_.seed_ip;
  }
  int seed_port() {
    return options_.seed_port;
  }
  std::string local_ip() {
    return options_.local_ip;
  }
  int local_port() {
    return options_.local_port;
  }

  Status Distribute(int num);


  Status AddNodeAlive(const std::string& ip_port);
  Status GetAllNode(ZPMeta::Nodes &nodes);
  void GetAllAliveNode(ZPMeta::Nodes &nodes, std::vector<ZPMeta::NodeStatus> &alive_nodes);
  bool FindNode(ZPMeta::Nodes &nodes, const std::string &ip, int port);
  Status SetNodeStatus(ZPMeta::Nodes &nodes, const std::string &ip, int port, int status /*0-UP 1-DOWN*/);
  Status AddNode(const std::string &ip, int port);
  Status OffNode(const std::string &ip, int port);
  void CheckNodeAlive();
  void UpdateNodeAlive(const std::string& ip_port);
  
  Status SetReplicaset(uint32_t partition_id, const ZPMeta::Replicaset &replicaset);
  Status SetMSInfo(const ZPMeta::MetaCmd_Update &ms_info);
  Status GetMSInfo(ZPMeta::MetaCmd_Update &ms_info);

  int PNums();

  // Leader related
  bool IsLeader();
  Status RedirectToLeader(ZPMeta::MetaCmd &request, ZPMeta::MetaCmdResponse &response);

private:

  // Server related
  int worker_num_;
  ZPMetaWorkerThread* zp_meta_worker_thread_[kMaxMetaWorkerThread];
  ZPMetaDispatchThread* zp_meta_dispatch_thread_;
  ZPOptions options_;
  slash::Mutex server_mutex_;

  // Floyd related
  floyd::Floyd* floyd_;
  std::string PartitionId2Key(uint32_t id) {
    std::string key(ZP_META_KEY_PREFIX);
    key += std::to_string(id);
    return key;
  }

  // Alive Check
  slash::Mutex alive_mutex_;
  slash::Mutex node_mutex_;
  NodeAliveMap node_alive_;
  ZPMetaUpdateThread update_thread_;
  void RestoreNodeAlive(std::vector<ZPMeta::NodeStatus> &alive_nodes);

  // Leader slave
  bool leader_first_time_;
  slash::Mutex leader_mutex_;
  pink::PbCli* leader_cli_;
  std::string leader_ip_;
  int leader_cmd_port_;
  Status BecomeLeader();
  void CleanLeader() {
    if (leader_cli_) {
      leader_cli_->Close();
      delete leader_cli_;
    }
    leader_ip_.clear();
    leader_cmd_port_ = 0;
  }
  bool GetLeader(std::string& ip, int& port) {
    int fy_port = 0;
    bool res = floyd_->GetLeader(ip, fy_port);
    if (res) {
      port = fy_port - kMetaPortShiftFY;
    }
    return res;
  }
};

#endif
