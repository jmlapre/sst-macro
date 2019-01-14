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

#include <sumi-mpi/mpi_api.h>
#include <sumi-mpi/mpi_queue/mpi_queue.h>
#include <sumi-mpi/otf2_output_stat.h>
#include <sstmac/software/process/operating_system.h>
#include <sstmac/software/process/thread.h>

#define do_vcoll(coll, fxn, ...) \
  start_mpi_call(fxn); \
  auto op = start##coll(#fxn, __VA_ARGS__); \
  waitCollective(op); \
  finish_mpi_call(fxn);

#define start_vcoll(coll, fxn, ...) \
  start_mpi_call(fxn); \
  auto op = start##coll(#fxn, __VA_ARGS__); \
  addImmediateCollective(op, req); \
  finish_mpi_call(fxn)

namespace sumi {

void
MpiApi::finishVcollectiveOp(CollectiveOpBase* op_)
{
  CollectivevOp* op = static_cast<CollectivevOp*>(op_);
  if (op->packed_recv){
    spkt_throw_printf(sprockit::unimplemented_error,
               "cannot handle non-contiguous types in collective %s",
               collective::tostr(op->ty));
  }
  if (op->packed_send){
    spkt_throw_printf(sprockit::unimplemented_error,
               "cannot handle non-contiguous types in collective %s",
               collective::tostr(op->ty));
  }
}

sumi::CollectiveDoneMessage*
MpiApi::startAllgatherv(CollectivevOp* op)
{
  return engine_->allgatherv(op->tmp_recvbuf, op->tmp_sendbuf,
                  op->recvcounts, op->sendtype->packed_size(), op->tag,
                  queue_->collCqId(), op->comm);

}

CollectiveOpBase*
MpiApi::startAllgatherv(const char* name, MPI_Comm comm, int sendcount, MPI_Datatype sendtype,
                          const int *recvcounts, const int *displs, MPI_Datatype recvtype,
                          const void *sendbuf, void *recvbuf)
{
  mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_collective,
    "%s(%d,%s,<...>,%s,%s)", name,
    sendcount, typeStr(sendtype).c_str(),
    typeStr(recvtype).c_str(),
    commStr(comm).c_str());

  CollectivevOp* op = new CollectivevOp(sendcount, const_cast<int*>(recvcounts),
                                          const_cast<int*>(displs), getComm(comm));
  startMpiCollective(collective::allgatherv, sendbuf, recvbuf, sendtype, recvtype, op);
  auto* msg = startAllgatherv(op);
  if (msg){
    op->complete = true;
    delete msg;
  }
  return op;
}

int
MpiApi::allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    void *recvbuf, const int *recvcounts, const int *displs,
                    MPI_Datatype recvtype, MPI_Comm comm)
{
  auto start_clock = trace_clock();

  do_vcoll(Allgatherv, MPI_Allgatherv, comm, sendcount, sendtype,
           recvcounts, displs, recvtype, sendbuf, recvbuf);

#ifdef SSTMAC_OTF2_ENABLED
  if (otf2_writer_){
    otf2_writer_->writer().mpi_allgatherv(start_clock, trace_clock(),
       get_comm(comm)->size(), sendcount, sendtype,
       recvcounts, recvtype, comm);
  }
#endif
  return MPI_SUCCESS;
}

int
MpiApi::allgatherv(int sendcount, MPI_Datatype sendtype,
                    const int *recvcounts, MPI_Datatype recvtype, MPI_Comm comm)
{
  return allgatherv(NULL, sendcount, sendtype, NULL, recvcounts, NULL, recvtype, comm);
}

int
MpiApi::iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    void *recvbuf, const int *recvcounts, const int *displs,
                    MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* req)
{
  start_vcoll(Allgatherv, MPI_Iallgatherv, comm, sendcount, sendtype,
           recvcounts, displs, recvtype, sendbuf, recvbuf);
  return MPI_SUCCESS;
}

int
MpiApi::iallgatherv(int sendcount, MPI_Datatype sendtype,
                     const int *recvcounts, MPI_Datatype recvtype,
                     MPI_Comm comm, MPI_Request* req)
{
  return iallgatherv(NULL, sendcount, sendtype,
                    NULL, recvcounts, NULL,
                    recvtype, comm, req);
}

sumi::CollectiveDoneMessage*
MpiApi::startAlltoallv(CollectivevOp* op)
{
  return engine_->alltoallv(op->tmp_recvbuf, op->tmp_sendbuf,
                  op->sendcounts, op->recvcounts,
                  op->sendtype->packed_size(), op->tag,
                  queue_->collCqId(), op->comm);
}

CollectiveOpBase*
MpiApi::startAlltoallv(const char* name, MPI_Comm comm,
                         const int *sendcounts, MPI_Datatype sendtype, const int *sdispls,
                         const int *recvcounts, MPI_Datatype recvtype, const int *rdispls,
                         const void *sendbuf,  void *recvbuf)
{
  if (sendbuf || recvbuf){
    mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_collective,
      "%s(<...>,%s,<...>,%s,%s)", name,
      typeStr(sendtype).c_str(), typeStr(recvtype).c_str(), commStr(comm).c_str());
    CollectivevOp* op = new CollectivevOp(const_cast<int*>(sendcounts), const_cast<int*>(sdispls),
                                              const_cast<int*>(recvcounts), const_cast<int*>(rdispls),
                                              getComm(comm));
    startMpiCollective(collective::alltoallv, sendbuf, recvbuf, sendtype, recvtype, op);
    auto* msg = startAlltoallv(op);
    if (msg){
      op->complete = true;
      delete msg;
    }
    return op;
  } else {
    MpiComm* commPtr = getComm(comm);
    int send_count = 0;
    int recv_count = 0;
    int nproc = commPtr->size();
    for (int i=0; i < nproc; ++i){
      send_count += sendcounts[i];
      recv_count += recvcounts[i];
    }
    send_count /= nproc;
    recv_count /= nproc;
    CollectiveOpBase* op = startAlltoall(name, comm, send_count, sendtype,
                                            recv_count, recvtype, sendbuf, recvbuf);
    return op;
  }
}

int
MpiApi::alltoallv(const void *sendbuf, const int *sendcounts,
                   const int *sdispls, MPI_Datatype sendtype,
                   void *recvbuf, const int *recvcounts,
                   const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
{
  auto start_clock = trace_clock();

  do_vcoll(Alltoallv, MPI_Alltoallv, comm,
           sendcounts, sendtype, sdispls,
           recvcounts, recvtype, rdispls, sendbuf, recvbuf);

#ifdef SSTMAC_OTF2_ENABLED
  if (otf2_writer_){
    otf2_writer_->writer().mpi_alltoallv(start_clock, trace_clock(),
        get_comm(comm)->size(), sendcounts, sendtype,
        recvcounts, recvtype, comm);
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::alltoallv(const int *sendcounts, MPI_Datatype sendtype,
                   const int *recvcounts, MPI_Datatype recvtype, MPI_Comm comm)
{
  return alltoallv(NULL, sendcounts, NULL, sendtype,
                   NULL, recvcounts, NULL, recvtype, comm);
}

int
MpiApi::ialltoallv(const void *sendbuf, const int *sendcounts,
                   const int *sdispls, MPI_Datatype sendtype,
                   void *recvbuf, const int *recvcounts,
                   const int *rdispls, MPI_Datatype recvtype,
                   MPI_Comm comm, MPI_Request* req)
{
  start_vcoll(Alltoallv, MPI_Ialltoallv, comm, sendcounts, sendtype, sdispls,
              recvcounts, recvtype, rdispls, sendbuf, recvbuf);
  return MPI_SUCCESS;
}

int
MpiApi::ialltoallv(const int *sendcounts, MPI_Datatype sendtype,
                   const int *recvcounts, MPI_Datatype recvtype,
                   MPI_Comm comm, MPI_Request* req)
{
  return ialltoallv(NULL, sendcounts, NULL, sendtype,
                    NULL, recvcounts, NULL, recvtype,
                    comm, req);
}

int
MpiApi::ialltoallw(const void *sendbuf, const int sendcounts[],
                    const int sdispls[], const MPI_Datatype sendtypes[],
                    void *recvbuf, const int recvcounts[],
                    const int rdispls[], const MPI_Datatype recvtypes[],
                    MPI_Comm comm, MPI_Request *request)
{
  sprockit::abort("MPI_Ialltoallw: unimplemented"
                  "but, seriously, why are you using this collective anyway?");
  return MPI_SUCCESS;
}

sumi::CollectiveDoneMessage*
MpiApi::startGatherv(CollectivevOp* op)
{
  sprockit::abort("sumi::gatherv: not implemented");
  return nullptr;
  //transport::gatherv(op->tmp_recvbuf, op->tmp_sendbuf,
  //                     op->sendcnt, typeSize, op->tag,
  //                false, options::initial_context, op->comm);
}

CollectiveOpBase*
MpiApi::startGatherv(const char* name, MPI_Comm comm, int sendcount, MPI_Datatype sendtype, int root,
                       const int *recvcounts, const int *displs, MPI_Datatype recvtype,
                       const void *sendbuf, void *recvbuf)
{
  if (sendbuf || recvbuf){
    mpi_api_debug(sprockit::dbg::mpi,
      "%s(%d,%s,<...>,%s,%d,%s)", name,
      sendcount, typeStr(sendtype).c_str(),
      typeStr(recvtype).c_str(),
      int(root), commStr(comm).c_str());
    CollectivevOp* op = new CollectivevOp(sendcount,
                const_cast<int*>(recvcounts),
                const_cast<int*>(displs), getComm(comm));

    op->root = root;
    if (root == op->comm->rank()){
      //pass
    } else {
      recvtype = MPI_DATATYPE_NULL;
      recvbuf = nullptr;
    }

    startMpiCollective(collective::gatherv, sendbuf, recvbuf, sendtype, recvtype, op);
    auto* msg = startGatherv(op);
    if (msg){
      op->complete = true;
      delete msg;
    }
    return op;
  } else {
    MpiComm* commPtr = getComm(comm);
    int recvcount = sendcount;
    if (commPtr->rank() == root){
      int total_count = 0;
      int nproc = commPtr->size();
      for (int i=0; i < nproc; ++i){
        total_count += recvcounts[i];
      }
      recvcount = total_count / nproc;
    }
    CollectiveOpBase* op = startGather(name, comm, sendcount, sendtype, root, recvcount, recvtype,
                                          sendbuf, recvbuf);
    return op;
  }
}

int
MpiApi::gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, const int *recvcounts, const int *displs,
                 MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  auto start_clock = trace_clock();

  do_vcoll(Gatherv, MPI_Gatherv, comm, sendcount, sendtype, root,
           recvcounts, displs, recvtype, sendbuf, recvbuf);

#ifdef SSTMAC_OTF2_ENABLED
  if (otf2_writer_){
    otf2_writer_->writer().mpi_gatherv(start_clock, trace_clock(),
      get_comm(comm)->size(), sendcount, sendtype,
      recvcounts, recvtype, root, comm);
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::gatherv(int sendcount, MPI_Datatype sendtype,
                 const int *recvcounts, MPI_Datatype recvtype,
                 int root, MPI_Comm comm)
{
  return gatherv(NULL, sendtype, sendcount, NULL, recvcounts, NULL, recvtype, root, comm);
}

int
MpiApi::igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, const int *recvcounts, const int *displs,
                 MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request* req)
{
  start_vcoll(Gatherv, MPI_Igatherv, comm, sendcount, sendtype, root,
              recvcounts, displs, recvtype, sendbuf, recvbuf);
  return MPI_SUCCESS;
}

int
MpiApi::igatherv(int sendcount, MPI_Datatype sendtype,
                 const int *recvcounts, MPI_Datatype recvtype,
                 int root, MPI_Comm comm, MPI_Request* req)
{
  return igatherv(NULL, sendcount, sendtype, NULL,
                  recvcounts, NULL, recvtype, root, comm, req);
}

sumi::CollectiveDoneMessage*
MpiApi::startScatterv(CollectivevOp* op)
{
  sprockit::abort("sumi::scatterv: not implemented");
  return nullptr;
  //transport::allgatherv(op->tmp_recvbuf, op->tmp_sendbuf,
  //                     op->sendcnt, typeSize, op->tag,
  //                false, options::initial_context, op->comm);
}

CollectiveOpBase*
MpiApi::startScatterv(const char* name, MPI_Comm comm, const int *sendcounts, MPI_Datatype sendtype, int root,
                        const int *displs, int recvcount, MPI_Datatype recvtype, const void *sendbuf, void *recvbuf)
{
  if (sendbuf || recvbuf){
    mpi_api_debug(sprockit::dbg::mpi,
      "%s(<...>,%s,%d,%s,%d,%s)", name,
      typeStr(sendtype).c_str(),
      recvcount, typeStr(recvtype).c_str(),
      int(root), commStr(comm).c_str());

    CollectivevOp* op = new CollectivevOp(const_cast<int*>(sendcounts),
                      const_cast<int*>(displs),
                      recvcount, getComm(comm));
    op->root = root;
    if (root == op->comm->rank()){
      //pass
    } else {
      sendtype = MPI_DATATYPE_NULL;
      sendbuf = nullptr;
    }

    startMpiCollective(collective::scatterv, sendbuf, recvbuf, sendtype, recvtype, op);
    auto* msg = startScatterv(op);
    if (msg){
      op->complete = true;
      delete msg;
    }
    return op;
  } else {
    MpiComm* commPtr = getComm(comm);
    int sendcount = recvcount;
    if (commPtr->rank() == root){
      int total_count = 0;
      int nproc = commPtr->size();
      for (int i=0; i < nproc; ++i){
        total_count += sendcounts[i];
      }
      sendcount = total_count / nproc;
    }
    CollectiveOpBase* op = startScatter(name, comm, sendcount, sendtype, root,
                                           recvcount, recvtype, sendbuf, recvbuf);
    return op;
  }
}

int
MpiApi::scatterv(const void* sendbuf, const int* sendcounts, const int *displs, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  auto start_clock = trace_clock();

  do_vcoll(Scatterv, MPI_Scatterv, comm, sendcounts, sendtype, root, displs,
           recvcount, recvtype, sendbuf, recvbuf);

#ifdef SSTMAC_OTF2_ENABLED
  if (otf2_writer_){
    otf2_writer_->writer().mpi_scatterv(start_clock, trace_clock(),
      get_comm(comm)->size(), sendcounts, sendtype,
      recvcount, recvtype, root, comm);
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::scatterv(const int *sendcounts, MPI_Datatype sendtype,
                  int recvcount, MPI_Datatype recvtype,
                  int root, MPI_Comm comm)
{
  return scatterv(NULL, sendcounts, NULL, sendtype, NULL,
                  recvcount, recvtype, root, comm);
}


int
MpiApi::iscatterv(const void* sendbuf, const int* sendcounts, const int *displs, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  int root, MPI_Comm comm, MPI_Request* req)
{
  start_vcoll(Scatterv, MPI_Iscatterv, comm, sendcounts, sendtype, root, displs,
              recvcount, recvtype, sendbuf, recvbuf);
  return MPI_SUCCESS;
}

int
MpiApi::iscatterv(const int *sendcounts, MPI_Datatype sendtype,
                  int recvcount, MPI_Datatype recvtype,
                  int root, MPI_Comm comm, MPI_Request* req)
{
  return iscatterv(NULL, sendcounts, NULL, sendtype,
                   NULL, recvcount, recvtype, root, comm, req);
}

}
