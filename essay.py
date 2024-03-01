Exploring Asynchronous in CUDA Streams

Data transfer between CPU and GPU can introduce significant overheads in GPU computing.
This is because the CPU and GPU have separate memory spaces, and data must be explicitly
Also, the bandwidth of the PCIe bus that connects the CPU and GPU is limited.
Besides, data transfer can also impact the overall performance, 
as the GPU may remain idle while waiting for data, 
undermining the potential speedup offered by GPU acceleration.

A CUDA stream is a queue of GPU operations that are executed in a specific order. 
Operations within a single stream are guaranteed to execute in the order they were issued, 
but CUDA also offers ways to synchronize between streams (e.g., cudaStreamWaitEvent) 
to manage dependencies across different sequences of operations.
Asynchronous data transfer can be achieved by using GPU streams 
(e.g., CUDA Streams), which could hide memory access latency from GPUs to main 
memory and so utilize GPU’s computing power more.
Many works have shown that using CUDA streams can improve the performance of GPU computing
by overlapping data transfer and computation.

CUDA streams share many similarities with POSIX threads.
Both CUDA streams and POSIX threads execute a sequence of operations in a specific order.
Works in a CUDA stream are executed in the order they were issued, but 
works in multiple streams can be executed concurrently.
Also, all streams can access the same memory space, and they could communicate with each other
by using some synchronization primitives, such as events and barriers.

One problem with CUDA streams is that programmers need to manually synchronize
streams to ensure that the work is done.
To be more specific, the programmer needs to insert cudaStreamSynchronize
to wait for the completion of the work in the stream.
This can be error-prone and can lead to performance issues.
While in some languages with a good support for asynchronous programming (e.g., Python, Rust),
programmers can use async/await to write asynchronous code,
which can be more readable and maintainable.

Owing to the asynchronous nature of the streams model, 
porting existing code to streams can be challenging.
Many existing codes do not have a natural support of streams,
and the asynchronous nature of the streams model can break many assumptions.
Though some works aim to hide the complexity of streams from the programmer,
they mainly focus on kernel-level programming, and the support for
lower-level programming is limited.
For example, a programmer may want to perform an operation with a single thread, warp, or block,
and it is not clear how to do this with streams.

Even though some libraries are designed to work with streams,
because of these difficulties, they still lack flexibility and generality.
Many important GPU libraries like matrix multiplication, all-reduce, and graph algorithms,
are heavily optimized.
However, a library designed to work with blocks probably does not work well with warps.
So for some applications, it is still necessary to write custom kernels from scratch.

To sum up, the asynchronous nature of the streams model makes porting existing code to streams can be challenging.
While some libraries are designed to work with streams,
it is still difficult to work them with any arbitrary code.
In the next section, we will discuss more
about the potential difficulties in using asynchronous CUDA streams in detail.




The asynchronous nature of the streams model breaks the
assumption of atomicity. 
Because of the asynchronous nature of the streams model,
many assumptions that are valid in a synchronous model are no longer valid.
One of them is the assumption of atomicity.
In a synchronous situation, a caller of a routine assumes the callee is done 
executing after it returns and the result.
However, if the callee is implemented using streams,
it could be the case that the routine has not even started executing yet.
This could lead to subtle bugs and unexpected behavior.

Another problem is that the asynchronous nature of the streams model
breaks the assumption of determinism.
The works are not guaranteed to be executed in a specific order.


Based on my own experience, I have encountered some difficulties when integrating CUDA kernels to a system.





Use streams internally but synchronize after every GPU operation. 
This is both safe and well- encapsulated: the user can trust that 
results are im- mediately available (just like on the CPU) but it is inefficient. 
Each call, no matter how compute- intensive, must pay the full cost of establishing coherence, 
and synchronizing stifles any opportunity for hiding latency.
This approach is commonly taken by ostensibly GPU-aware Message Passing Interface (MPI) 
imple- mentations, which may directly take device buffers as MPI routine arguments. 
For example, to imple- ment MPI_Allreduce() with device buffers, the im- plementation 
must synchronize its internal streams before returning control to the user in order to 
ensure the result is valid.



Another approach is to use streams internally and synchronize only when necessary.
This is more efficient, but it is difficult to get right.
For example, Thrust uses this to implement its asynchronous algorithms.
But there is a steep learning curve to use it correctly, and it is easy to make mistakes.
For example, for thrust async algorithms,
like reduce, copy_if, transform_reduce are blocking, because D2H copy is required before returning result to the calling thread,
so programmers don't need to call any synchronization primitives to ensure the result is valid.
While for_each is non-blocking, the return value can be, and is computed on the host, 
and no D2H copy is required before returning control to the calling thread.
Without much experience, it is difficult to clearly know which algorithm calls
include copies that are blocking and which are not.
Also, thrust developers also recommend
if an asynchronous execution is desired, the best bet would be to spawn a thread 
which will call thrust algorithm, which further demonstrates the difficulty of porting to asynchronous streams correctly.

# 
H2D, D2D and D2H copies are implemented via cudaMemCopyAsync+sync in the provided stream. 
Thus copies are blocking.
If asynchronous execution is desired, 
the best bet would be to spawn a thread, e.g. via async, which will call Thrust algorithm. 
Please be mindful most of the Thrust algorithms allocate temporary storage. If you use multiple threads to invoke Thrust algorithms, it is best to provide a custom thread-safe allocator (e.g. [1]) via the execution policy (e.g. [2]).


For example, when using 
Since thrust::future extracts the stream from the event in the first copy of the policy, 

This happens because the one-policy overload of async::copy copies its single policy argument and 
uses it as both the "to" and "from" policy when 
calling the two-policy overload of async::copy. 
This is problematic because the stream-stealing code 
in thrust::future extracts the stream from the event in the first copy of the policy, leaving the second copy in a moved-from state, leading to the no_state exception.

no_state: an operation that requires an event or future to have a stream 
or content has been performed on a event or future without either, e.g. a moved-from or default constructed event or future (anevent or future may have been consumed more than once)
  
Thrust exposes a selection of algorithms under the thrust::async,
which all return a thrust::device_event.

Return event objects that users can synchronize themselves.
This allows the user more fine-grained control as they may defer synchronization. For exam- ple, 
SYCL [5] and Thrust [6] support this approach. Calling the submit() method on a SYCL queue
object will return a SYCL event for users to wait on. Thrust exposes a selection of algorithms 
under the thrust::async namespace which all return a thrust::device_event.


While more performant, this approach is nevertheless unsavory. 
The caller now needs to store the returned events, or pass them further up the call chain, 
leaking implementation details. It is also difficult to compose such an approach with 
multiple cooperat- ing libraries, which might all have their own flavor of events. 
In general, this strategy makes generic code harder to write as the myriad of events 
pollutes the business logic.
For example, the aforementioned thrust:: device_events call cudaEventSynchronize() in 
their destructor, forcing the caller to store the events to avoid this. Since a library 
cannot know a priori when the user actually wants to synchronize, they have no choice but 
to either return the events, destroy them immediately, or store them indefinitely.



To solve this, one way is to synchronize the streams after every GPU operation.
This is safe. 
Programmers know that the results are immediately available, just like
serial code.
However, it is inefficient.
All GPU operations must pay the cost of coherence.
For example, many distributed training frameworks based on MPI
use this approach.
To implement allreduce() with device buffers, the implementation
must synchronize its internal streams before returning 
control to the user in order to ensure the result is valid.


Madrona is a research game engine designed specifically 
for creating learning environments that execute with extremely high throughput on a 
single GPU.
Programmers can write code without worrying about parallelism,
and the system will take care of the rest.
In Madrona, each Entity Component System (ECS) function will be
compiled into a node with batched threads.
A task graph will be created based on these nodes and data dependencies.
For one step of simulation, the task graph will be invoked once and 
only returns after synchronization with the GPU is complete.
The system is designed based on the first approach mentioned above.
The interface of Madrona is designed to be single-threaded,
and the system will launch a fixed number of threads for each function in one node.
Thus it is not easy to work with external CUDA kernels.
For example, if the programmer wants to use a custom CUDA kernel,
they need to create a custom node that can be connected to the task graph,
cast the input data to the correct type, and then execute the kernel
as a whole block of threads instead of a single thread.
Obviously, this is not easy to do.


launched a fixed number of threads for each function in one node, and
it's  for simplicity.


And then these nodes will be connected into a graph based on the data dependencies,
finally executed 

// Run one invocation of the task graph across all worlds (one step)
    // Only returns after synchronization with the GPU is complete (not async)
    
    
Then these nodes will be connected into a graph based on the data dependencies,
and compiled into a 
and then executed on the GPU.
Madrona would compiles each ECS function into a 
each node in the graph is represented by a thread,
and 
and then execute the graph on the GPU, either with or without synchronization.
We can conclude that the system is designed to work with GPU streams,
however it is still difficult to work with external code.
For example, if the programmer wants to use cuda kernels,


The essay first introduces GPU streams as a solution that enables 
asynchronous operations, thus optimizing performance by allowing computation 
and data transfers to occur simultaneously. 
Then it discusses the challenges of integrating GPU streams into existing codes,
highlighting the manual synchronization required, the steep learning curve for
adopting asynchronous streams, and the potential necessity to develop custom kernels from scratch.
The essay emphasizes the difficulties of porting code to GPU streams due to the asynchronous model's complexities and
and the technical barriers for seamless integration with established codes and libraries.
Most of the illustrative examples are from the our own experience,
demonstrating the nuanced challenges developers face 
in leveraging GPU streams.


By providing illustrative examples, the essay


It underscores the technical barriers 
and the lack of flexibility and generality in some 
libraries designed for stream integration, 
illustrating the nuanced challenges developers face 
in leveraging GPU streams for performance gains.

Through illustrative examples, it underscores the technical barriers and the lack of flexibility and generality in some libraries designed for stream integration, illustrating the nuanced challenges developers face in leveraging GPU streams for performance gains.

Despite the benefits, integrating GPU streams into existing codes presents significant challenges. The essay details the manual synchronization required, the steep learning curve for correctly adopting asynchronous streams, and the potential necessity to develop custom kernels from scratch. It emphasizes the difficulties of porting code to GPU streams due to the asynchronous model's complexities and the limited support for seamless integration with established codes and libraries. Through illustrative examples, it underscores the technical barriers and the lack of flexibility and generality in some libraries designed for stream integration, illustrating the nuanced challenges developers face in leveraging GPU streams for performance gains.