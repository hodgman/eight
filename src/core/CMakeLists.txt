# Core library
 
BeginLibrary( core )

AddSourceGroup( core
core.cpp
core_test.cpp
align.cpp
sort.cpp
test.cpp
types.h
id.h
traits.h
debug.h
throw.h
macro.h
align.h
performance.h
math/arithmetic.h
math/tmp.h
application.h
test.h
hash.h
noncopyable.h
variablestream.h
variablestream.hpp
bytestream.h
bytestream.hpp
typeinfo.h
bind.cpp
bind.h
message.cpp
message.h
 )
 
AddSourceGroup( Alloc 
alloc/stack.cpp
alloc/new.h
alloc/stack.h
alloc/stack.hpp
alloc/scope.h
alloc/scope.hpp
alloc/scopearray.h
alloc/pool.h 
alloc/malloc.h 
 )
 
AddSourceGroup( Blob  
blob/loader_win.cpp
blob/asset.cpp
blob/asset.h
blob/assetscope.h
blob/blobfactory.h
blob/loader.h
blob/loader_win.h
blob/statichash.h
blob/types.h 
 )
 
AddSourceGroup( Sort   
sort/radix.h
sort/radix.hpp
 )
 
AddSourceGroup( Thread 
thread/thread.cpp
thread/taskloop.cpp
thread/taskloop.h
thread/timedloop.cpp
thread/timedloop.h
thread/thread.cpp
thread/threadlocal.cpp
thread/pool.cpp
thread/task.cpp
thread/task_test.cpp
thread/tasksection.cpp
thread/tasksection_test.cpp
thread/atomic.cpp
thread/futex.cpp
thread/jobqueue.cpp
thread/fifo_spsc.cpp
thread/fifo_mpmc.cpp
thread/latentpipe.cpp
thread/latent.h
thread/thread.h
thread/pool.h
thread/task.h
thread/tasksection.h
thread/gate.h
thread/threadlocal.h
thread/hashtable.h
thread/atomic.h
thread/atomic.hpp
thread/futex.h
thread/jobqueue.h
thread/fifo_spsc.h
thread/fifo_spsc.hpp
thread/fifo_mpmc.h
thread/fifo_mpmc.hpp
thread/latentpipe.h
thread/latentpipe.hpp
 )
 
AddSourceGroup( Bit   
bit/twiddling.cpp 
bit/twiddling.h
 )
 
AddSourceGroup( OS   
os/window_win.cpp 
os/win32.h
os/window.h
os/window_impl.h 
 )
 
AddSourceGroup( Timer  
timer/timer.h
timer/timer_impl.h
timer/timer.cpp
../../external/timer_lib/timer.c
../../external/timer_lib/timer.h
 )
 
AddSourceGroup( RDE  
../../external/rdestl/allocator.cpp
../../external/rdestl/intrusive_list.cpp
../../external/rdestl/intrusive_slist.cpp
../../external/rdestl/list.cpp
../../external/rdestl/slist.cpp
../../external/rdestl/algorithm.h
../../external/rdestl/alignment.h
../../external/rdestl/allocator.h
../../external/rdestl/basic_string.h
../../external/rdestl/buffer_allocator.h
../../external/rdestl/cow_string_storage.h
../../external/rdestl/fixed_array.h
../../external/rdestl/fixed_list.h
../../external/rdestl/fixed_sorted_vector.h
../../external/rdestl/fixed_substring.h
../../external/rdestl/fixed_vector.h
../../external/rdestl/functional.h
../../external/rdestl/hash.h
../../external/rdestl/hash_map.h
../../external/rdestl/intrusive_list.h
../../external/rdestl/intrusive_slist.h
../../external/rdestl/int_to_type.h
../../external/rdestl/iterator.h
../../external/rdestl/list.h
../../external/rdestl/map.h
../../external/rdestl/pair.h
../../external/rdestl/radix_sorter.h
../../external/rdestl/rb_tree.h
../../external/rdestl/rdestl.h
../../external/rdestl/rdestl_common.h
../../external/rdestl/rde_string.h
../../external/rdestl/set.h
../../external/rdestl/simple_string_storage.h
../../external/rdestl/slist.h
../../external/rdestl/sort.h
../../external/rdestl/sorted_vector.h
../../external/rdestl/sstream.h
../../external/rdestl/stack.h
../../external/rdestl/stack_allocator.h
../../external/rdestl/string_utils.h
../../external/rdestl/type_traits.h
../../external/rdestl/utility.h
../../external/rdestl/vector.h  
 )

EndLibrary()

AddTest( test_core test_app.cpp core )
 