/**
Copyright 2009-2018 National Technology and Engineering Solutions of Sandia, 
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government 
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly 
owned subsidiary of Honeywell International, Inc., for the U.S. Department of 
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2018, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

#include <cstring>
#include <sumi/transport.h>
#include <sumi/allreduce.h>
#include <sumi/reduce_scatter.h>
#include <sumi/reduce.h>
#include <sumi/allgather.h>
#include <sumi/allgatherv.h>
#include <sumi/alltoall.h>
#include <sumi/alltoallv.h>
#include <sumi/communicator.h>
#include <sumi/bcast.h>
#include <sumi/gather.h>
#include <sumi/scatter.h>
#include <sumi/gatherv.h>
#include <sumi/scatterv.h>
#include <sumi/scan.h>
#include <sstmac/common/event_callback.h>
#include <sprockit/stl_string.h>
#include <sprockit/sim_parameters.h>
#include <sprockit/keyword_registration.h>

RegisterKeywords(
{ "lazy_watch", "whether failure notifications can be receive without active pinging" },
{ "eager_cutoff", "what message size in bytes to switch from eager to rendezvous" },
{ "use_put_protocol", "whether to use a put or get protocol for pt2pt sends" },
{ "algorithm", "the specific algorithm to use for a given collecitve" },
{ "comm_sync_stats", "whether to track synchronization stats for communication" },
{ "smp_single_copy_size", "the minimum size of message for single-copy protocol" },
{ "max_eager_msg_size", "the maximum size for using eager pt2pt protocol" },
{ "max_vshort_msg_size", "the maximum size for mailbox protocol" },
{ "post_rdma_delay", "the time it takes to post an RDMA operation" },
{ "post_header_delay", "the time it takes to send an eager message" },
{ "poll_delay", "the time it takes to poll for an incoming message" },
{ "rdma_pin_latency", "the latency for each RDMA pin information" },
{ "rdma_page_delay", "the per-page delay for RDMA pinning" },
);

#include <sstmac/common/sstmac_config.h>
#if SSTMAC_INTEGRATED_SST_CORE
#include <sst/core/event.h>
#endif
#include <sumi/transport.h>
#include <sumi/message.h>
#include <sstmac/software/process/app.h>
#include <sstmac/software/process/operating_system.h>
#include <sstmac/software/process/key.h>
#include <sstmac/software/launch/job_launcher.h>
#include <sstmac/hardware/node/node.h>
#include <sstmac/common/event_callback.h>
#include <sstmac/common/runtime.h>
#include <sstmac/common/stats/stat_spyplot.h>
#include <sprockit/output.h>

using namespace sprockit::dbg;

RegisterDebugSlot(sumi);

namespace sumi {

const int options::initial_context = -2;

class sumi_server :
  public sstmac::sw::Service
{

 public:
  sumi_server(Transport* tport)
    : Service(tport->serverLibname(),
       sstmac::sw::SoftwareId(-1, -1), //belongs to no application
       tport->os())
  {
  }

  void registerProc(int rank, Transport* proc){
    int app_id = proc->sid().app_;
    debug_printf(sprockit::dbg::sumi,
                 "sumi_server registering rank %d for app %d",
                 rank, app_id);
    Transport*& slot = procs_[app_id][rank];
    if (slot){
      spkt_abort_printf("sumi_server: already registered rank %d for app %d on node %d",
                        rank, app_id, os_->addr());
    }
    slot = proc;

    auto iter = pending_.begin();
    auto end = pending_.end();
    while (iter != end){
      auto tmp = iter++;
      Message* msg = *tmp;
      if (msg->targetRank() == rank && msg->aid() == proc->sid().app_){
        pending_.erase(tmp);
        proc->incomingMessage(msg);
      }
    }
  }

  bool unregister_proc(int rank, Transport* proc){
    int app_id = proc->sid().app_;
    auto iter = procs_.find(app_id);
    auto& subMap = iter->second;
    subMap.erase(rank);
    if (subMap.empty()){
      procs_.erase(iter);
    }
    return procs_.empty();
  }

  void incomingEvent(sstmac::Event *ev){
    Message* smsg = safe_cast(Message, ev);
    debug_printf(sprockit::dbg::sumi,
                 "sumi_server %d: incoming %s",
                 os_->addr(), smsg->toString().c_str());
    Transport* tport = procs_[smsg->aid()][smsg->targetRank()];
    if (!tport){
      debug_printf(sprockit::dbg::sumi,
                  "sumi_server %d: message pending to app %d, target %d",
                  os_->addr(), smsg->aid(), smsg->targetRank());
      pending_.push_back(smsg);
    } else {
      tport->incomingMessage(smsg);
    }
  }

 private:
  std::map<int, std::map<int, Transport*> > procs_;
  std::list<Message*> pending_;

};

Transport::Transport(sprockit::sim_parameters* params,
                               sstmac::sw::SoftwareId sid,
                               sstmac::sw::OperatingSystem* os) :
 Transport(params, "sumi", sid, os)
{
}

Transport::Transport(sprockit::sim_parameters* params,
               const char* prefix,
               sstmac::sw::SoftwareId sid,
               sstmac::sw::OperatingSystem* os) :
  Transport(params, standardLibname(prefix, sid), sid, os)
{
}

Transport::Transport(sprockit::sim_parameters* params,
               sstmac::sw::SoftwareId sid,
               sstmac::sw::OperatingSystem* os,
               const std::string& prefix,
               const std::string& server_name) :
  Transport(params, standardLibname(prefix.c_str(), sid), sid, os, server_name)
{
}

Transport::Transport(sprockit::sim_parameters* params,
                     const std::string& libname,
                     sstmac::sw::SoftwareId sid,
                     sstmac::sw::OperatingSystem* os,
                     const std::string& server_name) :
  //the name of the transport itself should be mapped to a unique name
  API(params, libname, sid, os),
  //the server is what takes on the specified libname
  inited_(false),
  engine_(nullptr),
  finalized_(false),
  server_libname_(server_name),
  user_lib_time_(nullptr),
  spy_num_messages_(nullptr),
  spy_bytes_(nullptr),
  completion_queues_(1),
  default_progress_queue_(os),
  nic_ioctl_(os->nicDataIoctl())
{
  completion_queues_[0] = std::bind(&default_progress_queue::incoming,
                                    &default_progress_queue_, 0, std::placeholders::_1);

  rank_ = sid.task_;
  auto* server_lib = os_->lib(server_libname_);
  sumi_server* server;
  // only do one server per app per node
  if (server_lib == nullptr) {
    server = new sumi_server(this);
    server->start();
  } else {
    server = safe_cast(sumi_server, server_lib);
  }

  post_rdma_delay_ = params->get_optional_time_param("post_rdma_delay", 0);
  post_header_delay_ = params->get_optional_time_param("post_header_delay", 0);
  poll_delay_ = params->get_optional_time_param("poll_delay", 0);
  user_lib_time_ = new sstmac::sw::LibComputeTime(params, "sumi-user-lib-time", sid, os);

  rdma_pin_latency_ = params->get_optional_time_param("rdma_pin_latency", 0);
  rdma_page_delay_ = params->get_optional_time_param("rdma_page_delay", 0);
  pin_delay_ = rdma_pin_latency_.ticks() || rdma_page_delay_.ticks();
  page_size_ = params->get_optional_byte_length_param("rdma_page_size", 4096);

  rank_mapper_ = sstmac::sw::TaskMapping::globalMapping(sid.app_);
  nproc_ = rank_mapper_->nproc();
  componentId_ = os_->componentId();

  server->registerProc(rank_, this);

  spy_num_messages_ = sstmac::optionalStats<sstmac::StatSpyplot>(desScheduler(),
        params, "traffic_matrix", "ascii", "num_messages");
  spy_bytes_ = sstmac::optionalStats<sstmac::StatSpyplot>(desScheduler(),
        params, "traffic_matrix", "ascii", "bytes");
}

void
Transport::makeEngine()
{
  if (!engine_) engine_ = new CollectiveEngine(params_, this);
}

void
Transport::allocateCq(int id, std::function<void(Message*)>&& f)
{
  completion_queues_[id] = std::move(f);
  auto iter = held_.find(id);
  if (iter != held_.end()){
    auto& list = iter->second;
    for (Message* m : list){
      f(m);
    }
    held_.erase(iter);
  }
}

void
Transport::validateApi()
{
  if (!inited_ || finalized_){
    sprockit::abort("SUMI transport calling function while not inited or already finalized");
  }
}

void
Transport::init()
{
  //THIS SHOULD ONLY BE CALLED AFTER RANK and NPROC are known
  inited_ = true;
}

void
Transport::finish()
{
  //this should really loop through and kill off all the pings
  //so none of them execute
  finalized_ = true;
}

Transport::~Transport()
{
#ifdef FEATURE_TAG_SUMI_RESILIENCE
  if (monitor_) delete monitor_;
#endif

  delete user_lib_time_;
  sumi_server* server = safe_cast(sumi_server, os_->lib(server_libname_));
  bool del = server->unregister_proc(rank_, this);
  if (del) delete server;

  if (engine_) delete engine_;

  //if (spy_bytes_) delete spy_bytes_;
  //if (spy_num_messages_) delete spy_num_messages_;
}

void
Transport::pinRdma(uint64_t bytes)
{
  int num_pages = bytes / page_size_;
  if (bytes % page_size_) ++num_pages;
  sstmac::Timestamp pin_delay = rdma_pin_latency_ + num_pages*rdma_page_delay_;
  compute(pin_delay);
}

sstmac::EventScheduler*
Transport::desScheduler() const
{
  return os_->node();
}

void
Transport::memcopy(uint64_t bytes)
{
  os_->currentThread()->parentApp()->computeBlockMemcpy(bytes);
}

void
Transport::incomingEvent(sstmac::Event *ev)
{
  spkt_abort_printf("sumi_transport::incoming_event: should not directly handle events");
}

int*
Transport::nidlist() const
{
  //just cast an int* - it's fine
  //the types are the same size and the bits can be
  //interpreted correctly
  return (int*) rank_mapper_->rankToNode().data();
}

void
Transport::compute(sstmac::Timestamp t)
{
  user_lib_time_->compute(t);
}

double
Transport::wallTime() const
{
  return now().sec();
}

void
Transport::send(Message* m)
{
#if SSTMAC_COMM_SYNC_STATS
  msg->setTimeSent(wall_time());
#endif
  if (spy_num_messages_) spy_num_messages_->add_one(m->sender(), m->recver());
  if (spy_bytes_){
    switch(m->sstmac::hw::NetworkMessage::type()){
    case sstmac::hw::NetworkMessage::payload:
      spy_bytes_->add(m->sender(), m->recver(), m->byteLength());
      break;
    case sstmac::hw::NetworkMessage::rdma_get_request:
    case sstmac::hw::NetworkMessage::rdma_put_payload:
      spy_bytes_->add(m->sender(), m->recver(), m->payloadBytes());
      break;
    default:
      break;
    }
  }

  switch(m->sstmac::hw::NetworkMessage::type()){
    case sstmac::hw::NetworkMessage::payload:
      if (m->recver() == rank_){
        //deliver to self
        debug_printf(sprockit::dbg::sumi,
          "Rank %d SUMI sending self message", rank_);
        if (m->needsRecvAck()){
          completion_queues_[m->recvCQ()](m);
          //os_->sendExecutionEventNow(sstmac::newCallback(os_->componentId(), this, &transport::incoming_message, m));
        }
        if (m->needsSendAck()){
          auto* ack = m->cloneInjectionAck();
          completion_queues_[m->sendCQ()](static_cast<Message*>(ack));
          //os_->sendExecutionEventNow(
          //  sstmac::newCallback(os_->componentId(), this, &transport::incoming_message, static_cast<message*>(ack)));
        }
      } else {
        if (post_header_delay_.ticks_int64()) {
          user_lib_time_->compute(post_header_delay_);
        }
        nic_ioctl_(m);
      }
      break;
    case sstmac::hw::NetworkMessage::rdma_get_request:
    case sstmac::hw::NetworkMessage::rdma_put_payload:
      if (post_rdma_delay_.ticks_int64()) {
        user_lib_time_->compute(post_rdma_delay_);
      }
      nic_ioctl_(m);
      break;
    default:
      spkt_abort_printf("attempting to initiate send with invalid type %d",
                        m->type())
  }
}

void
Transport::smsgSendResponse(Message* m, uint64_t size, void* buffer, int local_cq, int remote_cq)
{
  //reverse both hardware and software info
  m->sstmac::hw::NetworkMessage::reverse();
  m->reverse();
  m->setupSmsg(buffer, size);
  m->setSendCq(local_cq);
  m->setRecvCQ(remote_cq);
  m->sstmac::hw::NetworkMessage::setType(Message::payload);
  send(m);
}

void
Transport::rdmaGetRequestResponse(Message* m, uint64_t size,
                                     void* local_buffer, void* remote_buffer,
                                     int local_cq, int remote_cq)
{
  //do not reverse send/recver - this is hardware reverse, not software reverse
  m->sstmac::hw::NetworkMessage::reverse();
  m->setupRdmaGet(local_buffer, remote_buffer, size);
  m->setSendCq(remote_cq);
  m->setRecvCQ(local_cq);
  m->sstmac::hw::NetworkMessage::setType(Message::rdma_get_request);
  send(m);
}

void
Transport::rdmaGetResponse(Message* m, uint64_t size, int local_cq, int remote_cq)
{
  smsgSendResponse(m, size, nullptr, local_cq, remote_cq);
}

void
Transport::rdmaPutResponse(Message* m, uint64_t payload_bytes,
                 void* loc_buffer, void* remote_buffer, int local_cq, int remote_cq)
{
  m->reverse();
  m->sstmac::hw::NetworkMessage::reverse();
  m->setupRdmaPut(loc_buffer, remote_buffer, payload_bytes);
  m->setSendCq(local_cq);
  m->setRecvCQ(remote_cq);
  m->sstmac::hw::NetworkMessage::setType(Message::rdma_put_payload);
  send(m);
}

uint64_t
Transport::allocateFlowId()
{
  return os_->node()->allocateUniqueId();
}

void
Transport::incomingMessage(Message *msg)
{
#if SSTMAC_COMM_SYNC_STATS
  if (msg){
    msg->get_payload()->setTimeArrived(wall_time());
  }
#endif

  int cq = msg->isNicAck() ? msg->sendCQ() : msg->recvCQ();
  if (cq != Message::no_ack){
    if (cq >= completion_queues_.size()){
      held_[cq].push_back(msg);
    } else {
      completion_queues_[cq](msg);
    }
  }
}

CollectiveEngine::CollectiveEngine(sprockit::sim_parameters *params, Transport *tport) :
  system_collective_tag_(-1), //negative tags reserved for special system work
  eager_cutoff_(512),
  use_put_protocol_(false),
  global_domain_(nullptr),
  tport_(tport)
{
  global_domain_ = new global_communicator(tport);
  eager_cutoff_ = params->get_optional_int_param("eager_cutoff", 512);
  use_put_protocol_ = params->get_optional_bool_param("use_put_protocol", false);
}

CollectiveEngine::~CollectiveEngine()
{
  if (global_domain_) delete global_domain_;
}

void
CollectiveEngine::notifyCollectiveDone(int rank, collective::type_t ty, int tag)
{
  collective* coll = collectives_[ty][tag];
  if (!coll){
    spkt_throw_printf(sprockit::value_error,
      "transport::notify_collective_done: invalid collective of type %s, tag %d",
       collective::tostr(ty), tag);
  }
  finishCollective(coll, rank, ty, tag);
}

void
CollectiveEngine::deadlock_check()
{
  collective_map::iterator it, end = collectives_.end();
  for (it=collectives_.begin(); it != end; ++it){
    tag_to_collective_map& next = it->second;
    tag_to_collective_map::iterator cit, cend = next.end();
    for (cit=next.begin(); cit != cend; ++cit){
      collective* coll = cit->second;
      if (!coll->complete()){
        coll->deadlock_check();
      }
    }
  }
}

CollectiveDoneMessage*
CollectiveEngine::skip_collective(collective::type_t ty,
  int cq_id, Communicator* comm,
  void* dst, void *src,
  int nelems, int type_size,
  int tag)
{
  if (!comm) comm = global_domain_;
  if (comm->nproc() == 1){
    if (dst && src && (dst != src)){
      ::memcpy(dst, src, nelems*type_size);
    }
    return new CollectiveDoneMessage(tag, ty, comm, cq_id);
  } else {
    return nullptr;
  }
}

CollectiveDoneMessage*
CollectiveEngine::allreduce(void* dst, void *src, int nelems, int type_size, int tag, reduce_fxn fxn,
                             int cq_id, Communicator* comm)
{
 auto* msg = skip_collective(collective::allreduce, cq_id, comm, dst, src, nelems, type_size, tag);
 if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new WilkeHalvingAllreduce(this, dst, src, nelems, type_size, tag, fxn, cq_id, comm);
  return startCollective(coll);
}

sumi::CollectiveDoneMessage*
CollectiveEngine::reduceScatter(void* dst, void *src, int nelems, int type_size, int tag, reduce_fxn fxn,
                                  int cq_id, Communicator* comm)
{
  auto* msg = skip_collective(collective::reduce_scatter, cq_id, comm, dst, src, nelems, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new HalvingReduceScatter(this, dst, src, nelems, type_size, tag, fxn, cq_id, comm);
  return startCollective(coll);
}

sumi::CollectiveDoneMessage*
CollectiveEngine::scan(void* dst, void* src, int nelems, int type_size, int tag, reduce_fxn fxn,
                        int cq_id, Communicator* comm)
{
  auto* msg = skip_collective(collective::scan, cq_id, comm, dst, src, nelems, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new SimultaneousBtreeScan(this, dst, src, nelems, type_size, tag, fxn, cq_id, comm);
  return startCollective(coll);
}


CollectiveDoneMessage*
CollectiveEngine::reduce(int root, void* dst, void *src, int nelems, int type_size, int tag, reduce_fxn fxn,
                          int cq_id, Communicator* comm)
{
  auto* msg = skip_collective(collective::reduce, cq_id, comm, dst, src, nelems, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new WilkeHalvingReduce(this, root, dst, src, nelems, type_size, tag, fxn, cq_id, comm);
  return startCollective(coll);
}

CollectiveDoneMessage*
CollectiveEngine::bcast(int root, void *buf, int nelems, int type_size, int tag,
                         int cq_id, Communicator* comm)
{
  auto* msg = skip_collective(collective::bcast, cq_id, comm, buf, buf, nelems, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new BinaryTreeBcastCollective(this, root, buf, nelems, type_size, tag, cq_id, comm);
  return startCollective(coll);
}

CollectiveDoneMessage*
CollectiveEngine::gatherv(int root, void *dst, void *src,
                   int sendcnt, int *recv_counts,
                   int type_size, int tag, int cq_id, Communicator* comm)
{
  auto* msg = skip_collective(collective::gatherv, cq_id, comm, dst, src, sendcnt, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new BtreeGatherv(this, root, dst, src, sendcnt, recv_counts, type_size, tag, cq_id, comm);
  sprockit::abort("gatherv");
  return startCollective(coll);
}

CollectiveDoneMessage*
CollectiveEngine::gather(int root, void *dst, void *src, int nelems, int type_size, int tag,
                          int cq_id, Communicator* comm)
{
  auto* msg = skip_collective(collective::gather, cq_id, comm, dst, src, nelems, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new BtreeGather(this, root, dst, src, nelems, type_size, tag, cq_id, comm);
  return startCollective(coll);
}

CollectiveDoneMessage*
CollectiveEngine::scatter(int root, void *dst, void *src, int nelems, int type_size, int tag,
                           int cq_id, Communicator* comm)
{
  auto* msg = skip_collective(collective::scatter, cq_id, comm, dst, src, nelems, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new BtreeScatter(this, root, dst, src, nelems, type_size, tag, cq_id, comm);
  return startCollective(coll);
}

CollectiveDoneMessage*
CollectiveEngine::scatterv(int root, void *dst, void *src, int* send_counts, int recvcnt, int type_size, int tag,
                            int cq_id, Communicator* comm)
{
  auto* msg = skip_collective(collective::scatterv, cq_id, comm, dst, src, recvcnt, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new BtreeScatterv(this, root, dst, src, send_counts, recvcnt, type_size, tag, cq_id, comm);
  sprockit::abort("scatterv");
  return startCollective(coll);
}

CollectiveDoneMessage*
CollectiveEngine::alltoall(void *dst, void *src, int nelems, int type_size, int tag,
                            int cq_id, Communicator* comm)
{
  auto* msg = skip_collective(collective::alltoall, cq_id, comm, dst, src, nelems, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new BruckAlltoallCollective(this, dst, src, nelems, type_size, tag, cq_id, comm);
  return startCollective(coll);
}

CollectiveDoneMessage*
CollectiveEngine::alltoallv(void *dst, void *src, int* send_counts, int* recv_counts, int type_size, int tag,
                             int cq_id, Communicator* comm)
{
  auto* msg = skip_collective(collective::alltoallv, cq_id, comm, dst, src, send_counts[0], type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new DirectAlltoallvCollective(this, dst, src, send_counts, recv_counts, type_size, tag, cq_id, comm);
  return startCollective(coll);
}

CollectiveDoneMessage*
CollectiveEngine::allgather(void *dst, void *src, int nelems, int type_size, int tag,
                             int cq_id, Communicator* comm)
{
 auto* msg = skip_collective(collective::allgather, cq_id, comm, dst, src, nelems, type_size, tag);
 if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new BruckAllgatherCollective(
        collective::allgather, this, dst, src, nelems, type_size, tag, cq_id, comm);
  return startCollective(coll);
}

CollectiveDoneMessage*
CollectiveEngine::allgatherv(void *dst, void *src, int* recv_counts, int type_size, int tag,
                              int cq_id, Communicator* comm)
{
  //if the allgatherv is skipped, we have a single recv count
  int nelems = *recv_counts;
  auto* msg = skip_collective(collective::allgatherv, cq_id, comm, dst, src, nelems, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new BruckAllgathervCollective(this, dst, src, recv_counts, type_size, tag, cq_id, comm);
  return startCollective(coll);
}

CollectiveDoneMessage*
CollectiveEngine::barrier(int tag, int cq_id, Communicator* comm)
{
  auto* msg = skip_collective(collective::barrier, cq_id, comm, 0, 0, 0, 0, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new BruckAllgatherCollective(collective::barrier, this, nullptr, nullptr, 0, 0, tag, cq_id, comm);
  return startCollective(coll);
}

CollectiveDoneMessage*
CollectiveEngine::deliverPending(collective* coll, int tag, collective::type_t ty)
{
  std::list<collective_work_message*> pending = pending_collective_msgs_[ty][tag];
  pending_collective_msgs_[ty].erase(tag);
  std::list<collective_work_message*>::iterator it, end = pending.end();

  CollectiveDoneMessage* dmsg = nullptr;
  for (it = pending.begin(); it != end; ++it){
    collective_work_message* msg = *it;
    dmsg = coll->recv(msg);
  }
  return dmsg;
}

void
CollectiveEngine::validateCollective(collective::type_t ty, int tag)
{
  tag_to_collective_map::iterator it = collectives_[ty].find(tag);
  if (it == collectives_[ty].end()){
    return; // all good
  }

  collective* coll = it->second;
  if (!coll){
   spkt_throw_printf(sprockit::illformed_error,
    "sumi_api::validate_collective: lingering null collective of type %s with tag %d",
    collective::tostr(ty), tag);
  }

  if (coll->persistent() && coll->complete()){
    return; // all good
  }

  spkt_throw_printf(sprockit::illformed_error,
    "sumi_api::validate_collective: cannot overwrite collective of type %s with tag %d",
    collective::tostr(ty), tag);
}

CollectiveDoneMessage*
CollectiveEngine::startCollective(collective* coll)
{
  coll->init_actors();
  int tag = coll->tag();
  collective::type_t ty = coll->type();
  //validate_collective(ty, tag);
  collective*& existing = collectives_[ty][tag];
  if (existing){
    coll->start();
    auto* msg = existing->add_actors(coll);
    delete coll;
    return msg;
  } else {
    existing = coll;
    coll->start();
    return deliverPending(coll, tag, ty);
  }
}

void
CollectiveEngine::finishCollective(collective* coll, int rank, collective::type_t ty, int tag)
{
  bool deliver_cq_msg; bool delete_collective;
  coll->actor_done(rank, deliver_cq_msg, delete_collective);
  debug_printf(sprockit::dbg::sumi,
    "Rank %d finishing collective of type %s tag %d",
    tport_->rank(), ty, tag);

  if (!deliver_cq_msg)
    return;

  coll->complete();
  if (delete_collective && !coll->persistent()){ //otherwise collective must exist FOREVER
    collectives_[ty].erase(tag);
    todel_.push_back(coll);
  }

  pending_collective_msgs_[ty].erase(tag);
  debug_printf(sprockit::dbg::sumi,
    "Rank %d finished collective of type %s tag %d",
    tport_->rank(), ty, tag);
}

void
CollectiveEngine::wait_barrier(int tag)
{
  if (tport_->nproc() == 1) return;
  barrier(tag, Message::default_cq);
  auto* dmsg = blockUntilNext(Message::default_cq);
}

void
CollectiveEngine::clean_up()
{
  for (collective* coll : todel_){
    delete coll;
  }
  todel_.clear();
}

CollectiveDoneMessage*
CollectiveEngine::incoming(Message* msg)
{
  clean_up();

  collective_work_message* cmsg = dynamic_cast<collective_work_message*>(msg);
  if (cmsg->sendCQ() == -1 && cmsg->recvCQ() == -1){
    spkt_abort_printf("both CQs are invalid for %s", msg->toString().c_str())
  }
  int tag = cmsg->tag();
  collective::type_t ty = cmsg->type();
  tag_to_collective_map::iterator it = collectives_[ty].find(tag);
  if (it == collectives_[ty].end()){
    debug_printf(sprockit::dbg::sumi_collective,
      "Rank %d, queuing %p %s from %d on tag %d for type %s",
      tport_->rank(), msg,
      Message::tostr(msg->classType()),
      msg->sender(),
      tag, collective::tostr(ty));
      //message for collective we haven't started yet
      pending_collective_msgs_[ty][tag].push_back(cmsg);
      return nullptr;
  } else {
    collective* coll = it->second;
    auto* dmsg = coll->recv(cmsg);
    return dmsg;
  }
}

CollectiveDoneMessage*
CollectiveEngine::blockUntilNext(int cq_id)
{
  CollectiveDoneMessage* dmsg = nullptr;
  while (dmsg == nullptr){
    auto* msg = tport_->blockingPoll(cq_id);
    dmsg = incoming(msg);
  }
  return dmsg;
}


}
