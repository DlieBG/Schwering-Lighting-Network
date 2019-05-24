// fltk_threads.h
//
// merged from FL_API.H and Threads.h of FLTK Version 2
// for use with FLTK version 1.3 or others
// few changes by Lutz alias ilLUTZminator
//
//
// This file is included by all FLTK header files. Originally it was to
// get that damn dllimport/export stuff on Windows. It now also turns
// off warnings on Windows so that you can use stdc functions, and
// defines the uchar type that is used by FLTK a lot.
//
// Copyright 1998-2006 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems to "fltk-bugs@fltk.org".
#ifndef FLTK_THREADS
#define FLTK_THREADS

#if _MSC_VER > 1000
# pragma once
// suppress vc2005 warnings: 
// C4312 because we have no problem to cast a smaller type to a greater (potentially 64bit) one
// and C4996 (deprecated C-lib calls like strcpy that should be _strcpy, etc ...)
# pragma warning(disable:4312 4996)
#endif

#if !defined(DOXYGEN) && defined(_WIN32) && defined(FL_SHARED)
#  ifdef FL_LIBRARY
#    define FL_API	__declspec(dllexport)
#  else
#    define FL_API	__declspec(dllimport)
#  endif
#  ifdef FL_IMAGES_LIBRARY
#    define FL_IMAGES_API	__declspec(dllexport)
#  else
#    define FL_IMAGES_API	__declspec(dllimport)
#  endif
#  ifdef FL_GLUT_LIBRARY
#    define FL_GLUT_API		__declspec(dllexport)
#  else
#    define FL_GLUT_API		__declspec(dllimport)
#  endif
#  ifdef FL_FORMS_LIBRARY
#    define FL_FORMS_API	__declspec(dllexport)
#  else
#    define FL_FORMS_API	__declspec(dllimport)
#  endif
#  ifdef FL_GL_LIBRARY
#    define FL_GL_API		__declspec(dllexport)
#  else
#    define FL_GL_API		__declspec(dllimport)
#  endif
#else
#  define FL_API
#  define FL_IMAGES_API
#  define FL_GLUT_API
#  define FL_FORMS_API
#  define FL_GL_API
#endif

typedef unsigned char uchar;

//#endif // FLTK_THREADS

//
// End of "$Id: FL_API.h 5448 2006-09-19 01:14:07Z spitzak $".
//




//#ifndef fltk_Threads_h
//#define fltk_Threads_h

#if !defined( _WIN32) || defined(__CYGWIN__)
// pthreads:

#include <pthread.h>

//namespace fltk {

/// \name fltk/Threads.h
//@{

/** Hides whatever the system uses to identify a thread. Used so
  the "toy" interface is portable. */
typedef pthread_t Thread;

/** Fork a new thread and make it run \a f(p). Returns negative number
  on error, otherwise \a t is set to the new thread. */
inline int create_thread(Thread& t, void *(*f) (void *), void* p) {
  return pthread_create((pthread_t*)&t, 0, f, p);
}

/**
  "Mutual-exclusion lock" for simple multithreaded programs.  Calling
  lock() will wait until nobody else has the lock and then will
  return. <i>Calling lock() more than once will "deadlock"!</i>
  To avoid this, use RecursiveMutex.
*/
class Mutex {
  friend class SignalMutex;
  pthread_mutex_t mutex;
  Mutex(const Mutex&);
  Mutex& operator=(const Mutex&);
protected:
  Mutex(const pthread_mutexattr_t* a) {pthread_mutex_init(&mutex, a);}
public:
  Mutex() {pthread_mutex_init(&mutex, 0);}
  void lock() {pthread_mutex_lock(&mutex);}
  void unlock() {pthread_mutex_unlock(&mutex);}
  bool trylock() {return pthread_mutex_trylock(&mutex) == 0;}
  ~Mutex() {pthread_mutex_destroy(&mutex);}
};

/**
  A portable "semaphore". A thread that holds this lock() can call
  wait(), which will unlock it, then wait for another thread to
  call signal(), then lock() it again.

  The other thread can call signal() at any time, though usually
  it will have called lock() as well, as the lock can be used to
  protect the data that is actually being shared between the threads.

  If more than one thread is in wait(), then calling signal_one()
  will only wake one of them up. This may be more efficient, and
  can be done safely if all threads that call wait() also call
  signal_one() just before calling unlock().

  Warning: wait() can return even if signal() was not called. You
  must then check other data (protected by the lock()) to see if
  the condition really is fulfilled. In many cases this is the
  best implementation, it is also necessary to work around design
  errors in Windows, where always returns after 1/2 second to
  avoid a deadlock due to the non-atomic nature of Windows calls.
*/
class SignalMutex : public Mutex {
  pthread_cond_t cond;
public:
  SignalMutex() : Mutex() {pthread_cond_init(&cond, 0);}
  void signal() {pthread_cond_broadcast(&cond);}
  void signal_one() {pthread_cond_signal(&cond);}
  void wait() {pthread_cond_wait(&cond, &mutex);}
};

// Linux supports recursive locks, use them directly, with some cheating:
#if defined(PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP) || defined(PTHREAD_MUTEX_RECURSIVE)

class RecursiveMutex : public Mutex {
public:
  RecursiveMutex();
};

#else // standard pthread mutexes need a bit of work to be recursive:

/**
  "Mutual exclusion lock" to protect data in multithreaded programs.
  This is a "recursive lock". Calling lock() will wait until nobody
  else has the lock and then will take it. Calling lock() multiple
  times by the same thread is allowed, and unlock() must then be
  called the same number of times before another thread can get the
  lock.
*/
class RecursiveMutex : public Mutex {
  pthread_t owner;
  int counter;
public:
  RecursiveMutex() : Mutex(), counter(0) {}
  void lock() {
    if (!counter || owner != pthread_self()) {
      Mutex::lock();
      owner = pthread_self();
      counter = 1;
    } else {
      ++counter;
    }
  }
  bool trylock() {
    if (!counter || owner != pthread_self()) {
      if (!Mutex::trylock()) return false;
      owner = pthread_self();
    }
    counter++;
    return true;
  }
  void unlock() {if (!--counter) Mutex::unlock();}
};

#endif

#else // _WIN32:

/*
# define _WIN32_WINNT 0x0500
*/
# include <windows.h>
# include <process.h>
/*
// undefine some of the more annoying crap:
# undef DELETE
# undef ERROR
# undef IN
# undef OUT
# undef POINT
# undef far
# undef max
# undef min
# undef near
*/

//namespace fltk {

typedef unsigned long Thread;

inline int create_thread(Thread& t, void *(*f) (void *), void* p) {
  return t = (Thread)_beginthread((void( __cdecl * )( void * ))f, 0, p);
}

class FL_API Mutex {
  CRITICAL_SECTION cs;
  Mutex(const Mutex&);
  Mutex& operator=(const Mutex&);
public:
  Mutex() {InitializeCriticalSection(&cs);}
  void lock() {while (!TryEnterCriticalSection(&cs)) SwitchToThread();}
  void unlock() {LeaveCriticalSection(&cs);}
  bool trylock() {return TryEnterCriticalSection(&cs);}
  ~Mutex() {DeleteCriticalSection(&cs);}
};

// After many experiments we have determined that this very stupid
// implementation has the lowest overhead:
class FL_API SignalMutex : public Mutex {
public:
  SignalMutex() : Mutex() {}
  void signal() {}
  void signal_one() {}
  void wait() {
    // the following three calls should be atomic, sigh...
    unlock();
    SwitchToThread();
    lock();
  }
};

typedef Mutex RecursiveMutex;

#endif

/**
   C++ convienence object for locking a Mutex.
   Creating a local one of these will lock() the mutex and it means
   unlock() will be called no matter how a function exits, because
   the destructor ~Guard() does an unlock().

\code
   static fltk::Mutex mutex;
   function() {
     fltk::Guard guard(mutex);
     do_stuff;
     throw_exceptions;
     if (test()) return;
     etc;
   }
\endcode

*/
class FL_API Guard {
  Mutex& lock;
 public:
  Guard(Mutex& m) : lock(m) {lock.lock();}
  Guard(Mutex* m) : lock(*m) {lock.lock();}
  ~Guard() {lock.unlock();}
};

//@}

//}

#endif // FLTK_THREADS
 
