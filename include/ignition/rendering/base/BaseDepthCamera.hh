/*
 * Copyright (C) 2018 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifndef IGNITION_RENDERING_BASE_BASEDEPTHCAMERA_HH_
#define IGNITION_RENDERING_BASE_BASEDEPTHCAMERA_HH_

#include <string>

#include <ignition/math/Matrix3.hh>
#include <ignition/math/Pose3.hh>

#include <ignition/common/Event.hh>
#include <ignition/common/Console.hh>

#include "ignition/rendering/DepthCamera.hh"
#include "ignition/rendering/Image.hh"
#include "ignition/rendering/RenderEngine.hh"
#include "ignition/rendering/Scene.hh"
#include "ignition/rendering/base/BaseRenderTarget.hh"
#include "ignition/rendering/base/BaseCamera.hh"

namespace ignition
{
  namespace rendering
  {
    inline namespace IGNITION_RENDERING_VERSION_NAMESPACE {
    template <class T>
    class IGNITION_RENDERING_VISIBLE BaseDepthCamera :
      public virtual DepthCamera,
      protected virtual BaseCamera<T>,
      public virtual T
    {
      protected: BaseDepthCamera();

      public: virtual ~BaseDepthCamera();

      public: virtual void CreateDepthTexture();

      public: virtual const float *DepthData() const;

      public: virtual ignition::common::ConnectionPtr ConnectNewDepthFrame(
          std::function<void(const float *, unsigned int, unsigned int,
          unsigned int, const std::string &)>  _subscriber);

      public: virtual ignition::common::ConnectionPtr ConnectNewRGBPointCloud(
          std::function<void(const float *, unsigned int, unsigned int,
          unsigned int, const std::string &)>  _subscriber);
    };

    //////////////////////////////////////////////////
    template <class T>
    BaseDepthCamera<T>::BaseDepthCamera()
    {
    }

    //////////////////////////////////////////////////
    template <class T>
    BaseDepthCamera<T>::~BaseDepthCamera()
    {
    }

    //////////////////////////////////////////////////
    template <class T>
    void BaseDepthCamera<T>::CreateDepthTexture()
    {
    }

    //////////////////////////////////////////////////
    template <class T>
    const float *BaseDepthCamera<T>::DepthData() const
    {
      return nullptr;
    }

    //////////////////////////////////////////////////
    template <class T>
    ignition::common::ConnectionPtr BaseDepthCamera<T>::ConnectNewDepthFrame(
          std::function<void(const float *, unsigned int, unsigned int,
          unsigned int, const std::string &)>)
    {
      return nullptr;
    }

    //////////////////////////////////////////////////
    template <class T>
    ignition::common::ConnectionPtr BaseDepthCamera<T>::ConnectNewRGBPointCloud(
          std::function<void(const float *, unsigned int, unsigned int,
          unsigned int, const std::string &)>)
    {
      return nullptr;
    }
  }
  }
}
#endif
