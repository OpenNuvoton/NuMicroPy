import _thread
import pyb
 
def testThread():
  while True:
    print("Hello from thread---1")
    pyb.delay(1000)
 
def testThread2():
  while True:
    print("Hello from thread---2")
    pyb.delay(1000)

_thread.start_new_thread(testThread, ())
_thread.start_new_thread(testThread2, ())
