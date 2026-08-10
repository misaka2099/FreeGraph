#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <vector>
#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_stream.h"
using namespace std;

namespace hls::sim
{
  template<size_t n>
  struct Byte {
    unsigned char a[n];

    Byte()
    {
      for (size_t i = 0; i < n; ++i) {
        a[i] = 0;
      }
    }

    template<typename T>
    Byte<n>& operator= (const T &val)
    {
      std::memcpy(a, &val, n);
      return *this;
    }
  };

  struct SimException : public std::exception {
    const char *msg;
    const size_t line;
    SimException(const char *msg, const size_t line)
      : msg(msg), line(line)
    {
    }
  };

  void errExit(const size_t line, const char *msg)
  {
    std::string s;
    s += "at line ";
    s += std::to_string(line);
    s += " occurred problem: ";
    s += msg;
    s += "\n";
    fputs(s.c_str(), stderr);
    exit(1);
  }
}


namespace hls::sim
{
  struct Buffer {
    char *first;
    Buffer(char *addr) : first(addr)
    {
    }
  };

  struct DBuffer : public Buffer {
    static const size_t total = 1<<10;
    size_t ufree;

    DBuffer(size_t usize) : Buffer(nullptr), ufree(total)
    {
      first = new char[usize*ufree];
    }

    ~DBuffer()
    {
      delete[] first;
    }
  };

  struct CStream {
    char *front;
    char *back;
    size_t num;
    size_t usize;
    std::list<Buffer*> bufs;
    bool dynamic;

    CStream() : front(nullptr), back(nullptr),
                num(0), usize(0), dynamic(true)
    {
    }

    ~CStream()
    {
      for (Buffer *p : bufs) {
        delete p;
      }
    }

    template<typename T>
    T* data()
    {
      return (T*)front;
    }

    template<typename T>
    void transfer(hls::stream<T> *param)
    {
      while (!empty()) {
        param->write(*(T*)nextRead());
      }
    }

    bool empty();
    char* nextRead();
    char* nextWrite();
  };

  bool CStream::empty()
  {
    return num == 0;
  }

  char* CStream::nextRead()
  {
    assert(num > 0);
    char *res = front;
    front += usize;
    if (dynamic) {
      if (++static_cast<DBuffer*>(bufs.front())->ufree == DBuffer::total) {
        if (bufs.size() > 1) {
          bufs.pop_front();
          front = bufs.front()->first;
        } else {
          front = back = bufs.front()->first;
        }
      }
    }
    --num;
    return res;
  }

  char* CStream::nextWrite()
  {
    if (dynamic) {
      if (static_cast<DBuffer*>(bufs.back())->ufree == 0) {
        bufs.push_back(new DBuffer(usize));
        back = bufs.back()->first;
      }
      --static_cast<DBuffer*>(bufs.back())->ufree;
    }
    char *res = back;
    back += usize;
    ++num;
    return res;
  }

  std::list<CStream> streams;
  std::map<char*, CStream*> prebuilt;

  CStream* createStream(size_t usize)
  {
    streams.emplace_front();
    CStream &s = streams.front();
    {
      s.dynamic = true;
      s.bufs.push_back(new DBuffer(usize));
      s.front = s.bufs.back()->first;
      s.back = s.front;
      s.num = 0;
      s.usize = usize;
    }
    return &s;
  }

  template<typename T>
  CStream* createStream(hls::stream<T> *param)
  {
    CStream *s = createStream(sizeof(T));
    {
      s->dynamic = true;
      while (!param->empty()) {
        T data = param->read();
        memcpy(s->nextWrite(), (char*)&data, sizeof(T));
      }
      prebuilt[s->front] = s;
    }
    return s;
  }

  template<typename T>
  CStream* createStream(T *param, size_t usize)
  {
    streams.emplace_front();
    CStream &s = streams.front();
    {
      s.dynamic = false;
      s.bufs.push_back(new Buffer((char*)param));
      s.front = s.back = s.bufs.back()->first;
      s.usize = usize;
      s.num = ~0UL;
    }
    prebuilt[s.front] = &s;
    return &s;
  }

  CStream* findStream(char *buf)
  {
    return prebuilt.at(buf);
  }
}
class AESL_RUNTIME_BC {
  public:
    AESL_RUNTIME_BC(const char* name) {
      file_token.open( name);
      if (!file_token.good()) {
        cout << "Failed to open tv file " << name << endl;
        exit (1);
      }
      file_token >> mName;//[[[runtime]]]
    }
    ~AESL_RUNTIME_BC() {
      file_token.close();
    }
    int read_size () {
      int size = 0;
      file_token >> mName;//[[transaction]]
      file_token >> mName;//transaction number
      file_token >> mName;//pop_size
      size = atoi(mName.c_str());
      file_token >> mName;//[[/transaction]]
      return size;
    }
  public:
    fstream file_token;
    string mName;
};
unsigned int ap_apatb_online_in_cap_bc;
static AESL_RUNTIME_BC __xlx_online_in_V_size_Reader("../tv/stream_size/stream_size_in_online_in.dat");
unsigned int ap_apatb_pre_expand_out_cap_bc;
static AESL_RUNTIME_BC __xlx_pre_expand_out_V_size_Reader("../tv/stream_size/stream_size_out_pre_expand_out.dat");
unsigned int ap_apatb_edge_out_cap_bc;
static AESL_RUNTIME_BC __xlx_edge_out_V_size_Reader("../tv/stream_size/stream_size_out_edge_out.dat");
unsigned int ap_apatb_pre_ack_in_cap_bc;
static AESL_RUNTIME_BC __xlx_pre_ack_in_V_size_Reader("../tv/stream_size/stream_size_in_pre_ack_in.dat");
unsigned int ap_apatb_ack_in_cap_bc;
static AESL_RUNTIME_BC __xlx_ack_in_V_size_Reader("../tv/stream_size/stream_size_in_ack_in.dat");
using hls::sim::Byte;
struct __cosim_s16__ { char data[16]; };
struct __cosim_s68__ { char data[68]; };
struct __cosim_s4__ { char data[4]; };
struct __cosim_s128__ { char data[128]; };
extern "C" void task_reader_kernel(int, __cosim_s16__*, __cosim_s68__*, __cosim_s16__*, __cosim_s4__*, __cosim_s4__*);
extern "C" void apatb_task_reader_kernel_hw(int __xlx_apatb_param_worker_id, volatile void * __xlx_apatb_param_online_in, volatile void * __xlx_apatb_param_pre_expand_out, volatile void * __xlx_apatb_param_edge_out, volatile void * __xlx_apatb_param_pre_ack_in, volatile void * __xlx_apatb_param_ack_in) {
using hls::sim::createStream;
auto* sonline_in = createStream((hls::stream<__cosim_s16__>*)__xlx_apatb_param_online_in);
  //Create input buffer for pre_expand_out
  ap_apatb_pre_expand_out_cap_bc = __xlx_pre_expand_out_V_size_Reader.read_size();
  __cosim_s68__* __xlx_pre_expand_out_input_buffer= new __cosim_s68__[ap_apatb_pre_expand_out_cap_bc];
auto* spre_expand_out = createStream((hls::stream<__cosim_s68__>*)__xlx_apatb_param_pre_expand_out);
  //Create input buffer for edge_out
  ap_apatb_edge_out_cap_bc = __xlx_edge_out_V_size_Reader.read_size();
  __cosim_s16__* __xlx_edge_out_input_buffer= new __cosim_s16__[ap_apatb_edge_out_cap_bc];
auto* sedge_out = createStream((hls::stream<__cosim_s16__>*)__xlx_apatb_param_edge_out);
auto* spre_ack_in = createStream((hls::stream<__cosim_s4__>*)__xlx_apatb_param_pre_ack_in);
auto* sack_in = createStream((hls::stream<__cosim_s4__>*)__xlx_apatb_param_ack_in);
  // DUT call
  task_reader_kernel(__xlx_apatb_param_worker_id, sonline_in->data<__cosim_s16__>(), spre_expand_out->data<__cosim_s68__>(), sedge_out->data<__cosim_s16__>(), spre_ack_in->data<__cosim_s4__>(), sack_in->data<__cosim_s4__>());
sonline_in->transfer((hls::stream<__cosim_s16__>*)__xlx_apatb_param_online_in);
spre_expand_out->transfer((hls::stream<__cosim_s68__>*)__xlx_apatb_param_pre_expand_out);
sedge_out->transfer((hls::stream<__cosim_s16__>*)__xlx_apatb_param_edge_out);
spre_ack_in->transfer((hls::stream<__cosim_s4__>*)__xlx_apatb_param_pre_ack_in);
sack_in->transfer((hls::stream<__cosim_s4__>*)__xlx_apatb_param_ack_in);
}
