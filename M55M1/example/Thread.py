import _thread
import pyb
import time
 
def testThread():
  while True:
    print("Hello from thread---1")
    time.sleep_ms(1000)
 
def testThread2():
  while True:
    print("Hello from thread---2")
    time.sleep_ms(1000)

_thread.start_new_thread(testThread, ())
_thread.start_new_thread(testThread2, ())
