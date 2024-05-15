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

#ifndef OUTPUT_H
#define OUTPUT_H

#include <vector>
#include <map>
#include <ostream>

//Default string output for unit tests
template <class T> class ClassOutput
{

 public:
  static void
  print(const T& t, std::ostream& os) {
    os << t;
  }

};

template <class T>
class ClassOutput< std::vector<T> >
{

 public:
  static void
  print(const std::vector<T>& test, std::ostream& os) {
    os << "{ ";
    typename std::vector<T>::const_iterator
    it, begin = test.begin(), end = test.end();
    for (it=begin; it != end; ++it) {
      if (it != begin) {
        os << ",";
      }
      ClassOutput<T>::print(*it, os);
    }
    os << " }";
  }
};

template <class T, class U>
class ClassOutput< std::map<T, U> >
{

 public:
  static void
  print(const std::map<T, U>& test, std::ostream& os) {
    os << "map {\n";
    typename std::map<T,U>::const_iterator
    it, begin = test.begin(), end = test.end();
    for (it=begin; it != end; ++it) {
      if (it != begin) {
        os << ",";
      }
      ClassOutput<T>::print(it->first);
      os << "->";
      ClassOutput<T>::print(it->second);
    }
    os << "   }";
  }

};

#endif // OUTPUT_H
