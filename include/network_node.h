#pragma once

#include <zmq.hpp>
#include <string>
#include <iostream>
#include <unistd.h>
#include <unordered_map>
#include <fstream>
#include <errno.h>
#include <sstream>

class Network_Node {
public:
  int sid;  // server-id in [0, nsrvs)
  int wid;  // worker-id in [0, nwkrs)
  zmq::context_t context;
  zmq::socket_t* receiver;

  std::vector<std::string> ipset;
  std::unordered_map<int, zmq::socket_t*> senders;

  inline int code(int _sid, int _wid) {
    return _sid * 200 + _wid;
  }

  Network_Node(int _sid, int _wid, std::string fname)
    : sid(_sid), wid(_wid), context(1) {

    std::ifstream hostfile(fname);
    std::string ip;

    while (hostfile >> ip)
      ipset.push_back(ip);

    receiver = new zmq::socket_t(context, ZMQ_PULL);
    char address[30] = "";
    sprintf(address, "tcp://*:%d", 5500 + code(_sid, _wid));
    //fprintf(stdout, "tcp binding address %s\n", address);
    receiver->bind(address);
  }

  ~Network_Node() {
    for (auto iter : senders) {
      if (iter.second != NULL) {
        delete iter.second;
        iter.second = NULL;
      }
    }
    delete receiver;
  }

  std::string ip_of(int _sid) {
    return ipset[_sid];
  }

  void Send(int _sid, int _wid, std::string msg) {
    int id = code(_sid, _wid);

    if (senders.find(id) == senders.end()) {
      senders[id] = new zmq::socket_t(context, ZMQ_PUSH);

      char address[30] = "";
      snprintf(address, 30, "tcp://%s:%d", ipset[_sid].c_str(), 5500 + id);
      //fprintf(stdout,"mul estalabish %s\n",address);
      senders[id]->connect(address);
    }
    zmq::message_t request(msg.length());
    memcpy ((void *)request.data(), msg.c_str(), msg.length());
    senders[id]->send(request);
  }

  std::string Recv() {
    zmq::message_t reply;
    if (receiver->recv(&reply) < 0) {
      fprintf(stderr, "recv with error %s\n", strerror(errno));
      exit(-1);
    }
    return std::string((char *)reply.data(), reply.size());
  }

  std::string tryRecv() {
    zmq::message_t reply;
    if (receiver->recv(&reply, ZMQ_NOBLOCK))
      return std::string((char *)reply.data(), reply.size());
    else
      return "";
  }
};
