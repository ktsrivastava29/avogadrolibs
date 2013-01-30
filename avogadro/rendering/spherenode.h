/******************************************************************************

  This source file is part of the Avogadro project.

  Copyright 2013 Kitware, Inc.

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

******************************************************************************/

#ifndef AVOGADRO_RENDERING_SPHERENODE_H
#define AVOGADRO_RENDERING_SPHERENODE_H

#include "geometrynode.h"

#include <avogadro/core/vector.h>

namespace Avogadro {
namespace Rendering {

struct SphereColor
{
  SphereColor(Vector3f centre, float r, Vector3ub c)
    : center(centre), radius(r), color(c) {}
  Vector3f center;
  float radius;
  Vector3ub color;
};

/**
 * @class SphereNode spherenode.h <avogadro/rendering/spherenode.h>
 * @brief The SphereNode class contains one or more spheres.
 * @author Marcus D. Hanwell
 *
 * This node is capaable of storing the geometry for one or more spheres in the
 * scene. A sphere is defined by a center point, a radius and a color. If the
 * spheres are not a densely packed one-to-one mapping with the objects indices
 * they can also optionally use an identifier that will point to some numberic
 * ID for the purposes of picking.
 */

class AVOGADRORENDERING_EXPORT SphereNode : public GeometryNode
{
public:
  explicit SphereNode(Node *parent = 0);
  ~SphereNode();

  void render(const Camera &camera);

  /**
   * Add a sphere to the scene object.
   */
  void addSphere(const Vector3f &position, const Vector3ub &color, float radius);

  /**
   * Get a reference to the spheres.
   */
  std::vector<SphereColor>& spheres() { return m_spheres; }
  const std::vector<SphereColor>& spheres() const { return m_spheres; }

  /**
   * Clear the contents of the node.
   */
  void clear();

  /**
   * Get the number of spheres in the node object.
   */
  size_t size() const { return m_spheres.size(); }

private:
  std::vector<SphereColor> m_spheres;
  std::vector<size_t> m_indices;

  bool m_dirty;

  class Private;
  Private *d;
};

} // End namespace Rendering
} // End namespace Avogadro

#endif // AVOGADRO_RENDERING_SPHERENODE_H
