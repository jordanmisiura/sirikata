/*  Meru
 *  GraphicsResourceManager.hpp
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
#ifndef _GRAPHICS_RESOURCE_MANAGER_HPP_
#define _GRAPHICS_RESOURCE_MANAGER_HPP_

#include "GraphicsResource.hpp"
#include "MeruDefs.hpp"
#include <oh/ProxyObject.hpp>
#include "Singleton.hpp"
#include "Event.hpp"

namespace Meru {

class DependencyManager;
class GraphicsEntity;

class GraphicsResourceManager : public ManualSingleton<GraphicsResourceManager>
{
protected:
  struct GraphicsResourcePriorityLessThanFunctor
  {
    bool operator()(const std::pair<float, GraphicsResource*>& key1, const std::pair<float, GraphicsResource*>& key2) const {
      if (key1.first != key2.first)
        return key1.first > key2.first;
      return key1.second < key2.second;
    }
  };

  typedef std::set<std::pair<float, GraphicsResource *>, GraphicsResourcePriorityLessThanFunctor> ResourcePriorityQueue;

public:

  GraphicsResourceManager();
  virtual ~GraphicsResourceManager();

  //virtual void loadMesh(WeakProxyPtr proxy, const String &meshName);

  void computeLoadedSet();

  SharedResourcePtr getResourceEntity(const SpaceObjectReference &id, GraphicsEntity *graphicsEntity);
  SharedResourcePtr getResourceAsset(const URI &id, GraphicsResource::Type resourceType);

  DependencyManager* getDependencyManager() {
    return mDependencyManager;
  }

  void unregisterResource(GraphicsResource* resource);

  float getBudget() {
    return mBudget / (1024.0f * 1024.0f);
  }

  void setBudget(float budget) {
    mBudget = budget * (1024.0f * 1024.0f);
  }

  void updateLoadValue(GraphicsResource* resource, float oldValue);
  void registerLoad(GraphicsResource* resource, float oldValue);
  void unregisterLoad(GraphicsResource* resource);
  void setEnabled(bool enabled) {
    mEnabled = enabled;
  }

protected:

  WeakResourcePtr getResource(const String &id);
  EventResponse tick(const EventPtr &evtPtr);

  std::map<String, WeakResourcePtr> mIDResourceMap;
  std::set<GraphicsResource *> mResources;
  std::set<GraphicsResource *> mToUnload;
  std::set<GraphicsResource *> mEntities;
  //std::set<GraphicsResource *> mMeshes;
  ResourcePriorityQueue mQueue;

  unsigned int mEpoch;
  DependencyManager *mDependencyManager;
  float mBudget;
  SubscriptionId mTickListener;
  bool mEnabled;
};

}

#endif
