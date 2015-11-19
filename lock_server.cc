// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <map>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "rpc/slock.h"

lock_server::lock_server():
  nacquire (0)
{
  pthread_mutex_init(&server_mutex, NULL); 
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{
  ScopedLock s(&server_mutex);

  lock_protocol::status ret = lock_protocol::OK;
  printf("acquire request from clt %d\n", clt);
  r = nacquire;
  //fetch from map and judge
  //std::map<lock_protocol::lockid_t, pthread_cond_t>::iterator it= locks.find(lid);
  auto it = locks.find(lid);
  if(it == locks.end()) {
  	//cant find and granted
  	lock_status stat;
    stat.cond = PTHREAD_COND_INITIALIZER;
    stat.clt = clt;
    //pthread_cond_init(&cond, NULL);
  	locks.insert(std::make_pair(lid, stat));
  } else {
  	//check which acquired
  	if (it->second.clt > 0 && it->second.clt != clt){
	  	//wait
	  	printf("client %d waiting for busy lock %lld\n", clt, lid);
	    while(it->second.clt > 0) {
	      pthread_cond_wait(&(it->second.cond), &server_mutex);
        printf("clnt %d request wake up\n", clt);
	    }
    }
    printf("clt %d got lock %lld\n", clt, lid);
    //race ?
    it->second.clt = clt;

  }

  return ret;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
  ScopedLock s(&server_mutex);
  
  lock_protocol::status ret = lock_protocol::OK;
  printf("release request from clt %d\n", clt);
  r = nacquire;
  auto it = locks.find(lid);
  if(it != locks.end() && it->second.clt > 0) {
  	it->second.clt = -1;
  	if(pthread_cond_broadcast(&(it->second.cond)))
      ret = lock_protocol::RETRY;
  }

  return ret;
}


