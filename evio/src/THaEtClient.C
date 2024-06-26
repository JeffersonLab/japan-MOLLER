//////////////////////////////////////////////////////////////////////
//
//   THaEtClient
//   Data from ET Online System
//
//   THaEtClient contains normal CODA data obtained via
//   the ET (Event Transfer) online system invented
//   by the JLab DAQ group. 
//   This code works locally or remotely and uses the
//   ET system in a particular mode favored by  hall A.
//
//   Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaEtClient.h"
#include <stdio.h>
#include <time.h>
#define CODA_VERB 0

#ifndef STANDALONE
ClassImp(THaEtClient)
#endif

THaEtClient::THaEtClient() {        // Uses default mode=1 and a default server
   initflags();
   int smode = 1;
   TString defaultcomputer(ADAQ3);
   codaOpen(defaultcomputer,smode);
}
THaEtClient::THaEtClient(int smode) {      // uses default server (where CODA runs)
   initflags();
   TString defaultcomputer(ADAQ3);
   codaOpen(defaultcomputer,smode);
}

THaEtClient::THaEtClient(TString computer,int smode) {
   initflags();
   codaOpen(computer,smode);
}

THaEtClient::THaEtClient(TString computer, TString mysession, int smode) {
   initflags();
   codaOpen(computer, mysession, smode);
}

THaEtClient::THaEtClient(TString computer, TString mysession, int smode,
			 const TString stationname) {
   initflags();
   fStationName = stationname;
   codaOpen(computer, mysession, smode);
}

THaEtClient::~THaEtClient() {
   fStatus = codaClose();
   if (fStatus == CODA_ERROR) std::cout << "ERROR: closing THaEtClient"<<std::endl<<std::flush;
};

void THaEtClient::initflags()
{
   DEBUG = 0;
   FAST = 25;
   SMALL_TIMEOUT = 10;
   BIG_TIMEOUT = 45;
   initetfile = 0;
   didclose = 0;
   notopened = 0;
   firstread = 1;
   nread = 0;
   nused = 0;
   timeout = BIG_TIMEOUT;
   fStationName = "";
};

int THaEtClient::init() {
// initialize with a unique station name; therefore each client gets
// 100% of data - if possible - it still is not allowed to cause deadtime.
  if (fStationName != ""){
    return init(fStationName);
  }
  return init(uniqueStation());
};

int THaEtClient::init(TString mystation) 
{
  if (CODA_VERBOSE) 
    std::cout << "THaEtClient::init:  About to initialize ET."<<std::endl<<std::flush;
  char *station;
  station = new char[strlen(mystation.Data()+1)];
  strcpy(station,mystation.Data());
  et_open_config_init(&openconfig);
  et_open_config_sethost(openconfig, daqhost);
  et_open_config_setmode(openconfig, ET_HOST_AS_LOCAL);
  et_open_config_setcast(openconfig, ET_DIRECT);
  // Not using default port.  Instead ...
  int port=4444;
  et_open_config_setport(openconfig, port);

  if (CODA_VERBOSE) 
    std::cout << "THaEtClient::init:  Opening ET..."<<std::endl<<std::flush;
  if (et_open(&id, etfile, openconfig) != ET_OK) {
    notopened = 1;
    std::cout << "THaEtClient: cannot open ET system"<<std::endl<<std::flush;
    std::cout << "Likely causes:  "<<std::endl<<std::flush;
    std::cout << "  1. Incorrect SESSION environment variable (it can also be passed to codaOpen)"<<std::endl<<std::flush;
    std::cout << "  2. ET not running (CODA not running) on specified computer"<<std::endl<<std::flush;
    fStatus = CODA_ERROR;
    return fStatus;
  }
  if (CODA_VERBOSE) 
    std::cout << "THaEtClient::init:  about to set ET config"<<std::endl<<std::flush;
  et_open_config_destroy(openconfig);
  et_station_config_init(&sconfig);
  et_station_config_setuser(sconfig, ET_STATION_USER_MULTI);
  et_station_config_setrestore(sconfig, ET_STATION_RESTORE_OUT);
  et_station_config_setprescale(sconfig, 1);
  et_station_config_setcue(sconfig, 2*ET_CHUNK_SIZE);
  et_station_config_setselect(sconfig, ET_STATION_SELECT_ALL);
  et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);
  int status;
  if (CODA_VERBOSE) 
    std::cout << "THaEtClient::init:  creating station"<<std::endl<<std::flush;
  if ((status = et_station_create(id, &my_stat, station, sconfig)) < ET_OK) {
      if (status == ET_ERROR_EXISTS) {        
          // ok 
      }
      else if (status == ET_ERROR_TOOMANY) {
        std::cout << "THaEtClient: too many stations created"<<std::endl<<std::flush;
	fStatus = CODA_ERROR;
        return fStatus;
      }
      else if (status == ET_ERROR_REMOTE) {
        std::cout << "THaEtClient: memory or improper arg problems"<<std::endl<<std::flush;
	fStatus = CODA_ERROR;
        return fStatus;
      }
      else if (status == ET_ERROR_READ) {
        std::cout << "THaEtClient: network reading problem"<<std::endl<<std::flush;
	fStatus = CODA_ERROR;
        return fStatus;
      }
      else if (status == ET_ERROR_WRITE) {
        std::cout << "THaEtClient: network writing problem"<<std::endl<<std::flush;
	fStatus = CODA_ERROR;
        return fStatus;
      }
      else {
        std::cout << "THaEtClient: error in station creation"<<std::endl<<std::flush;
	fStatus = CODA_ERROR;
        return fStatus;
      }
  }
  et_station_config_destroy(sconfig);
  if (CODA_VERBOSE) 
    std::cout << "THaEtClient::init:  attaching station"<<std::endl<<std::flush;
  if (et_station_attach(id, my_stat, &my_att) < 0) {
    std::cout << "THaEtClient: error in station attach"<<std::endl<<std::flush;
    fStatus = CODA_ERROR;
    return fStatus;
  }
  fStatus = CODA_OK;
  return fStatus;
};

TString THaEtClient::uniqueStation() {
// Construct a unique station name from the system time.  
// Works if clients start at >1 sec separate in time.
     char *cp,*anum;
     time_t btime;
     char csta[]="----------------------------------------------";
     time(&btime);
     anum = ctime(&btime);
     int j=0;
     cp = anum;
     int size = strlen(anum) < strlen(csta) ? strlen(anum) : strlen(csta);
     while( j++ < size+1 ) {
       if(*cp!=' '&&*cp!=':') csta[j] = *cp;
       cp++;
     }
     csta[j]='\0';
     TString result = csta;
     return result;
}

int THaEtClient::codaClose() {
  fStatus = CODA_OK;
  if (didclose || firstread) 
    {
      return fStatus;
    }
  didclose = 1;
  if (notopened) 
    {
      fStatus = CODA_ERROR;
      return fStatus;
    }
  if (my_att == 0) {
     std::cout << "ERROR: codaClose: no attachment ?"<<std::endl<<std::flush;
     fStatus = CODA_ERROR; 
     return fStatus;
  }
  if (CODA_VERBOSE) 
    std::cout << "THaEtClient::codaClose:  detaching station "<<std::endl<<std::flush;
  fStatus = et_station_detach(id, my_att);
  if (fStatus != ET_OK) {
    std::cout << "ERROR: codaClose: detaching from ET, status = "<<fStatus<<std::endl<<std::flush;
    fStatus = CODA_ERROR;
  }
  if (CODA_VERBOSE) 
    std::cout << "THaEtClient::codaClose:  removing station "<<std::endl<<std::flush;
  fStatus = et_station_remove(id, my_stat);
  if (fStatus != ET_OK) {
    std::cout << "ERROR: codaClose: removing ET station, status = "<<fStatus<<std::endl<<std::flush;
    fStatus = CODA_ERROR;
  }
  if (CODA_VERBOSE) 
    std::cout << "THaEtClient::codaClose:  closing ET "<<std::endl<<std::flush;
  if (et_close(id) != ET_OK) {
    std::cout << "ERROR: codaClose: error closing ET"<<std::endl<<std::flush;
    fStatus = CODA_ERROR;
  }
  if (CODA_VERBOSE) 
    std::cout << "THaEtClient::codaClose:  all done "<<std::endl<<std::flush;
  return fStatus;
};

int THaEtClient::codaRead() {
//  Read a chunk of data, return read status (0 = ok, else not).
//  To try to use network efficiently, it actually gets
//  the events in chunks, and passes them to the user.

  static et_event *evs[ET_CHUNK_SIZE];
  struct timespec twait;
  int *data, *pdata;
  int i, j, err, status;
  int lencpy;
  size_t  nbytes;
  const size_t bpi = sizeof(int);
  int event_size;
  int swapflg;
  
// rate calculation  
  static int firstRateCalc = 1;
  static int evsum = 0, xcnt = 0;
  static time_t daqt1, daqt2, *tapt;
  static double ratesum = 0;
  double tdiff, daqrate, avgrate;

  if (firstread) {
    firstread = 0;
    status = init();
    if (status == CODA_ERROR) {
      std::cout << "THaEtClient: ERROR: codaRead, cannot connect to CODA"<<std::endl<<std::flush;
      fStatus = CODA_EXIT;
      return fStatus;
    }
  }

// pull out a ET_CHUNK_SIZE of events from ET  
  if (nused >= nread) {
    if (waitflag == 0) {  
      err = et_events_get(id, my_att, evs, ET_SLEEP, NULL, ET_CHUNK_SIZE, &nread);
    } else {
      twait.tv_sec  = timeout;
      twait.tv_nsec = 0;
      err = et_events_get(id, my_att, evs, ET_TIMED, &twait, ET_CHUNK_SIZE, &nread);
    }
    if (err < ET_OK) {
      if (err == ET_ERROR_TIMEOUT) {
	 printf("et_netclient: timeout calling et_events_get\n");
	 printf("Probably means CODA is not running...\n");
      }
      else {
	 printf("et_netclient: error calling et_events_get, %d\n", err);
      }
      nread = nused = 0;
      fStatus = CODA_EXIT;
      return fStatus;
    }
    
// reset 
    nused = 0;
    
    for (j=0; j < nread; j++) {
	
      et_event_getdata(evs[j], (void **) &data);
      et_event_needtoswap(evs[j], &swapflg);
      if (swapflg == ET_SWAP) {            
	et_event_CODAswap(evs[j]);
      }
      pdata = data;
      event_size = *pdata + 1; 
      if ( event_size > MAXEVLEN ) {
         printf("\nET:codaRead:ERROR:  Event from ET truncated\n");
         printf("-> Need a larger value than MAXEVLEN = %d \n",MAXEVLEN);
	 fStatus = CODA_ERROR;
         return fStatus;
      }
      if (CODA_DEBUG) {
  	 std::cout<<"\n\n===== Event "<<j<<"  length "<<event_size<<std::endl<<std::flush;
	 pdata = data;
         for (i=0; i < event_size; i++, pdata++) {
           std::cout<<"evbuff["<<std::dec<<i<<"] = "<<*pdata<<" = 0x"<<std::hex<<*pdata<<std::dec<<std::endl<<std::flush;
	 }
      }
    }

    if (firstRateCalc) {
      firstRateCalc = 0;
      daqt1 = time(tapt);
    }
    else {
      daqt2 = time(tapt);
      tdiff = difftime(daqt2, daqt1);
      evsum += nread;
      if ((tdiff > 4) && (evsum > 30)) {
         daqrate  = (float)evsum/tdiff;
         evsum    = 0;
         ratesum += daqrate;
         avgrate  = ratesum/++xcnt;

         if (CODA_VERBOSE) 
           printf("ET rate %4.1f Hz in %2.0f sec, avg %4.1f Hz\n",
          	      daqrate, tdiff, avgrate);
         if (waitflag != 0) {
           timeout = (avgrate > FAST) ? SMALL_TIMEOUT : BIG_TIMEOUT;
         }
         daqt1 = time(tapt);
      }
    }
  }
  
// return an event 
  et_event_getdata(evs[nused], (void **) &data);
  et_event_getlength(evs[nused], &nbytes);
  lencpy = (nbytes < bpi*MAXEVLEN) ? nbytes : bpi*MAXEVLEN;
  memcpy((void *)evbuffer,(void *)data,lencpy);
  nused++;
  if (nbytes > bpi*MAXEVLEN) {
      std::cout<<"\nET:codaRead:ERROR:  CODA event truncated"<<std::endl<<std::flush;
      std::cout<<"-> Byte size exceeds bytes "<<bpi*MAXEVLEN<<std::endl<<std::flush;
      fStatus = CODA_ERROR;
      return fStatus;
  }

// if we've used all our events, put them back 
  if (nused >= nread) {
    err = et_events_put(id, my_att, evs, nread);
    if (err < ET_OK) {
      std::cout<<"THaEtClient::codaRead: ERROR: calling et_events_put"<<std::endl<<std::flush;
      std::cout<<"This is potentially very bad !!\n"<<std::endl<<std::flush;
      std::cout<<"best not continue.... exiting... \n"<<std::endl<<std::flush;
      exit(1);
    }
  }
  fStatus = CODA_OK;
  return fStatus;

};


int* THaEtClient::getEvBuffer() {
// return the event buffer, raw 32-bit integers from CODA.
// One must do "codaRead()" first.
   return evbuffer;
};

int THaEtClient::codaOpen(TString computer, TString mysession, int smode) {
// To run codaOpen, you need to know: 
// 1) What computer is ET running on ? (e.g. computer='adaql2')
// 2) What session ? (usually env. variable $SESSION, e.g. 'onla')
// 3) mode (0 = wait forever for data,  1 = time-out in a few seconds)
    if (initetfile) delete[] etfile;
    etfile = new char[strlen(ETMEM_PREFIX)+strlen(mysession.Data())+1];
    strcpy(etfile,ETMEM_PREFIX);
    strcat(etfile,mysession.Data());
    session = new char[strlen(mysession.Data())+1];
    strcpy(session,mysession.Data());
    initetfile = 1;
    fStatus = codaOpen(computer, smode);
    return fStatus;
};

int THaEtClient::codaOpen(TString computer) {
// See comment in the above version of codaOpen()
     int mymode = 1;
     fStatus = codaOpen(computer, mymode);
     return fStatus;
};

int THaEtClient::codaOpen(TString computer, int smode) {
// See comment in the above version of codaOpen()
     daqhost = new char[strlen(computer.Data())+1];
     strcpy(daqhost,computer.Data());
     waitflag = smode;
     if (!initetfile) {  // construct et memory file from $SESSION env variable.
       initetfile = 1;
       char *s;
       s = getenv("SESSION");
       if (s == NULL) {
	 fStatus = 1;
         return fStatus;
       } else {
         session = new char[strlen(s)+1];
         strcpy(session,s);
       }
       etfile = new char[strlen(ETMEM_PREFIX)+strlen(session)+1];
       strcpy(etfile,ETMEM_PREFIX);
       strcat(etfile,session);
     }
     fStatus = 0;
     return fStatus;
};

int THaEtClient::getheartbeat() {
// Just gets the heartbeat of the ET server

  int status;
  if(firstread) {
    status = init("hbstation");
    if(status == CODA_ERROR) {
      std::cout << "THaEtClient: ERROR: cannot connect to CODA"<<std::endl<<std::flush;
      firstread=1;
      return 0;
    }
  }
  firstread=0;

  int heartbeat;
  status = et_system_getheartbeat(id,&heartbeat);
  //  codaClose();

  if(status == ET_OK) {
    return heartbeat;
  } else if (status == ET_ERROR) {    
    std::cout << "THaEtClient: ERROR: heartbeat is NULL" << std::endl;
    return 0;
  } else if (status == ET_ERROR_READ) {
    firstread=1;
    std::cout << "THaEtClient: ERROR: Remote user's network read error" << std::endl;
    return 0;
  } else if (status == ET_ERROR_WRITE) {
    firstread=1;
    std::cout << "THaEtClient: ERROR: Remote user's network write error" << std::endl;
    return 0;
  }

  return 0;
};
