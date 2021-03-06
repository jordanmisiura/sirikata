/*  Meru
 *  SequentialWorkQueue.hpp
 *
 *  Copyright (c) 2009, Stanford University
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "Schedulable.hpp"
#include "MeruDefs.hpp"
#include "Singleton.hpp"
#include "Event.hpp"
#include "util/ThreadSafeQueue.hpp"
namespace Meru {
/**
 * This SequentialWorkQueue class allows us to defer nonthreadsafe jobs
 * until a frame occurs that has little enough work that it is safe to
 * perform the operation without resulting in a framerate jitter
 * Use WorkQueue for threadsafe operations
 */
class SequentialWorkQueue:public ManualSingleton<SequentialWorkQueue>, public Schedulable {

protected:
    /// A list of work to be processed. Functions return true if they are done, false if they need to be called again
    std::queue<std::tr1::function<bool()> >mWork;
    ThreadSafeQueue<std::tr1::function<bool()> > mThreadSafeWorkQueue;

    ///Processes all work in the mWork queue and empties it
    void processAllWork();

public:
    unsigned int numSchedulableJobs();
    /**
     * processes a single job in the work queue if one exists
     * \returns true if there are any remaining jobs on the work queue
     */
    bool processOneJob();
    /**
     * constructs the work queue class
     */
    SequentialWorkQueue();
    /// destructs the work queue class, processes remaining tasks and stops it from being a singleton
    virtual ~SequentialWorkQueue();
    /**
     * Adds a job to the work queue to be run.
     */
    void queueWork(const std::tr1::function<bool()>&work);
    /**
     * Adds a job to the thread safe work queue to be run.
     * or as it's otherwise known: get back to the main thread, @*&#$ !
     */
    void threadSafeQueueWork(const std::tr1::function<bool()>&work);
};

}
