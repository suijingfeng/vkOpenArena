Devices aren't required to have a memory type that is both device local and host visible.
Even though most do, the size might be limited. Maybe things have changed in the last few years,
but it used to be that PCI-E and BIOS limitations means 256 MB or maybe 512 MB was as much as you could get.
And finally, CPU writes to over PCI-E are going to be lower bandwidth than to the CPU's own memory. 
So even though using a staging buffer uses twice as much total bandwidth, 
if it can be done asynchronously on a transfer queue, it minimizes the time that the CPU and
graphics pipeline spend on that transfer. So whether using a staging buffer is a net win 
is going to depend on the specific CPU and GPU combination, and what your application is doing.

However, on SOCs like mobile devices or integrated GPUs, using a staging buffer should seldom if ever be a win.
Mobile GPUs shouldn't have limited device-local + host-visible heap sizes. 
Looking at a couple Windows integrated GPUs on vulkan.gpuinfo.org, it looks like modern Intel integrated GPUs
don't have such limits either, but AMD integrated GPUs still do (I only looked at a few random samples, YMMV).

All this makes it hard to give a clear "always do X" recommendation. Personally, I would generally do this:

If I just want one code path that works everywhere and am not worried about performance or memory footprint,
use a staging buffer. This is probably a good choice for discrete GPUs, but suboptimal for integrated/SOC GPUs.

Otherwise, keep the staging buffer as a fallback path, but use a shared device-local/host-visible pool
when there's a big enough one available. When I start trying to get every last bit of performance,
then tune the above to prefer staging buffers with asynchronous transfers for some kinds of uploads 
on discrete GPUs, when I have data showing it's a net win.
