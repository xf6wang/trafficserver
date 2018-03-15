/** @file

    Unit tests for BufferFormat and bwprint.

    @section license License

    Licensed to the Apache Software Foundation (ASF) under one
    or more contributor license agreements.  See the NOTICE file
    distributed with this work for additional information
    regarding copyright ownership.  The ASF licenses this file
    to you under the Apache License, Version 2.0 (the
    "License"); you may not use this file except in compliance
    with the License.  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
 */

#include "catch.hpp"
#include <ts/BufferWriter.h>
#include <chrono>
#include <iostream>

TEST_CASE("Buffer Writer << operator", "[bufferwriter][stream]")
{
  ts::LocalBufferWriter<50> bw;

  bw << "The" << ' ' << "quick" << ' ' << "brown fox";

  REQUIRE(bw.view() == "The quick brown fox");

  bw.reduce(0);
  bw << "x=" << bw.capacity();
  REQUIRE(bw.view() == "x=50");
}

TEST_CASE("bwprint basics", "[bwprint]")
{
  ts::LocalBufferWriter<256> bw;
  auto fmt1{"Some text"_sv};

  bw.print(fmt1);
  REQUIRE(bw.view() == fmt1);
  bw.reduce(0);
  bw.print("Arg {}", 1);
  REQUIRE(bw.view() == "Arg 1");
  bw.reduce(0);
  bw.print("arg 1 {1} and 2 {2} and 0 {0}", "zero", "one", "two");
  REQUIRE(bw.view() == "arg 1 one and 2 two and 0 zero");
  bw.reduce(0);
  bw.print("args {2}{0}{1}", "zero", "one", "two");
  REQUIRE(bw.view() == "args twozeroone");
  bw.reduce(0);
  bw.print("left |{:<10}|", "text");
  REQUIRE(bw.view() == "left |text      |");
  bw.reduce(0);
  bw.print("right |{:>10}|", "text");
  REQUIRE(bw.view() == "right |      text|");
  bw.reduce(0);
  bw.print("right |{:.>10}|", "text");
  REQUIRE(bw.view() == "right |......text|");
  bw.reduce(0);
  bw.print("center |{:.=10}|", "text");
  REQUIRE(bw.view() == "center |...text...|");
  bw.reduce(0);
  bw.print("center |{:.=11}|", "text");
  REQUIRE(bw.view() == "center |...text....|");
  bw.reduce(0);
  bw.print("center |{:==10}|", "text");
  REQUIRE(bw.view() == "center |===text===|");
  bw.reduce(0);
  bw.print("center |{:%3A=10}|", "text");
  REQUIRE(bw.view() == "center |:::text:::|");
  bw.reduce(0);
  bw.print("left >{0:<9}< right >{0:>9}< center >{0:=9}<", 956);
  REQUIRE(bw.view() == "left >956      < right >      956< center >   956   <");

  bw.reduce(0);
  bw.print("Format |{:>#010x}|", -956);
  REQUIRE(bw.view() == "Format |0000-0x3bc|");
  bw.reduce(0);
  bw.print("Format |{:<#010x}|", -956);
  REQUIRE(bw.view() == "Format |-0x3bc0000|");
  bw.reduce(0);
  bw.print("Format |{:#010x}|", -956);
  REQUIRE(bw.view() == "Format |-0x00003bc|");

  bw.reduce(0);
  bw.print("{{BAD_ARG_INDEX:{} of {}}}", 17, 23);
  REQUIRE(bw.view() == "{BAD_ARG_INDEX:17 of 23}");

  bw.reduce(0);
  bw.print("Arg {0} Arg {3}", 1, 2);
  REQUIRE(bw.view() == "Arg 1 Arg {BAD_ARG_INDEX:3 of 2}");

  bw.reduce(0);
  bw.print("{{stuff}} Arg {0} Arg {}", 1, 2);
  REQUIRE(bw.view() == "{stuff} Arg 1 Arg 2");
  bw.reduce(0);
  bw.print("Arg {0} Arg {} and {{stuff}}", 3, 4);
  REQUIRE(bw.view() == "Arg 3 Arg 4 and {stuff}");
  bw.reduce(0);
  bw.print("Arg {{{0}}} Arg {} and {{stuff}}", 5, 6);
  REQUIRE(bw.view() == "Arg {5} Arg 6 and {stuff}");
  bw.reduce(0);
  bw.print("Arg {0} Arg {{}}{{}} {} and {{stuff}}", 7, 8);
  REQUIRE(bw.view() == "Arg 7 Arg {}{} 8 and {stuff}");
  bw.reduce(0);
  bw.print("Arg {0} Arg {{{{}}}} {}", 9, 10);
  REQUIRE(bw.view() == "Arg 9 Arg {{}} 10");

  bw.reduce(0);
  bw.print("Arg {0} Arg {{{{}}}} {}", 9, 10);
  REQUIRE(bw.view() == "Arg 9 Arg {{}} 10");
  bw.reduce(0);
  bw.print("Time is {now}");
  //  REQUIRE(bw.view() == "Time is");
}

TEST_CASE("BWFormat", "[bwprint][bwformat]")
{
  ts::LocalBufferWriter<256> bw;
  ts::BWFormat fmt("left >{0:<9}< right >{0:>9}< center >{0:=9}<");
  ts::string_view text{"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"};

  bw.reduce(0);
  static const ts::BWFormat bad_arg_fmt{"{{BAD_ARG_INDEX:{} of {}}}"};
  bw.print(bad_arg_fmt, 17, 23);
  REQUIRE(bw.view() == "{BAD_ARG_INDEX:17 of 23}");

  bw.reduce(0);
  bw.print(fmt, 956);
  REQUIRE(bw.view() == "left >956      < right >      956< center >   956   <");

  bw.reduce(0);
  bw.print("Text: _{0:.10,20}_", text);
  REQUIRE(bw.view() == "Text: _abcdefghijklmnopqrst_");
  bw.reduce(0);
  bw.print("Text: _{0:-<20.52,20}_", text);
  REQUIRE(bw.view() == "Text: _QRSTUVWXYZ----------_");
}

#if 0
TEST_CASE("bwperf", "[bwprint][performance]")
{
  // Force these so I can easily change the set of tests.
  auto start            = std::chrono::high_resolution_clock::now();
  auto delta = std::chrono::high_resolution_clock::now() - start;
  constexpr int N_LOOPS = 1000000;

  ts::string_view text{"Format |"};
  ts::LocalBufferWriter<256> bw;

  ts::BWFSpec spec;

  start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < N_LOOPS; ++i) {
    bw.reduce(0);
    bw.print( "Format |{:#010x}|", -956);
  }
  delta = std::chrono::high_resolution_clock::now() - start;
  std::cout << "BW Timing is " << delta.count() << "ns or " << std::chrono::duration_cast<std::chrono::milliseconds>(delta).count()
            << "ms" << std::endl;

  start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < N_LOOPS; ++i) {
    bw.reduce(0);
    bw.print("Format |{:#010x}|", -956);
  }
  delta = std::chrono::high_resolution_clock::now() - start;
  std::cout << "bw.print() " << delta.count() << "ns or " << std::chrono::duration_cast<std::chrono::milliseconds>(delta).count()
            << "ms" << std::endl;

  ts::BWFormat fmt("Format |{:#010x}|");
  start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < N_LOOPS; ++i) {
    bw.reduce(0);
    bw.print( fmt, -956);
  }
  delta = std::chrono::high_resolution_clock::now() - start;
  std::cout << "Preformatted: " << delta.count() << "ns or "
            << std::chrono::duration_cast<std::chrono::milliseconds>(delta).count() << "ms" << std::endl;

  char buff[256];
  start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < N_LOOPS; ++i) {
    snprintf(buff, sizeof(buff), "Format |%#0x10|", -956);
  }
  delta = std::chrono::high_resolution_clock::now() - start;
  std::cout << "snprint Timing is " << delta.count() << "ns or "
            << std::chrono::duration_cast<std::chrono::milliseconds>(delta).count() << "ms" << std::endl;
}
#endif
