/**
Copyright 2009-2024 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2024, NTESS

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

#ifndef sstmac_common_serializable_h
#define sstmac_common_serializable_h

#include <sstmac/common/sstmac_config.h>

#if SSTMAC_INTEGRATED_SST_CORE
#include <sst/core/serialization/serializer_fwd.h>
#include <sst/core/serialization/serializable.h>
#include <sst/core/serialization/serialize_serializable.h>
#include <sst/core/serialization/serializer.h>

#define START_SERIALIZATION_NAMESPACE namespace SST { namespace Core { namespace Serialization {
#define END_SERIALIZATION_NAMESPACE } } }

#define FRIEND_SERIALIZATION   template <class T, class Enable> friend class SST::Core::Serialization::serialize

/**
namespace sstmac {
using SST::Core::Serialization::serializable;
using SST::Core::Serialization::serializable_type;
using SST::Core::Serialization::serialize;
using SST::Core::Serialization::serializer;
using SST::Core::Serialization::array;
using SST::Core::Serialization::raw_ptr;
}
*/

#define SER_NAMESPACE_OPEN \
   namespace SST { namespace Core { namespace Serialization {
#define SER_NAMESPACE_CLOSE } } }

#define START_SERIALIZATION_NAMESPACE namespace SST { namespace Core { namespace Serialization {
#define END_SERIALIZATION_NAMESPACE } } }

#else
#include <sprockit/serializer_fwd.h>
#include <sprockit/serializable.h>
#include <sprockit/serialize_serializable.h>
#include <sprockit/serializer.h>

namespace SST {
namespace Core {
namespace Serialization {
template <class T> using serializable_type = sprockit::serializable_type<T>;
template <class T> using serialize = typename sprockit::serialize<T>;
using serializable = sprockit::serializable;
using sprockit::array;
using sprockit::raw_ptr;
using serializer = sprockit::serializer;
}
}
}

#define START_SERIALIZATION_NAMESPACE namespace sprockit {
#define END_SERIALIZATION_NAMESPACE }
#define FRIEND_SERIALIZATION   template <class T> friend class sprockit::serialize
#endif

namespace sstmac {
using serializer = SST::Core::Serialization::serializer;
template <class T> using serializable_type = SST::Core::Serialization::serializable_type<T>;
using serializable = SST::Core::Serialization::serializable;
using SST::Core::Serialization::array;
using SST::Core::Serialization::raw_ptr;
}

#define SPKT_CHECK_PACK(ser) \
  std::cout << this << ": " << __FILE__ << ":" << __LINE__ \
            << " size=" << ser.sizer().size() << " packed=" << ser.packer().size() << std::endl

#endif
